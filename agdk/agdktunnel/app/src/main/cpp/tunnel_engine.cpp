/*
 * Copyright 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "tunnel_engine.hpp"
#include "loader_scene.hpp"
#include "welcome_scene.hpp"

#include "filesystem_manager.h"
#include "platform_event_loop.h"
#include "android/platform_util_android.h"
#include "simple_renderer/renderer_interface.h"

#include "game-activity/GameActivity.h"
#include "gni/gni.h"

static TunnelEngine *_singleton = NULL;

static const char *VIBRATOR_SYSTEM_STRING = "vibrator";
static const char *VIBRATOR_MANAGER_SYSTEM_STRING = "vibrator_manager";

TunnelEngine *TunnelEngine::GetInstance() {
  MY_ASSERT(_singleton != NULL);
  return _singleton;
}

// Base class hack until the rest of the code gets refactored
NativeEngine *NativeEngine::GetInstance() {
  MY_ASSERT(_singleton != NULL);
  return _singleton;
}

TunnelEngine::TunnelEngine(struct android_app *app) : NativeEngine(app) {
  // only one instance of the engine may exist!
  MY_ASSERT(_singleton == NULL);
  _singleton = this;

  mGameAssetManager = new GameAssetManager(app->activity->assetManager,
                                           app->activity->vm,
                                           app->activity->javaGameActivity);
  mTextureManager = NULL;
  mGfxManager = NULL;

  // Initialize the GNI runtime. This function needs to be called before any
  // call to the wrapper code (the VibrationHelper depends on this).
  GniCore_init(app->activity->vm, app->activity->javaGameActivity);

  // Initialize the vibration helper, used to vibrate the device if supported
  String *vibratorString = String_fromCString(VIBRATOR_SYSTEM_STRING);
  String *vibrationManagerString = String_fromCString(VIBRATOR_MANAGER_SYSTEM_STRING);
  mVibrationHelper = new VibrationHelper(app->activity->javaGameActivity,
                                         String_getJniReference(vibratorString),
                                         String_getJniReference(vibrationManagerString));
  String_destroy(vibratorString);
  String_destroy(vibrationManagerString);

  mTuningManager = new TuningManager(GetJniEnv(), app->activity->javaGameActivity, app->config);

  // Set fields retrieved through JNI
  // Find the Java class
  jclass activityClass = GetJniEnv()->GetObjectClass(mApp->activity->javaGameActivity);

  // Flag to find if cloud save is enabled
  jmethodID isPlayGamesServicesLinkedID =
      GetJniEnv()->GetMethodID(activityClass, "isPlayGamesServicesLinked", "()Z");
  mCloudSaveEnabled = (bool) GetJniEnv()->CallBooleanMethod(
      mApp->activity->javaGameActivity, isPlayGamesServicesLinkedID);

  char *internalStorage = (char *)FilesystemManager::GetInstance().GetRootPath(
  FilesystemManager::kRootPathInternalStorage).c_str();
  mDataStateMachine = new DataLoaderStateMachine(mCloudSaveEnabled, internalStorage);

  // Flags to control how the IME behaves.
  constexpr int InputType_dot_TYPE_CLASS_TEXT = 1;
  constexpr int IME_ACTION_NONE = 1;
  constexpr int IME_FLAG_NO_FULLSCREEN = 33554432;

  GameActivity_setImeEditorInfo(app->activity, InputType_dot_TYPE_CLASS_TEXT,
                                IME_ACTION_NONE, IME_FLAG_NO_FULLSCREEN);

  WelcomeScene::InitAboutText(GetJniEnv(), app->activity->javaGameActivity);

  for (int i = 0; i < OURKEY_COUNT; ++i) {
    mJoyKeyState[i] = false;
  }
}

TunnelEngine::~TunnelEngine() {
  // Make sure any active scene is deleted to release references to its
  // graphic resources
  SceneManager::GetInstance()->PrepareShutdown();

  if (mTextureManager != NULL) {
    delete mTextureManager;
  }
  if (mGfxManager != NULL) {
    delete mGfxManager;
    mGfxManager = NULL;
    simple_renderer::Renderer::ShutdownInstance();
  }
  if (mVibrationHelper != NULL) {
    delete mVibrationHelper;
  }
  if (mTuningManager != NULL) {
    delete mTuningManager;
  }
  if (mGameAssetManager != NULL) {
    delete mGameAssetManager;
  }
  if (mDataStateMachine != NULL) {
    delete mDataStateMachine;
  }

  _singleton = NULL;
}

void TunnelEngine::GameLoop() {
  while (!mQuitting) {
    if (!mDisplayInitialized) {
      PlatformEventLoop::GetInstance().PollEvents();
      if (mApp->window == NULL || !AttemptDisplayInitialization()) {
        usleep(1000);
        continue;
      }
      mHasStarted = true;
    } else {
      PlatformEventLoop::GetInstance().PollEvents();
      PollGameController();
      mGameAssetManager->UpdateGameAssetManager();
      if (mApp->textInputState) {
        struct CookedEvent ev;
        ev.type = COOKED_EVENT_TYPE_TEXT_INPUT;
        ev.textInputState = true;
        ProcessCookedEvent(&ev);
        mApp->textInputState = 0;
      }

      if (IsAnimating()) {
        DoFrame();
      }
    }
  }
}

bool TunnelEngine::CheckRenderPrerequisites() {
  if (mGfxManager == NULL) {
    InitializeGfxManager();
  }
  return true;
}

void TunnelEngine::DoFirstFrameSetup() {
  SceneManager *mgr = SceneManager::GetInstance();
  mgr->RequestNewScene(new LoaderScene());
  mgr->DoFrame();
  mgr->StartGraphics();
}

void TunnelEngine::InitializeGfxManager() {
  // Initialize renderer and resources once we have a valid surface to render to
  simple_renderer::Renderer::SetSwapchainHandle(mSwapchainHandle);
  if (mIsVulkan) {
    simple_renderer::Renderer::SetRendererAPI(simple_renderer::Renderer::kAPI_Vulkan);
  } else {
    simple_renderer::Renderer::SetRendererAPI(simple_renderer::Renderer::kAPI_GLES);
  }
  mGfxManager = new GfxManager(mIsVulkan, mSurfWidth, mSurfHeight);
  if (mTextureManager == NULL) {
    mTextureManager = new TextureManager();
  }
}

void TunnelEngine::ScreenSizeChanged() {
  if (mGfxManager != NULL) {
    mGfxManager->UpdateDisplaySize(mSurfWidth, mSurfHeight);
  }
}

void TunnelEngine::SetInputSdkContext(int context) {
  jclass activityClass = GetJniEnv()->GetObjectClass(mApp->activity->javaGameActivity);
  jmethodID setInputContextID =
      GetJniEnv()->GetMethodID(activityClass, "setInputContext", "(I)V");
  GetJniEnv()->CallVoidMethod(
      mApp->activity->javaGameActivity, setInputContextID, (jint)context);
}

DataLoaderStateMachine *TunnelEngine::BeginSavedGameLoad() {
  if (IsCloudSaveEnabled()) {
    ALOGI("Scheduling task to load cloud data through JNI");
    jclass activityClass = GetJniEnv()->GetObjectClass(mApp->activity->javaGameActivity);
    jmethodID loadCloudCheckpointID =
        GetJniEnv()->GetMethodID(activityClass, "loadCloudCheckpoint", "()V");
    GetJniEnv()->CallVoidMethod(mApp->activity->javaGameActivity, loadCloudCheckpointID);
  } else {
    mDataStateMachine->LoadLocalProgress();
  }
  return mDataStateMachine;
}

bool TunnelEngine::SaveProgress(int level, bool forceSave) {
  if (!forceSave) {
    if (level <= mDataStateMachine->getLevelLoaded()) {
      // nothing to do
      ALOGI("No need to save level, current = %d, saved = %d",
            level, mDataStateMachine->getLevelLoaded());
      return false;
    } else if (!IsCheckpointLevel(level)) {
      ALOGI("Current level %d is not a checkpoint level. Nothing to save.", level);
      return false;
    }
  }

  // Save state locally and to the cloud if it is enabled
  ALOGI("Saving progress to LOCAL FILE: level %d", level);
  mDataStateMachine->SaveLocalProgress(level);
  if (IsCloudSaveEnabled()) {
    ALOGI("Saving progress to the cloud: level %d", level);
    SaveGameToCloud(level);
  }
  return true;
}

void TunnelEngine::SaveGameToCloud(int level) {
  MY_ASSERT(GetJniEnv() && IsCloudSaveEnabled());
  ALOGI("Scheduling task to save cloud data through JNI");
  jclass activityClass = GetJniEnv()->GetObjectClass(mApp->activity->javaGameActivity);
  jmethodID saveCloudCheckpointID =
      GetJniEnv()->GetMethodID(activityClass, "saveCloudCheckpoint", "(I)V");
  GetJniEnv()->CallVoidMethod(
      mApp->activity->javaGameActivity, saveCloudCheckpointID, (jint)level);
}

void TunnelEngine::ReportJoyKeyState(int keyCode, bool state) {
  bool wentDown = !mJoyKeyState[keyCode] && state;
  bool wentUp = mJoyKeyState[keyCode] && !state;
  mJoyKeyState[keyCode] = state;

  struct CookedEvent ev{0};
  ev.keyCode = keyCode;

  if (wentUp) {
    ev.type = COOKED_EVENT_TYPE_KEY_UP;
    ProcessCookedEvent(&ev);
  } else if (wentDown) {
    ev.type = COOKED_EVENT_TYPE_KEY_DOWN;
    ProcessCookedEvent(&ev);
  }
}

void TunnelEngine::ReportJoyKeyStatesFromAxis(float x, float y) {
  ReportJoyKeyState(OURKEY_LEFT, x < -0.5f);
  ReportJoyKeyState(OURKEY_RIGHT, x > 0.5f);
  ReportJoyKeyState(OURKEY_UP, y < -0.5f);
  ReportJoyKeyState(OURKEY_DOWN, y > 0.5f);
}

void TunnelEngine::CheckControllerButton(const uint32_t buttonsDown,
                                         const InputAction &inputAction) {
  if (buttonsDown & inputAction.buttonMask) {
    ReportJoyKeyState(inputAction.actionCode, true);
  } else if (mPrevButtonsDown & inputAction.buttonMask) {
    ReportJoyKeyState(inputAction.actionCode, false);
  }
}

void TunnelEngine::PollGameController() {
  GameControllerManager &gcManager = UserInputManager::GetInstance().GetGameControllerManager();

  if (gcManager.GetControllerSupport()) {
    // We only read game controller index 0
    GameController &gc = gcManager.GetGameController(0);
    if (gc.GetControllerActive()) {
      const GameController::GameControllerData &controllerData = gc.GetControllerData();

      // Generate events from buttons
      for (int i = 0; i < INPUT_ACTION_COUNT; ++i) {
        CheckControllerButton(controllerData.buttons_down, INPUT_ACTIONS[i]);
      }

      // Generate an event for the stick position
      struct CookedEvent ev;
      memset(&ev, 0, sizeof(ev));
      ev.type = COOKED_EVENT_TYPE_JOY;
      ev.joyX = controllerData.left_stick.stick_x;
      ev.joyY = controllerData.left_stick.stick_y;
      ProcessCookedEvent(&ev);

      // Also generate directional button events from the stick position
      ReportJoyKeyStatesFromAxis(ev.joyX, ev.joyY);

      // Update our prev variable so we can detect delta changes from down to up
      mPrevButtonsDown = controllerData.buttons_down;
    }
  }
}

// TODO: rename the methods according to your package name
extern "C" jboolean Java_com_google_sample_agdktunnel_PGSManager_isLoadingWorkInProgress(
    JNIEnv */*env*/, jobject /*pgsManager*/) {
  TunnelEngine *instance = TunnelEngine::GetInstance();
  return (jboolean)!instance->GetDataStateMachine()->isLoadingDataCompleted();
}

extern "C" void Java_com_google_sample_agdktunnel_PGSManager_savedStateInitLoading(
    JNIEnv */*env*/, jobject /*pgsManager*/) {
  TunnelEngine *instance = TunnelEngine::GetInstance();
  instance->GetDataStateMachine()->init();
}

extern "C" void Java_com_google_sample_agdktunnel_PGSManager_authenticationCompleted(
    JNIEnv */*env*/, jobject /*pgsManager*/) {
  TunnelEngine *instance = TunnelEngine::GetInstance();
  instance->GetDataStateMachine()->authenticationCompleted();
}

extern "C" void Java_com_google_sample_agdktunnel_PGSManager_authenticationFailed(
    JNIEnv */*env*/, jobject /*pgsManager*/) {
  TunnelEngine *instance = TunnelEngine::GetInstance();
  instance->GetDataStateMachine()->authenticationFailed();
}

extern "C" void Java_com_google_sample_agdktunnel_PGSManager_savedStateSnapshotNotFound(
    JNIEnv */*env*/, jobject /*pgsManager*/) {
  TunnelEngine *instance = TunnelEngine::GetInstance();
  instance->GetDataStateMachine()->savedStateSnapshotNotFound();
}

extern "C" void Java_com_google_sample_agdktunnel_PGSManager_savedStateCloudDataFound(
    JNIEnv */*env*/, jobject /*pgsManagerl*/) {
  TunnelEngine *instance = TunnelEngine::GetInstance();
  instance->GetDataStateMachine()->savedStateCloudDataFound();
}

extern "C" void Java_com_google_sample_agdktunnel_PGSManager_savedStateLoadingFailed(
    JNIEnv */*env*/, jobject /*pgsManager*/) {
  TunnelEngine *instance = TunnelEngine::GetInstance();
  instance->GetDataStateMachine()->savedStateLoadingFailed();
}

extern "C" void Java_com_google_sample_agdktunnel_PGSManager_savedStateLoadingCompleted(
    JNIEnv */*env*/, jobject /*pgsManager*/, jint level) {
  TunnelEngine *instance = TunnelEngine::GetInstance();
  instance->GetDataStateMachine()->savedStateLoadingCompleted(level);
}