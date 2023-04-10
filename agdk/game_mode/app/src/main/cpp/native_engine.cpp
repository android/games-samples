/*
 * Copyright 2022 The Android Open Source Project
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
#include "native_engine.h"

#include <android/window.h>

#include "common.h"
#include "demo_scene.h"
#include "game_mode_manager.h"
#include "imgui_manager.h"
#include "input_util.h"
#include "scene_manager.h"

// verbose debug logs on?
#define VERBOSE_LOGGING 1

#if VERBOSE_LOGGING
#define VLOGD ALOGI
#else
#define VLOGD
#endif

// max # of GL errors to print before giving up
#define MAX_GL_ERRORS 200

static bool all_motion_filter(const GameActivityMotionEvent *event) {
  // Process all motion events
  return true;
}

static NativeEngine *_singleton = NULL;

// workaround for internal bug b/149866792
static NativeEngineSavedState appState = {false};

NativeEngine::NativeEngine(struct android_app *app) {
  ALOGI("NativeEngine: initializing.");
  mApp = app;
  mHasFocus = mIsVisible = mHasWindow = false;
  mHasGLObjects = false;
  mEglDisplay = EGL_NO_DISPLAY;
  mEglSurface = EGL_NO_SURFACE;
  mEglContext = EGL_NO_CONTEXT;
  mEglConfig = 0;
  mSurfWidth = mSurfHeight = 0;
  mSurfNativeWidth = mSurfNativeHeight = 0;
  mGameMode = 0;
  mApiVersion = 0;
  mScreenDensity = AConfiguration_getDensity(app->config);
  mActiveAxisIds = 0;
  mJniEnv = NULL;
  mImGuiManager = NULL;
  memset(&mState, 0, sizeof(mState));
  mIsFirstFrame = true;

  app->motionEventFilter = all_motion_filter;

  if (app->savedState != NULL) {
    // we are starting with previously saved state -- restore it
    mState = *(struct NativeEngineSavedState *)app->savedState;
  }

  // only one instance of NativeEngine may exist!
  MY_ASSERT(_singleton == NULL);
  _singleton = this;

  // Initialize Swappy to adjuest swap timing properly.
  ALOGI("Calling SwappyGL_init");
  SwappyGL_init(GetJniEnv(), mApp->activity->javaGameActivity);
  SwappyGL_setSwapIntervalNS(SWAPPY_SWAP_60FPS);

  VLOGD("NativeEngine: querying API level.");
  ALOGI("NativeEngine: API version %d.", mApiVersion);
  ALOGI("NativeEngine: Density %d", mScreenDensity);
}

NativeEngine *NativeEngine::GetInstance() {
  MY_ASSERT(_singleton != NULL);
  return _singleton;
}

NativeEngine::~NativeEngine() {
  // Destroy Swappy instance.
  SwappyGL_destroy();

  VLOGD("NativeEngine: destructor running");
  KillContext();
  if (mImGuiManager != NULL) {
    delete mImGuiManager;
  }
  if (mJniEnv) {
    ALOGI("Detaching current thread from JNI.");
    mApp->activity->vm->DetachCurrentThread();
    ALOGI("Current thread detached from JNI.");
    mJniEnv = NULL;
  }
  _singleton = NULL;
}

static void _handle_cmd_proxy(struct android_app *app, int32_t cmd) {
  NativeEngine *engine = (NativeEngine *)app->userData;
  engine->HandleCommand(cmd);
}

bool NativeEngine::IsAnimating() {
  return mHasFocus && mIsVisible && mHasWindow;
}

static bool _cooked_event_callback(struct CookedEvent *event) {
  SceneManager *mgr = SceneManager::GetInstance();
  PointerCoords coords;
  memset(&coords, 0, sizeof(coords));
  coords.x_ = event->motion_x_;
  coords.y_ = event->motion_y_;
  coords.min_x_ = event->motion_min_x_;
  coords.max_x_ = event->motion_max_x_;
  coords.min_y_ = event->motion_min_y_;
  coords.max_y_ = event->motion_max_y_;
  coords.is_screen_ = event->motion_is_on_screen_;

  switch (event->type_) {
    case COOKED_EVENT_TYPE_POINTER_DOWN:
      mgr->OnPointerDown(event->motion_pointer_id_, &coords);
      return true;
    case COOKED_EVENT_TYPE_POINTER_UP:
      mgr->OnPointerUp(event->motion_pointer_id_, &coords);
      return true;
    case COOKED_EVENT_TYPE_POINTER_MOVE:
      mgr->OnPointerMove(event->motion_pointer_id_, &coords);
      return true;
    default:
      return false;
  }
}

// This is here and not in input_util.cpp due to being specific to the
// GameActivity version
static bool _cook_game_activity_motion_event(
    GameActivityMotionEvent *motionEvent, CookedEventCallback callback) {
  if (motionEvent->pointerCount > 0) {
    int action = motionEvent->action;
    int actionMasked = action & AMOTION_EVENT_ACTION_MASK;
    int ptrIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                   AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

    if (ptrIndex < motionEvent->pointerCount) {
      struct CookedEvent ev;
      memset(&ev, 0, sizeof(ev));

      if (actionMasked == AMOTION_EVENT_ACTION_DOWN ||
          actionMasked == AMOTION_EVENT_ACTION_POINTER_DOWN) {
        ev.type_ = COOKED_EVENT_TYPE_POINTER_DOWN;
      } else if (actionMasked == AMOTION_EVENT_ACTION_UP ||
                 actionMasked == AMOTION_EVENT_ACTION_POINTER_UP) {
        ev.type_ = COOKED_EVENT_TYPE_POINTER_UP;
      } else {
        ev.type_ = COOKED_EVENT_TYPE_POINTER_MOVE;
      }

      ev.motion_pointer_id_ = motionEvent->pointers[ptrIndex].id;
      ev.motion_is_on_screen_ =
          motionEvent->source == AINPUT_SOURCE_TOUCHSCREEN;
      ev.motion_x_ =
          GameActivityPointerAxes_getX(&motionEvent->pointers[ptrIndex]);
      ev.motion_y_ =
          GameActivityPointerAxes_getY(&motionEvent->pointers[ptrIndex]);

      if (ev.motion_is_on_screen_) {
        // use screen size as the motion range
        ev.motion_min_x_ = 0.0f;
        ev.motion_max_x_ = SceneManager::GetInstance()->GetScreenWidth();
        ev.motion_min_y_ = 0.0f;
        ev.motion_max_y_ = SceneManager::GetInstance()->GetScreenHeight();
      }

      return callback(&ev);
    }
  }
  return false;
}

void NativeEngine::GameLoop() {
  mApp->userData = this;
  mApp->onAppCmd = _handle_cmd_proxy;
  // mApp->onInputEvent = _handle_input_proxy;

  auto activity = NativeEngine::GetInstance()->GetAndroidApp()->activity;
  GameActivity_setWindowFlags(
      activity,
      AWINDOW_FLAG_KEEP_SCREEN_ON | AWINDOW_FLAG_TURN_SCREEN_ON |
          AWINDOW_FLAG_FULLSCREEN | AWINDOW_FLAG_SHOW_WHEN_LOCKED,
      0);
  UpdateSystemBarOffset();

  while (1) {
    int events;
    struct android_poll_source *source;

    // If not animating, block until we get an event; if animating, don't block.
    while ((ALooper_pollAll(IsAnimating() ? 0 : -1, NULL, &events,
                            (void **)&source)) >= 0) {
      // process event
      if (source != NULL) {
        source->process(mApp, source);
      }

      // are we exiting?
      if (mApp->destroyRequested) {
        return;
      }
    }

    HandleGameActivityInput();

    if (IsAnimating()) {
      DoFrame();
    }
  }
}

JNIEnv *NativeEngine::GetJniEnv() {
  if (!mJniEnv) {
    ALOGI("Attaching current thread to JNI.");
    if (0 != mApp->activity->vm->AttachCurrentThread(&mJniEnv, NULL)) {
      ALOGE("*** FATAL ERROR: Failed to attach thread to JNI.");
      ABORT_GAME;
    }
    MY_ASSERT(mJniEnv != NULL);
    ALOGI("Attached current thread to JNI, %p", mJniEnv);
  }

  return mJniEnv;
}

JNIEnv *NativeEngine::GetAppJniEnv() {
  if (!mAppJniEnv) {
    ALOGI("Attaching current thread to JNI.");
    if (0 != mApp->activity->vm->AttachCurrentThread(&mAppJniEnv, NULL)) {
      ALOGE("*** FATAL ERROR: Failed to attach thread to JNI.");
      ABORT_GAME;
    }
    MY_ASSERT(mAppJniEnv != NULL);
    ALOGI("Attached current thread to JNI, %p", mAppJniEnv);
  }

  return mAppJniEnv;
}

void NativeEngine::HandleCommand(int32_t cmd) {
  SceneManager *mgr = SceneManager::GetInstance();

  VLOGD("NativeEngine: handling command %d.", cmd);
  switch (cmd) {
    case APP_CMD_SAVE_STATE:
      // The system has asked us to save our current state.
      VLOGD("NativeEngine: APP_CMD_SAVE_STATE");
      mState.mHasFocus = mHasFocus;
      mApp->savedState = malloc(sizeof(mState));
      *((NativeEngineSavedState *)mApp->savedState) = mState;
      mApp->savedStateSize = sizeof(mState);
      break;
    case APP_CMD_INIT_WINDOW:
      // We have a window!
      VLOGD("NativeEngine: APP_CMD_INIT_WINDOW");
      if (mApp->window != NULL) {
        mHasWindow = true;

        // Set the window to Swappy instance.
        SwappyGL_setWindow(mApp->window);

        if (mApp->savedStateSize == sizeof(mState) &&
            mApp->savedState != nullptr) {
          mState = *((NativeEngineSavedState *)mApp->savedState);
          mHasFocus = mState.mHasFocus;
        } else {
          // Workaround APP_CMD_GAINED_FOCUS issue where the focus state is not
          // passed down from NativeActivity when restarting Activity
          mHasFocus = appState.mHasFocus;
        }
      }
      VLOGD("HandleCommand(%d): hasWindow = %d, hasFocus = %d", cmd,
            mHasWindow ? 1 : 0, mHasFocus ? 1 : 0);
      break;
    case APP_CMD_TERM_WINDOW:
      // The window is going away -- kill the surface
      VLOGD("NativeEngine: APP_CMD_TERM_WINDOW");
      KillSurface();
      mHasWindow = false;
      break;
    case APP_CMD_GAINED_FOCUS:
      VLOGD("NativeEngine: APP_CMD_GAINED_FOCUS");
      mHasFocus = true;
      mState.mHasFocus = appState.mHasFocus = mHasFocus;
      break;
    case APP_CMD_LOST_FOCUS:
      VLOGD("NativeEngine: APP_CMD_LOST_FOCUS");
      mHasFocus = false;
      mState.mHasFocus = appState.mHasFocus = mHasFocus;
      break;
    case APP_CMD_PAUSE:
      VLOGD("NativeEngine: APP_CMD_PAUSE");
      mgr->OnPause();
      VLOGD("NativeEngine: APP_CMD_PAUSE2");
      break;
    case APP_CMD_RESUME:
      VLOGD("NativeEngine: APP_CMD_RESUME");
      mgr->OnResume();
      break;
    case APP_CMD_STOP:
      VLOGD("NativeEngine: APP_CMD_STOP");
      mIsVisible = false;
      break;
    case APP_CMD_START:
      VLOGD("NativeEngine: APP_CMD_START");
      mIsVisible = true;
      break;
    case APP_CMD_WINDOW_RESIZED:
    case APP_CMD_CONFIG_CHANGED:
      VLOGD("NativeEngine: %s", cmd == APP_CMD_WINDOW_RESIZED
                                    ? "APP_CMD_WINDOW_RESIZED"
                                    : "APP_CMD_CONFIG_CHANGED");
      // Window was resized or some other configuration changed.
      // Note: we don't handle this event because we check the surface
      // dimensions every frame, so that's how we know it was resized. If you
      // are NOT doing that, then you need to handle this event!
      break;
    case APP_CMD_LOW_MEMORY:
      VLOGD("NativeEngine: APP_CMD_LOW_MEMORY");
      // system told us we have low memory. So if we are not visible, let's
      // cooperate by deallocating all of our graphic resources.
      if (!mHasWindow) {
        VLOGD("NativeEngine: trimming memory footprint (deleting GL objects).");
        KillGLObjects();
      }
      break;
    case APP_CMD_WINDOW_INSETS_CHANGED:
      VLOGD("NativeEngine: APP_CMD_WINDOW_INSETS_CHANGED");
      UpdateSystemBarOffset();
      break;
    default:
      VLOGD("NativeEngine: (unknown command).");
      break;
  }

  VLOGD("NativeEngine: STATUS: F%d, V%d, W%d, EGL: D %p, S %p, CTX %p, CFG %p",
        mHasFocus, mIsVisible, mHasWindow, mEglDisplay, mEglSurface,
        mEglContext, mEglConfig);
}

void NativeEngine::UpdateSystemBarOffset() {
  ARect insets;
  // Log all the insets types
  GameActivity_getWindowInsets(mApp->activity,
                               GAMECOMMON_INSETS_TYPE_SYSTEM_BARS, &insets);
  mSystemBarOffset = insets.top;
}

bool NativeEngine::HandleInput(AInputEvent *event) { return false; }

void NativeEngine::HandleGameActivityInput() {
  // Swap input buffers so we don't miss any events while processing
  // inputBuffer.
  android_input_buffer *inputBuffer = android_app_swap_input_buffers(mApp);
  // Early exit if no events.
  if (inputBuffer == nullptr) return;

  if (inputBuffer->keyEventsCount != 0) {
    android_app_clear_key_events(inputBuffer);
  }
  if (inputBuffer->motionEventsCount != 0) {
    for (uint64_t i = 0; i < inputBuffer->motionEventsCount; ++i) {
      GameActivityMotionEvent *motionEvent = &inputBuffer->motionEvents[i];
      // Didn't belong to a game controller, process it ourselves if it is a
      // touch event
      _cook_game_activity_motion_event(motionEvent, _cooked_event_callback);
    }
    android_app_clear_motion_events(inputBuffer);
  }
}

bool NativeEngine::InitDisplay() {
  if (mEglDisplay != EGL_NO_DISPLAY) {
    // nothing to do
    ALOGI("NativeEngine: no need to init display (already had one).");
    return true;
  }

  ALOGI("NativeEngine: initializing display.");
  mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (EGL_FALSE == eglInitialize(mEglDisplay, 0, 0)) {
    ALOGE("NativeEngine: failed to init display, error %d", eglGetError());
    return false;
  }

  return true;
}

bool NativeEngine::InitSurface() {
  // need a display
  MY_ASSERT(mEglDisplay != EGL_NO_DISPLAY);

  if (mEglSurface != EGL_NO_SURFACE) {
    // nothing to do
    ALOGI("NativeEngine: no need to init surface (already had one).");
    return true;
  }

  ALOGI("NativeEngine: initializing surface.");

  EGLint numConfigs;

  const EGLint attribs[] = {EGL_RENDERABLE_TYPE,
                            EGL_OPENGL_ES2_BIT,  // request OpenGL ES 2.0
                            EGL_SURFACE_TYPE,
                            EGL_WINDOW_BIT,
                            EGL_BLUE_SIZE,
                            8,
                            EGL_GREEN_SIZE,
                            8,
                            EGL_RED_SIZE,
                            8,
                            EGL_DEPTH_SIZE,
                            16,
                            EGL_NONE};

  // since this is a simple sample, we have a trivial selection process. We pick
  // the first EGLConfig that matches:
  eglChooseConfig(mEglDisplay, attribs, &mEglConfig, 1, &numConfigs);

  // create EGL surface
  mEglSurface =
      eglCreateWindowSurface(mEglDisplay, mEglConfig, mApp->window, NULL);
  if (mEglSurface == EGL_NO_SURFACE) {
    ALOGE("Failed to create EGL surface, EGL error %d", eglGetError());
    return false;
  }

  // keep the original surface size as native size
  eglQuerySurface(mEglDisplay, mEglSurface, EGL_WIDTH, &mSurfNativeWidth);
  eglQuerySurface(mEglDisplay, mEglSurface, EGL_HEIGHT, &mSurfNativeHeight);
  ALOGI("NativeEngine: native display size: %d x %d", mSurfNativeWidth,
        mSurfNativeHeight);

  ALOGI("NativeEngine: successfully initialized surface.");
  return true;
}

bool NativeEngine::InitContext() {
  // need a display
  MY_ASSERT(mEglDisplay != EGL_NO_DISPLAY);

  EGLint attribList[] = {EGL_CONTEXT_CLIENT_VERSION, 2,
                         EGL_NONE};  // OpenGL ES 2.0

  if (mEglContext != EGL_NO_CONTEXT) {
    // nothing to do
    ALOGI("NativeEngine: no need to init context (already had one).");
    return true;
  }

  ALOGI("NativeEngine: initializing context.");

  // create EGL context
  mEglContext = eglCreateContext(mEglDisplay, mEglConfig, NULL, attribList);
  if (mEglContext == EGL_NO_CONTEXT) {
    ALOGE("Failed to create EGL context, EGL error %d", eglGetError());
    return false;
  }

  ALOGI("NativeEngine: successfully initialized context.");

  return true;
}

void NativeEngine::ConfigureOpenGL() {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}

bool NativeEngine::PrepareToRender() {
  if (mEglDisplay == EGL_NO_DISPLAY || mEglSurface == EGL_NO_SURFACE ||
      mEglContext == EGL_NO_CONTEXT) {
    // create display if needed
    if (!InitDisplay()) {
      ALOGE("NativeEngine: failed to create display.");
      return false;
    }

    // create surface if needed
    if (!InitSurface()) {
      ALOGE("NativeEngine: failed to create surface.");
      return false;
    }

    // create context if needed
    if (!InitContext()) {
      ALOGE("NativeEngine: failed to create context.");
      return false;
    }

    ALOGI(
        "NativeEngine: binding surface and context (display %p, surface %p, "
        "context %p)",
        mEglDisplay, mEglSurface, mEglContext);

    // bind them
    if (EGL_FALSE ==
        eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext)) {
      ALOGE("NativeEngine: eglMakeCurrent failed, EGL error %d", eglGetError());
      HandleEglError(eglGetError());
    }

    // configure our global OpenGL settings
    ConfigureOpenGL();

    if (mImGuiManager == NULL) {
      mImGuiManager = new ImGuiManager();
    }
  }
  if (!mHasGLObjects) {
    ALOGI("NativeEngine: creating OpenGL objects.");
    if (!InitGLObjects()) {
      ALOGE("NativeEngine: unable to initialize OpenGL objects.");
      return false;
    }
  }

  // Keep the ImGui display size up to date
  mImGuiManager->SetDisplaySize(mSurfWidth, mSurfHeight, mScreenDensity);

  // ready to render
  return true;
}

void NativeEngine::KillGLObjects() {
  if (mHasGLObjects) {
    SceneManager *mgr = SceneManager::GetInstance();
    mgr->KillGraphics();
    mHasGLObjects = false;
  }
}

void NativeEngine::KillSurface() {
  ALOGI("NativeEngine: killing surface.");
  eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  if (mEglSurface != EGL_NO_SURFACE) {
    eglDestroySurface(mEglDisplay, mEglSurface);
    mEglSurface = EGL_NO_SURFACE;
  }
  ALOGI("NativeEngine: Surface killed successfully.");
}

void NativeEngine::KillContext() {
  ALOGI("NativeEngine: killing context.");

  // since the context is going away, we have to kill the GL objects
  KillGLObjects();

  eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

  if (mEglContext != EGL_NO_CONTEXT) {
    eglDestroyContext(mEglDisplay, mEglContext);
    mEglContext = EGL_NO_CONTEXT;
  }
  ALOGI("NativeEngine: Context killed successfully.");
}

void NativeEngine::KillDisplay() {
  // causes context and surface to go away too, if they are there
  ALOGI("NativeEngine: killing display.");
  KillContext();
  KillSurface();

  if (mEglDisplay != EGL_NO_DISPLAY) {
    ALOGI("NativeEngine: terminating display now.");
    eglTerminate(mEglDisplay);
    mEglDisplay = EGL_NO_DISPLAY;
  }
  ALOGI("NativeEngine: display killed successfully.");
}

bool NativeEngine::HandleEglError(EGLint error) {
  switch (error) {
    case EGL_SUCCESS:
      // nothing to do
      return true;
    case EGL_CONTEXT_LOST:
      ALOGW("NativeEngine: egl error: EGL_CONTEXT_LOST. Recreating context.");
      KillContext();
      return true;
    case EGL_BAD_CONTEXT:
      ALOGW("NativeEngine: egl error: EGL_BAD_CONTEXT. Recreating context.");
      KillContext();
      return true;
    case EGL_BAD_DISPLAY:
      ALOGW("NativeEngine: egl error: EGL_BAD_DISPLAY. Recreating display.");
      KillDisplay();
      return true;
    case EGL_BAD_SURFACE:
      ALOGW("NativeEngine: egl error: EGL_BAD_SURFACE. Recreating display.");
      KillSurface();
      return true;
    default:
      ALOGW("NativeEngine: unknown egl error: %d", error);
      return false;
  }
}

static void _log_opengl_error(GLenum err) {
  switch (err) {
    case GL_NO_ERROR:
      ALOGE("*** OpenGL error: GL_NO_ERROR");
      break;
    case GL_INVALID_ENUM:
      ALOGE("*** OpenGL error: GL_INVALID_ENUM");
      break;
    case GL_INVALID_VALUE:
      ALOGE("*** OpenGL error: GL_INVALID_VALUE");
      break;
    case GL_INVALID_OPERATION:
      ALOGE("*** OpenGL error: GL_INVALID_OPERATION");
      break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      ALOGE("*** OpenGL error: GL_INVALID_FRAMEBUFFER_OPERATION");
      break;
    case GL_OUT_OF_MEMORY:
      ALOGE("*** OpenGL error: GL_OUT_OF_MEMORY");
      break;
    default:
      ALOGE("*** OpenGL error: error %d", err);
      break;
  }
}

void NativeEngine::CheckGameMode() {
  GameModeManager &gameModeManager = GameModeManager::getInstance();
  int game_mode = gameModeManager.GetGameMode();
  if (game_mode != mGameMode) {
    // game mode changed, make necessary adjustments to the game
    // in this sample, we are capping the frame rate and the resolution
    SceneManager *sceneManager = SceneManager::GetInstance();
    NativeEngine *nativeEngine = NativeEngine::GetInstance();
    int native_width = nativeEngine->GetNativeWidth();
    int native_height = nativeEngine->GetNativeHeight();
    int preferred_width;
    int preferred_height;
    int32_t preferredSwapInterval = SWAPPY_SWAP_30FPS;
    if (game_mode == GAME_MODE_STANDARD) {
      // GAME_MODE_STANDARD : fps: 30, res: 1/2
      preferredSwapInterval = SWAPPY_SWAP_30FPS;
      preferred_width = native_width / 2;
      preferred_height = native_height / 2;
    } else if (game_mode == GAME_MODE_PERFORMANCE) {
      // GAME_MODE_PERFORMANCE : fps: 60, res: 1/1
      preferredSwapInterval = SWAPPY_SWAP_60FPS;
      preferred_width = native_width;
      preferred_height = native_height;
    } else if (game_mode == GAME_MODE_BATTERY) {
      // GAME_MODE_BATTERY : fps: 30, res: 1/4
      preferred_height = SWAPPY_SWAP_30FPS;
      preferred_width = native_width / 4;
      preferred_height = native_height / 4;
    } else {  // game_mode == 0 : fps: 30, res: 1/2
      // GAME_MODE_UNSUPPORTED
      preferredSwapInterval = SWAPPY_SWAP_30FPS;
      preferred_width = native_width / 2;
      preferred_height = native_height / 2;
    }
    ALOGI("GameMode SetPreferredSizeAndFPS: %d, %d, %d", preferred_width,
          preferred_height, preferredSwapInterval);
    sceneManager->SetPreferredSize(preferred_width, preferred_height);
    sceneManager->SetPreferredSwapInterval(preferredSwapInterval);
    mGameMode = game_mode;
  }
}

void NativeEngine::SwitchToPreferredDisplaySize() {
  SceneManager *mgr = SceneManager::GetInstance();
  ImGuiManager *gui_mgr = NativeEngine::GetInstance()->GetImGuiManager();

  int preferred_width = mgr->GetPreferredWidth();
  int preferred_height = mgr->GetPreferredHeight();
  bool bCanSwitch = preferred_width <= mSurfNativeWidth &&
                    preferred_height <= mSurfNativeHeight;
  bool bNeedSwitch =
      preferred_width != mSurfWidth && preferred_height != mSurfHeight;
  if (bCanSwitch && bNeedSwitch) {
    ALOGI(
        "GameMode canSwitch: %d needSwitch: %d, (%f) size: %d x %d, preferred: "
        "%d x %d",
        bCanSwitch, bNeedSwitch, gui_mgr->GetFontScale(), mSurfWidth,
        mSurfHeight, preferred_width, preferred_height);
    ANativeWindow_setBuffersGeometry(mApp->window, preferred_width,
                                     preferred_height, 0);

    int width, height;
    eglQuerySurface(mEglDisplay, mEglSurface, EGL_WIDTH, &width);
    eglQuerySurface(mEglDisplay, mEglSurface, EGL_HEIGHT, &height);

    ALOGI("GameMode switched (%d, %d) preferred: %d x %d", width, height,
          preferred_width, preferred_height);
  }

  // how big is the surface? We query every frame because it's cheap, and some
  // strange devices out there change the surface size without calling any
  // callbacks...
  int width, height;
  eglQuerySurface(mEglDisplay, mEglSurface, EGL_WIDTH, &width);
  eglQuerySurface(mEglDisplay, mEglSurface, EGL_HEIGHT, &height);

  if (width != mSurfWidth || height != mSurfHeight) {
    // notify scene manager that the surface has changed size
    ALOGI("NativeEngine: surface changed size %dx%d --> %dx%d", mSurfWidth,
          mSurfHeight, width, height);
    mSurfWidth = width;
    mSurfHeight = height;
    mgr->SetScreenSize(mSurfWidth, mSurfHeight);
    glViewport(0, 0, mSurfWidth, mSurfHeight);
  }
}

void NativeEngine::DoFrame() {
  // prepare to render (create context, surfaces, etc, if needed)
  if (!PrepareToRender()) {
    // not ready
    VLOGD("NativeEngine: preparation to render failed.");
    return;
  }

  SceneManager *mgr = SceneManager::GetInstance();
  //  ImGuiManager *guiManager = NativeEngine::GetInstance()->GetImGuiManager();

  CheckGameMode();
  SwitchToPreferredDisplaySize();

  // if this is the first frame, install the demo scene
  if (mIsFirstFrame) {
    mIsFirstFrame = false;
    ALOGI("GameMode first frame");
    mgr->RequestNewScene(&DemoScene::getInstance());
  }

  // render!
  mgr->DoFrame();

  if (mImGuiManager != NULL) {
    mImGuiManager->EndImGuiFrame();
  }

  // swap buffers
  if (!SwappyGL_swap(mEglDisplay, mEglSurface)) {  // failed to swap buffers...
    ALOGW("NativeEngine: SwappyGL_swap failed, EGL error %d", eglGetError());
    HandleEglError(eglGetError());
  }

  // print out GL errors, if any
  GLenum e;
  static int errorsPrinted = 0;
  while ((e = glGetError()) != GL_NO_ERROR) {
    if (errorsPrinted < MAX_GL_ERRORS) {
      _log_opengl_error(e);
      ++errorsPrinted;
      if (errorsPrinted >= MAX_GL_ERRORS) {
        ALOGE("*** NativeEngine: TOO MANY OPENGL ERRORS. NO LONGER PRINTING.");
      }
    }
  }
}

android_app *NativeEngine::GetAndroidApp() { return mApp; }

bool NativeEngine::InitGLObjects() {
  if (!mHasGLObjects) {
    SceneManager *mgr = SceneManager::GetInstance();
    mgr->StartGraphics();
    _log_opengl_error(glGetError());
    mHasGLObjects = true;
  }
  return true;
}
