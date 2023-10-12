/*
 * Copyright 2021 The Android Open Source Project
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

#include "common.hpp"
#include "input_util.hpp"
#include "joystick-support.hpp"
#include "scene_manager.hpp"
#include "loader_scene.hpp"
#include "native_engine.hpp"

#include "game-activity/GameActivity.h"
#include "memory_advice/memory_advice.h"
#include "paddleboat/paddleboat.h"
#include "welcome_scene.hpp"
#include "gni/gni.h"
#include "android/platform_util_android.h"
#include "simple_renderer/renderer_interface.h"

using namespace base_game_framework;

// verbose debug logs on?
#define VERBOSE_LOGGING 1

#if VERBOSE_LOGGING
#define VLOGD ALOGI
#else
#define VLOGD
#endif

// TODO: remove after merging SimpleRender vulkan implementation
// so we can actually use Vulkan
static bool s_disable_vulkan = true;

static const char *VIBRATOR_SYSTEM_STRING = "vibrator";
static const char *VIBRATOR_MANAGER_SYSTEM_STRING = "vibrator_manager";

static NativeEngine *_singleton = NULL;

// workaround for internal bug b/149866792
static NativeEngineSavedState appState = {false};

static void _GameControllerStatusCallback(const int32_t controllerIndex,
                                          const Paddleboat_ControllerStatus status, void *) {
    if (_singleton != NULL) {
        // Always make the most recently connected controller the active one
        if (status == PADDLEBOAT_CONTROLLER_JUST_CONNECTED) {
            _singleton->SetActiveGameControllerIndex(controllerIndex);
        } else if (status == PADDLEBOAT_CONTROLLER_JUST_DISCONNECTED) {
            // We only care if the controller that disconnected was the one
            // we are currently using
            if (controllerIndex == _singleton->GetActiveGameControllerIndex()) {
                // Default to no fallback controller, loop and look for another connected
                // one
                int32_t newControllerIndex = -1;

                for (int32_t i = 0; i < PADDLEBOAT_MAX_CONTROLLERS; ++i) {
                    if (i != controllerIndex &&
                        Paddleboat_getControllerStatus(i) == PADDLEBOAT_CONTROLLER_ACTIVE) {
                        newControllerIndex = i;
                        break;
                    }
                }
                _singleton->SetActiveGameControllerIndex(newControllerIndex);
            }
        }
    }
}

NativeEngine::NativeEngine(struct android_app *app) {
    ALOGI("NativeEngine: initializing.");
    mApp = app;
    mHasFocus = mHasStarted = mDisplayInitialized = false;
    mHasSwapchain = false;
    mHasGfxObjects = false;
    mSurfWidth = mSurfHeight = 0;
    mGameControllerIndex = -1;
    mApiVersion = 0;
    mScreenDensity = 0;
    mActiveAxisIds = 0;
    mJniEnv = NULL;
    mAppJniEnv = NULL;
    memset(&mState, 0, sizeof(mState));
    mSwapchainFrameHandle = DisplayManager::kInvalid_swapchain_handle;
    mSwapchainHandle = DisplayManager::kInvalid_swapchain_handle;
    mIsVulkan = false;
    mIsFirstFrame = true;

    mGameAssetManager = new GameAssetManager(app->activity->assetManager,
                                             app->activity->vm,
                                             app->activity->javaGameActivity);
    mTextureManager = NULL;
    mGfxManager = NULL;

    if (app->savedState != NULL) {
        // we are starting with previously saved state -- restore it
        mState = *(struct NativeEngineSavedState *) app->savedState;
    }

    // only one instance of NativeEngine may exist!
    MY_ASSERT(_singleton == NULL);
    _singleton = this;

    Paddleboat_init(GetJniEnv(), app->activity->javaGameActivity);
    Paddleboat_setControllerStatusCallback(_GameControllerStatusCallback, NULL);

    const MemoryAdvice_ErrorCode memoryError = MemoryAdvice_init(GetJniEnv(),
                                                                 app->activity->javaGameActivity);
    if (memoryError != MEMORYADVICE_ERROR_OK) {
        ALOGE("MemoryAdvice_init failed with error: %d", memoryError);
    } else {
        ALOGI("Initialized MemoryAdvice");
    }

    // Initialize the memory consumer, used to exercise the
    // Memory Advice library. Off by default.
    mMemoryConsumer = new MemoryConsumer(false);

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

    WelcomeScene::InitAboutText(GetJniEnv(), app->activity->javaGameActivity);

    // This is needed to allow controller events through to us.
    // By default, only touch-screen events are passed through, to match the
    // behaviour of NativeActivity.
    android_app_set_motion_event_filter(app, nullptr);

    // Flags to control how the IME behaves.
    constexpr int InputType_dot_TYPE_CLASS_TEXT = 1;
    constexpr int IME_ACTION_NONE = 1;
    constexpr int IME_FLAG_NO_FULLSCREEN = 33554432;

    GameActivity_setImeEditorInfo(app->activity, InputType_dot_TYPE_CLASS_TEXT,
                                  IME_ACTION_NONE, IME_FLAG_NO_FULLSCREEN);

    // Set fields retrieved through JNI
    // Find the Java class
    jclass activityClass = GetJniEnv()->GetObjectClass(mApp->activity->javaGameActivity);

    // Field that stores the path to save files to internal storage
    jmethodID getInternalStoragePathID = GetJniEnv()->GetMethodID(
            activityClass, "getInternalStoragePath", "()Ljava/lang/String;");
    jobject jInternalStoragePath = GetJniEnv()->CallObjectMethod(
            mApp->activity->javaGameActivity, getInternalStoragePathID);
    jboolean isCopy;
    const char * str = GetJniEnv()->GetStringUTFChars((jstring)jInternalStoragePath, &isCopy);
    char *internalStoragePath = new char[strlen(str) + 2];
    strcpy(internalStoragePath, str);

    if (isCopy == JNI_TRUE) {
        GetJniEnv()->ReleaseStringUTFChars((jstring)jInternalStoragePath, str);
    }
    GetJniEnv()->DeleteLocalRef(jInternalStoragePath);

    // Flag to find if cloud save is enabled
    jmethodID isPlayGamesServicesLinkedID =
            GetJniEnv()->GetMethodID(activityClass, "isPlayGamesServicesLinked", "()Z");
    mCloudSaveEnabled = (bool) GetJniEnv()->CallBooleanMethod(
            mApp->activity->javaGameActivity, isPlayGamesServicesLinkedID);

    mDataStateMachine = new DataLoaderStateMachine(mCloudSaveEnabled, internalStoragePath);

    // Temp init code until rest of BaseGameFramework refactoring goes in
    PlatformUtilAndroid::SetAndroidApp(app);
    PlatformUtilAndroid::SetMainThreadJniEnv(GetJniEnv());
    PlatformUtilAndroid::SetActivityClassObject(app->activity->javaGameActivity);
}

NativeEngine *NativeEngine::GetInstance() {
    MY_ASSERT(_singleton != NULL);
    return _singleton;
}

NativeEngine::~NativeEngine() {
    VLOGD("NativeEngine: destructor running");
    if (mTextureManager != NULL) {
        delete mTextureManager;
    }

    if (mGfxManager != NULL) {
        delete mGfxManager;
        mGfxManager = NULL;
        simple_renderer::Renderer::ShutdownInstance();
    }

    if (mDisplayInitialized) {
        // Temp destruct code until rest of BaseGameFramework refactoring goes in
        DisplayManager::GetInstance().ShutdownSwapchain(mSwapchainHandle);
        DisplayManager::GetInstance().ShutdownGraphicsAPI();
        DisplayManager::ShutdownInstance();
    }

    delete mVibrationHelper;
    delete mTuningManager;
    Paddleboat_setControllerStatusCallback(NULL, NULL);
    Paddleboat_destroy(GetJniEnv());
    delete mGameAssetManager;
    if (mJniEnv) {
        ALOGI("Detaching current thread from JNI.");
        mApp->activity->vm->DetachCurrentThread();
        ALOGI("Current thread detached from JNI.");
        mJniEnv = NULL;
    }
    _singleton = NULL;
    delete mDataStateMachine;
}

static void _handle_cmd_proxy(struct android_app *app, int32_t cmd) {
    NativeEngine *engine = (NativeEngine *) app->userData;
    engine->HandleCommand(cmd);
}

//static int _handle_input_proxy(struct android_app* app, AInputEvent* event) {
//    NativeEngine *engine = (NativeEngine*) app->userData;
//    return engine->HandleInput(event) ? 1 : 0;
//}

bool NativeEngine::IsAnimating() {
    return mHasFocus && mHasStarted && mHasSwapchain;
}

static bool _cooked_event_callback(struct CookedEvent *event) {
    SceneManager *mgr = SceneManager::GetInstance();
    PointerCoords coords;
    memset(&coords, 0, sizeof(coords));
    coords.x = event->motionX;
    coords.y = event->motionY;
    coords.minX = event->motionMinX;
    coords.maxX = event->motionMaxX;
    coords.minY = event->motionMinY;
    coords.maxY = event->motionMaxY;
    coords.isScreen = event->motionIsOnScreen;

    switch (event->type) {
        case COOKED_EVENT_TYPE_JOY:
            mgr->UpdateJoy(event->joyX, event->joyY);
            return true;
        case COOKED_EVENT_TYPE_POINTER_DOWN:
            mgr->OnPointerDown(event->motionPointerId, &coords);
            return true;
        case COOKED_EVENT_TYPE_POINTER_UP:
            mgr->OnPointerUp(event->motionPointerId, &coords);
            return true;
        case COOKED_EVENT_TYPE_POINTER_MOVE:
            mgr->OnPointerMove(event->motionPointerId, &coords);
            return true;
        case COOKED_EVENT_TYPE_KEY_DOWN:
            mgr->OnKeyDown(getOurKeyFromAndroidKey(event->keyCode));
            return true;
        case COOKED_EVENT_TYPE_KEY_UP:
            mgr->OnKeyUp(getOurKeyFromAndroidKey(event->keyCode));
            return true;
        case COOKED_EVENT_TYPE_BACK:
            return mgr->OnBackKeyPressed();
        case COOKED_EVENT_TYPE_TEXT_INPUT:
            mgr->OnTextInput();
            return true;
        default:
            return false;
    }
}

void NativeEngine::GameLoop() {
    mApp->userData = this;
    mApp->onAppCmd = _handle_cmd_proxy;

    while (1) {
        int events;
        struct android_poll_source *source;

        while ((ALooper_pollAll(0, NULL, &events, (void **) &source)) >= 0) {

            // process event
            if (source != NULL) {
                source->process(source->app, source);
            }
            // are we exiting?
            if (mApp->destroyRequested) {
                return;
            }
        }

        if (!mDisplayInitialized) {
            if (mApp->window == NULL || !AttemptDisplayInitialization()) {
			    usleep(1000);
				continue;
			}
            mHasStarted = true;
		} else {
            mMemoryConsumer->Update();
            mGameAssetManager->UpdateGameAssetManager();
            Paddleboat_update(GetJniEnv());
            HandleGameActivityInput();

            if (mApp->textInputState) {
                struct CookedEvent ev;
                ev.type = COOKED_EVENT_TYPE_TEXT_INPUT;
                ev.textInputState = true;
                _cooked_event_callback(&ev);
                mApp->textInputState = 0;
            }

            if (IsAnimating()) {
                DoFrame();
            }
        }
    }
}

// We may have to wait for the display to become available, return true once we are
// able to initialize the display and graphics API
bool NativeEngine::AttemptDisplayInitialization() {
    DisplayManager &display_manager = DisplayManager::GetInstance();

    const uint32_t vk_api_flags = display_manager.GetGraphicsAPISupportFlags(
        DisplayManager::kGraphicsAPI_Vulkan);
    // Early out if waiting for availability info
    if ((vk_api_flags & DisplayManager::kGraphics_API_Waiting) != 0) {
        return false;
    }

    DisplayManager::GraphicsAPI graphics_api = DisplayManager::kGraphicsAPI_Vulkan;
    uint32_t requested_features = DisplayManager::kVulkan_1_1_Support;
    mIsVulkan = true;

    if (vk_api_flags == DisplayManager::kGraphics_API_Unsupported ||
        ((vk_api_flags & DisplayManager::kVulkan_1_1_Support) == 0) || s_disable_vulkan) {
        // Fall back to GLES 3 if Vulkan isn't supported, or isn't at
        // least a Vulkan 1.1+ device
        graphics_api = DisplayManager::kGraphicsAPI_GLES;
        requested_features = DisplayManager::kGLES_3_0_Support;
        mIsVulkan = false;

        const uint32_t gles_api_flags = display_manager.GetGraphicsAPISupportFlags(
            DisplayManager::kGraphicsAPI_GLES);
        // Early out if waiting for availability info
        if ((gles_api_flags & DisplayManager::kGraphics_API_Waiting) != 0) {
            return false;
        }

        if (gles_api_flags == DisplayManager::kGraphics_API_Unsupported ||
            ((gles_api_flags & DisplayManager::kGLES_3_0_Support) == 0)) {
            ALOGE("Device does not support Vulkan or OpenGLES 3.0!");
            MY_ASSERT(false);
            return false;
        }
    }

    const DisplayManager::InitGraphicsAPIResult init_result = display_manager.InitGraphicsAPI(
        graphics_api, requested_features);
    if (init_result != DisplayManager::kInit_GraphicsAPI_Success) {
        ALOGE("Failed to initialize %s API!",
              (graphics_api == DisplayManager::kGraphicsAPI_Vulkan ? "Vulkan" : "GLES"));
        MY_ASSERT(false);
        return false;
    }

    return CreateSwapchain();
}

bool NativeEngine::CreateSwapchain() {
    DisplayManager &display_manager = DisplayManager::GetInstance();
    std::unique_ptr<DisplayManager::SwapchainConfigurations> swapchain_configurations =
        display_manager.GetSwapchainConfigurations(DisplayManager::kDefault_Display);

    const DisplayManager::DisplayColorSpace swapchain_color_space = mIsVulkan ?
        DisplayManager::kDisplay_Color_Space_SRGB : DisplayManager::kDisplay_Color_Space_Linear;
    DisplayManager::DisplayFormat display_format(swapchain_color_space,
                                                 DisplayManager::kDisplay_Depth_Format_D24S8_Packed,
                                                 DisplayManager::kDisplay_Pixel_Format_RGBA8,
                                                 DisplayManager::kDisplay_Stencil_Format_D24S8_Packed);

    bool found_display_format = false;
    for (auto iter = swapchain_configurations->display_formats.begin();
         iter != swapchain_configurations->display_formats.end(); ++iter) {
        if (*iter == display_format) {
            found_display_format = true;
            break;
        }
    }

    if (found_display_format) {
        mDisplayFormat = display_format;
        const DisplayManager::InitSwapchainResult swapchain_result = display_manager.InitSwapchain(
            display_format, swapchain_configurations->display_resolutions[0],
            swapchain_configurations->display_swap_intervals[0],
            swapchain_configurations->min_swapchain_frame_count,
            DisplayManager::kSwapchain_Present_Fifo,
            DisplayManager::kDefault_Display,
            &mSwapchainHandle);

        if (swapchain_result == DisplayManager::kInit_Swapchain_Success) {
            mSwapchainImageCount = swapchain_configurations->min_swapchain_frame_count;
            mSurfWidth = swapchain_configurations->display_resolutions[0].display_width;
            mSurfHeight = swapchain_configurations->display_resolutions[0].display_height;
            mScreenDensity = swapchain_configurations->display_resolutions[0].display_dpi;
            SceneManager *mgr = SceneManager::GetInstance();
            mgr->SetScreenSize(mSurfWidth, mSurfHeight);
            mDisplayInitialized = true;
            mHasSwapchain = true;
            ALOGI("Initialized swapchain");
            mSwapchainFrameHandle = display_manager.GetCurrentSwapchainFrame(mSwapchainHandle);
            display_manager.SetSwapchainChangedCallback(
                std::bind(&NativeEngine::SwapchainChanged, this,
                          std::placeholders::_1, std::placeholders::_2), nullptr);
            display_manager.SetDisplayChangedCallback(
                std::bind(&NativeEngine::DisplayResolutionChanged, this,
                          std::placeholders::_1, std::placeholders::_2),nullptr);
        } else {
            ALOGE("Failed to create swapchain");
            MY_ASSERT(false);
            return false;
        }
    } else {
        ALOGE("Failed to find compatible display format");
        MY_ASSERT(false);
        return false;
    }
    return true;
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

DataLoaderStateMachine *NativeEngine::BeginSavedGameLoad() {
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

bool NativeEngine::SaveProgress(int level, bool forceSave) {
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

void NativeEngine::SaveGameToCloud(int level) {
    MY_ASSERT(GetJniEnv() && IsCloudSaveEnabled());
    ALOGI("Scheduling task to save cloud data through JNI");
    jclass activityClass = GetJniEnv()->GetObjectClass(mApp->activity->javaGameActivity);
    jmethodID saveCloudCheckpointID =
            GetJniEnv()->GetMethodID(activityClass, "saveCloudCheckpoint", "(I)V");
    GetJniEnv()->CallVoidMethod(
            mApp->activity->javaGameActivity, saveCloudCheckpointID, (jint)level);
}

// TODO: rename the methods according to your package name
extern "C" jboolean Java_com_google_sample_agdktunnel_PGSManager_isLoadingWorkInProgress(
        JNIEnv */*env*/, jobject /*pgsManager*/) {
    NativeEngine *instance = NativeEngine::GetInstance();
    return (jboolean)!instance->GetDataStateMachine()->isLoadingDataCompleted();
}

extern "C" void Java_com_google_sample_agdktunnel_PGSManager_savedStateInitLoading(
        JNIEnv */*env*/, jobject /*pgsManager*/) {
    NativeEngine *instance = NativeEngine::GetInstance();
    instance->GetDataStateMachine()->init();
}

extern "C" void Java_com_google_sample_agdktunnel_PGSManager_authenticationCompleted(
        JNIEnv */*env*/, jobject /*pgsManager*/) {
    NativeEngine *instance = NativeEngine::GetInstance();
    instance->GetDataStateMachine()->authenticationCompleted();
}

extern "C" void Java_com_google_sample_agdktunnel_PGSManager_authenticationFailed(
        JNIEnv */*env*/, jobject /*pgsManager*/) {
    NativeEngine *instance = NativeEngine::GetInstance();
    instance->GetDataStateMachine()->authenticationFailed();
}

extern "C" void Java_com_google_sample_agdktunnel_PGSManager_savedStateSnapshotNotFound(
        JNIEnv */*env*/, jobject /*pgsManager*/) {
    NativeEngine *instance = NativeEngine::GetInstance();
    instance->GetDataStateMachine()->savedStateSnapshotNotFound();
}

extern "C" void Java_com_google_sample_agdktunnel_PGSManager_savedStateCloudDataFound(
        JNIEnv */*env*/, jobject /*pgsManagerl*/) {
    NativeEngine *instance = NativeEngine::GetInstance();
    instance->GetDataStateMachine()->savedStateCloudDataFound();
}

extern "C" void Java_com_google_sample_agdktunnel_PGSManager_savedStateLoadingFailed(
        JNIEnv */*env*/, jobject /*pgsManager*/) {
    NativeEngine *instance = NativeEngine::GetInstance();
    instance->GetDataStateMachine()->savedStateLoadingFailed();
}

extern "C" void Java_com_google_sample_agdktunnel_PGSManager_savedStateLoadingCompleted(
        JNIEnv */*env*/, jobject /*pgsManager*/, jint level) {
    NativeEngine *instance = NativeEngine::GetInstance();
    instance->GetDataStateMachine()->savedStateLoadingCompleted(level);
}

static char sInsetsTypeName[][32] = {
    "CAPTION_BAR",
    "DISPLAY_CUTOUT",
    "IME",
    "MANDATORY_SYSTEM_GESTURES",
    "NAVIGATION_BARS",
    "STATUS_BARS",
    "SYSTEM_BARS",
    "SYSTEM_GESTURES",
    "TAPABLE_ELEMENT",
    "WATERFALL",
};

void NativeEngine::HandleCommand(int32_t cmd) {
    SceneManager *mgr = SceneManager::GetInstance();

    VLOGD("NativeEngine: handling command %d.", cmd);
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.
            VLOGD("NativeEngine: APP_CMD_SAVE_STATE");
            mState.mHasFocus = mHasFocus;
            mApp->savedState = malloc(sizeof(mState));
            *((NativeEngineSavedState *) mApp->savedState) = mState;
            mApp->savedStateSize = sizeof(mState);
            break;
        case APP_CMD_INIT_WINDOW:
            // We have a window!
            VLOGD("NativeEngine: APP_CMD_INIT_WINDOW");
            if (mApp->window != NULL) {
                if (mApp->savedStateSize == sizeof(mState) && mApp->savedState != nullptr) {
                    mState = *((NativeEngineSavedState *) mApp->savedState);
                    mHasFocus = mState.mHasFocus;
                } else {
                    // Workaround APP_CMD_GAINED_FOCUS issue where the focus state is not
                    // passed down from GameActivity when restarting Activity
                    mHasFocus = appState.mHasFocus;
                }
                PlatformUtilAndroid::SetNativeWindow(mApp->window);
                DisplayManager::GetInstance().HandlePlatformDisplayChange(
                    DisplayManager::kDisplay_Change_Window_Init);
            }
            VLOGD("HandleCommand(%d): hasWindow = %d, hasFocus = %d", cmd,
                  (mApp->window != NULL) ? 1 : 0, mHasFocus ? 1 : 0);
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is going away -- kill the surface
            VLOGD("NativeEngine: APP_CMD_TERM_WINDOW");
            DisplayManager::GetInstance().HandlePlatformDisplayChange(
                DisplayManager::kDisplay_Change_Window_Terminate);
            PlatformUtilAndroid::SetNativeWindow(nullptr);
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
            mGameAssetManager->OnPause();
            mgr->OnPause();
            break;
        case APP_CMD_RESUME:
            VLOGD("NativeEngine: APP_CMD_RESUME");
            mGameAssetManager->OnResume();
            mgr->OnResume();
            break;
        case APP_CMD_STOP:
            VLOGD("NativeEngine: APP_CMD_STOP");
            Paddleboat_onStop(GetJniEnv());
            break;
        case APP_CMD_START:
            VLOGD("NativeEngine: APP_CMD_START");
            Paddleboat_onStart(GetJniEnv());
            break;
        case APP_CMD_WINDOW_RESIZED:
            DisplayManager::GetInstance().HandlePlatformDisplayChange(
                DisplayManager::kDisplay_Change_Window_Resized);
            break;
      case APP_CMD_CONFIG_CHANGED:
            VLOGD("NativeEngine: %s", cmd == APP_CMD_WINDOW_RESIZED ?
                                      "APP_CMD_WINDOW_RESIZED" : "APP_CMD_CONFIG_CHANGED");
            // Window was resized or some other configuration changed.
            // Note: we don't handle this event because we check the surface dimensions
            // every frame, so that's how we know it was resized. If you are NOT doing that,
            // then you need to handle this event!
            break;
        case APP_CMD_LOW_MEMORY:
            VLOGD("NativeEngine: APP_CMD_LOW_MEMORY");
            break;
        case APP_CMD_CONTENT_RECT_CHANGED:
            DisplayManager::GetInstance().HandlePlatformDisplayChange(
                DisplayManager::kDisplay_Change_Window_Content_Rect_Changed);
            VLOGD("NativeEngine: APP_CMD_CONTENT_RECT_CHANGED");
            break;
        case APP_CMD_WINDOW_REDRAW_NEEDED:
            DisplayManager::GetInstance().HandlePlatformDisplayChange(
                DisplayManager::kDisplay_Change_Window_Redraw_Needed);
            VLOGD("NativeEngine: APP_CMD_WINDOW_REDRAW_NEEDED");
            break;
        case APP_CMD_WINDOW_INSETS_CHANGED:
            DisplayManager::GetInstance().HandlePlatformDisplayChange(
                DisplayManager::kDisplay_Change_Window_Insets_Changed);
            VLOGD("NativeEngine: APP_CMD_WINDOW_INSETS_CHANGED");
            ARect insets;
            // Log all the insets types
            for (int type = 0; type < GAMECOMMON_INSETS_TYPE_COUNT; ++type) {
                GameActivity_getWindowInsets(mApp->activity, (GameCommonInsetsType)type, &insets);
                VLOGD("%s insets: left=%d right=%d top=%d bottom=%d",
                      sInsetsTypeName[type], insets.left, insets.right, insets.top, insets.bottom);
            }
            break;
        default:
            VLOGD("NativeEngine: (unknown command).");
            break;
    }
}

bool NativeEngine::HandleInput(AInputEvent */*event*/) {
    return false;
}

int32_t NativeEngine::GetActiveGameControllerIndex() {
    return mGameControllerIndex;
}

void NativeEngine::SetActiveGameControllerIndex(const int32_t controllerIndex) {
    mGameControllerIndex = controllerIndex;
}

void NativeEngine::HandleGameActivityInput() {
    CheckForNewAxis();
    // If we get any key or motion events that were handled by a game controller,
    // read controller data and cook it into an event
    bool cookGameControllerEvent = false;

    // Swap input buffers so we don't miss any events while processing inputBuffer.
    android_input_buffer* inputBuffer = android_app_swap_input_buffers(mApp);
    // Early exit if no events.
    if (inputBuffer == nullptr) return;

    if (inputBuffer->keyEventsCount != 0) {
        for (uint64_t i = 0; i < inputBuffer->keyEventsCount; ++i) {
            GameActivityKeyEvent* keyEvent = &inputBuffer->keyEvents[i];
            if (Paddleboat_processGameActivityKeyInputEvent(keyEvent,
                                                            sizeof(GameActivityKeyEvent))) {
                cookGameControllerEvent = true;
            } else {
                CookGameActivityKeyEvent(keyEvent, _cooked_event_callback);
            }
        }
        android_app_clear_key_events(inputBuffer);
    }
    if (inputBuffer->motionEventsCount != 0) {
        for (uint64_t i = 0; i < inputBuffer->motionEventsCount; ++i) {
            GameActivityMotionEvent* motionEvent = &inputBuffer->motionEvents[i];

            if (Paddleboat_processGameActivityMotionInputEvent(motionEvent,
                                                               sizeof(GameActivityMotionEvent))) {
                cookGameControllerEvent = true;
            } else {
                // Didn't belong to a game controller, process it ourselves if it is a touch event
                CookGameActivityMotionEvent(motionEvent,
                                            _cooked_event_callback);
            }
        }
        android_app_clear_motion_events(inputBuffer);
    }

    if (cookGameControllerEvent) {
        CookGameControllerEvent(mGameControllerIndex, _cooked_event_callback);
    }
}

void NativeEngine::CheckForNewAxis() {
    // Tell GameActivity about any new axis ids so it reports
    // their events
    const uint64_t activeAxisIds = Paddleboat_getActiveAxisMask();
    uint64_t newAxisIds = activeAxisIds ^mActiveAxisIds;
    if (newAxisIds != 0) {
        mActiveAxisIds = activeAxisIds;
        int32_t currentAxisId = 0;
        while (newAxisIds != 0) {
            if ((newAxisIds & 1) != 0) {
                ALOGI("Enable Axis: %d", currentAxisId);
                GameActivityPointerAxes_enableAxis(currentAxisId);
            }
            ++currentAxisId;
            newAxisIds >>= 1;
        }
    }
}

bool NativeEngine::PrepareToRender() {
    // Early out conditions
    if (mDisplayInitialized == false) {
        return mDisplayInitialized;
    }
    // Make sure our context is set for rendering using the 'swapchain'
    DisplayManager& display_manager = DisplayManager::GetInstance();
    if (!display_manager.GetSwapchainValid(mSwapchainHandle)) {
        return false;
    }

    if (mGfxManager == NULL) {
        InitializeGfxManager();
    }

    if (!mHasGfxObjects) {
        ALOGI("NativeEngine: creating OpenGL objects.");
        if (!InitGfxObjects()) {
            ALOGE("NativeEngine: unable to initialize OpenGL objects.");
            return false;
        }
    }

    // ready to render
    return mDisplayInitialized;
}

void NativeEngine::InitializeGfxManager() {
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

void NativeEngine::KillGfxObjects() {
    if (mHasGfxObjects) {
        SceneManager *mgr = SceneManager::GetInstance();
        mgr->KillGraphics();
        mHasGfxObjects = false;
    }
}

void NativeEngine::DoFrame() {
    // prepare to render (create context, surfaces, etc, if needed)
    if (!PrepareToRender()) {
        // not ready
        VLOGD("NativeEngine: preparation to render failed.");
        return;
    }

    simple_renderer::Renderer& renderer = simple_renderer::Renderer::GetInstance();
    renderer.BeginFrame(mSwapchainHandle);

    SceneManager *mgr = SceneManager::GetInstance();

    // if this is the first frame, install the welcome scene
    if (mIsFirstFrame) {
        mIsFirstFrame = false;
        mgr->RequestNewScene(new LoaderScene());
    }

    // render!
    mgr->DoFrame();

    renderer.EndFrame();

    // swap buffers
    DisplayManager& display_manager = DisplayManager::GetInstance();
    mSwapchainFrameHandle = display_manager.PresentCurrentSwapchainFrame(mSwapchainHandle);
}

android_app *NativeEngine::GetAndroidApp() {
    return mApp;
}

bool NativeEngine::InitGfxObjects() {
    if (!mHasGfxObjects) {
        SceneManager *mgr = SceneManager::GetInstance();
        mgr->StartGraphics();
        mHasGfxObjects = true;
    }
    return true;
}

void NativeEngine::SetInputSdkContext(int context) {
    jclass activityClass = GetJniEnv()->GetObjectClass(mApp->activity->javaGameActivity);
    jmethodID setInputContextID =
            GetJniEnv()->GetMethodID(activityClass, "setInputContext", "(I)V");
    GetJniEnv()->CallVoidMethod(
            mApp->activity->javaGameActivity, setInputContextID, (jint)context);
}

void NativeEngine::SwapchainChanged(const DisplayManager::SwapchainChangeMessage reason,
                                    void* user_data) {
    if (reason == DisplayManager::kSwapchain_Gained_Window) {
        mHasSwapchain = true;
    } else if (reason == DisplayManager::kSwapchain_Lost_Window) {
        mHasSwapchain = false;
    } else if (reason == DisplayManager::kSwapchain_Needs_Recreation) {
        mGfxManager->SwapchainRecreated();
    }
}

void NativeEngine::DisplayResolutionChanged(const DisplayManager::DisplayChangeInfo
                              &display_change_info, void *user_data) {
    if (display_change_info.change_message == DisplayManager::kDisplay_Change_Window_Resized) {
        const int32_t width = display_change_info.display_resolution.display_width;
        const int32_t height = display_change_info.display_resolution.display_height;
        if (width != mSurfWidth || height != mSurfHeight) {
            // notify scene manager that the surface has changed size
            ALOGI("NativeEngine: surface changed size %dx%d --> %dx%d", mSurfWidth, mSurfHeight,
                  width, height);
            mSurfWidth = width;
            mSurfHeight = height;
            mScreenDensity = display_change_info.display_resolution.display_dpi;
            SceneManager::GetInstance()->SetScreenSize(mSurfWidth, mSurfHeight);
            // TODO: Make sure changes are passed through to renderer
        }
    }
}