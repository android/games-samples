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
#include "scene_manager.hpp"
#include "loader_scene.hpp"
#include "native_engine.hpp"
#include "welcome_scene.hpp"

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

// Set to true to force GLES always
static bool s_disable_vulkan = false;

// workaround for internal bug b/149866792
static NativeEngineSavedState appState = {false};

NativeEngine::NativeEngine(struct android_app *app) {
    ALOGI("NativeEngine: initializing.");
    mApp = app;
    mHasFocus = mHasStarted = mDisplayInitialized = false;
    mHasSwapchain = false;
    mQuitting = false;
    mSurfWidth = mSurfHeight = 0;
    mScreenDensity = 0;
    mJniEnv = NULL;
    mAppJniEnv = NULL;
    memset(&mState, 0, sizeof(mState));
    mSwapchainFrameHandle = DisplayManager::kInvalid_swapchain_handle;
    mSwapchainHandle = DisplayManager::kInvalid_swapchain_handle;
    mIsVulkan = false;
    mIsFirstFrame = true;

    SystemEventManager& event_manager = SystemEventManager::GetInstance();
    event_manager.SetFocusEventCallback(std::bind(&NativeEngine::FocusEvent,
                                                  this, std::placeholders::_1,
                                                  std::placeholders::_2),nullptr);
    event_manager.SetLifecycleEventCallback(std::bind(&NativeEngine::LifecycleEvent,
                                                      this, std::placeholders::_1,
                                                      std::placeholders::_2), nullptr);
    event_manager.SetMemoryWarningEventCallback(std::bind(&NativeEngine::MemoryWarningEvent,
                                                          this, std::placeholders::_1,
                                                          std::placeholders::_2),nullptr);
    event_manager.SetReadSaveStateCallback(std::bind(&NativeEngine::ReadSaveStateEvent,
                                                     this, std::placeholders::_1,
                                                     std::placeholders::_2),nullptr);

    UserInputManager& input_manager = UserInputManager::GetInstance();
    input_manager.SetKeyEventCallback(std::bind(&NativeEngine::KeyEventCallback,
                                                this, std::placeholders::_1,
                                                std::placeholders::_2),nullptr);
    input_manager.SetTouchEventCallback(std::bind(&NativeEngine::TouchEventCallback,
                                                  this, std::placeholders::_1,
                                                  std::placeholders::_2),nullptr);

    if (app->savedState != NULL) {
        // we are starting with previously saved state -- restore it
        mState = *(struct NativeEngineSavedState *) app->savedState;
    }
}

NativeEngine::~NativeEngine() {
    VLOGD("NativeEngine: destructor running");

    if (mDisplayInitialized) {
        // Temp destruct code until rest of BaseGameFramework refactoring goes in
        DisplayManager::GetInstance().ShutdownSwapchain(mSwapchainHandle);
        DisplayManager::GetInstance().ShutdownGraphicsAPI();
        DisplayManager::ShutdownInstance();
    }
}

bool NativeEngine::IsAnimating() {
    return mHasFocus && mHasStarted && mHasSwapchain;
}

void NativeEngine::GameLoop() {
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
    uint32_t requested_features = DisplayManager::kVulkan_1_1_Support |
        DisplayManager::kVulkan_ETC2_Support;
    mIsVulkan = true;

    if (vk_api_flags == DisplayManager::kGraphics_API_Unsupported ||
        ((vk_api_flags & DisplayManager::kVulkan_1_1_Support) == 0) ||
        ((vk_api_flags & DisplayManager::kVulkan_ETC2_Support) == 0) || s_disable_vulkan) {
        // Fall back to GLES 3 if Vulkan isn't supported, or isn't at
        // least a Vulkan 1.1+ device that supports ETC2
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

    if (!CheckRenderPrerequisites()) {
        return false;
    }

    // Update our display rotation matrix, used for pre-rotation when running under Vulkan
    // We swap 90/270 rotations because we are flipping Y to pretend Vulkan is
    // actually GL style Y axis points up instead of down
    const DisplayManager::SwapchainRotationMode rotation =
        display_manager.GetSwapchainRotationMode(mSwapchainHandle);
    glm::mat4 rotationMatrix = glm::mat4(1.0f);
    glm::vec3 rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);

    if (rotation == DisplayManager::kSwapchain_Rotation_90) {
      rotationMatrix = glm::rotate(rotationMatrix, glm::radians(270.0f), rotationAxis);
    } else if (rotation == DisplayManager::kSwapchain_Rotation_180) {
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(180.0f), rotationAxis);
    } else if (rotation == DisplayManager::kSwapchain_Rotation_270) {
      rotationMatrix = glm::rotate(rotationMatrix, glm::radians(90.0f), rotationAxis);
    }
    SceneManager::GetInstance()->SetRotationMatrix(rotationMatrix);

    // ready to render
    return mDisplayInitialized;
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
        DoFirstFrameSetup();
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

bool NativeEngine::ProcessCookedEvent(struct CookedEvent *event) {
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

// BaseGameFramework callbacks
void NativeEngine::SwapchainChanged(const DisplayManager::SwapchainChangeMessage reason,
                                    void* user_data) {
    if (reason == DisplayManager::kSwapchain_Gained_Window) {
        mHasSwapchain = true;
    } else if (reason == DisplayManager::kSwapchain_Lost_Window) {
        mHasSwapchain = false;
    } else if (reason == DisplayManager::kSwapchain_Needs_Recreation) {
        simple_renderer::Renderer::GetInstance().SwapchainRecreated();
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
            ScreenSizeChanged();
        }
    }
}

void NativeEngine::FocusEvent(const SystemEventManager::FocusEvent focus_event, void *user_data) {
    switch (focus_event) {
        case SystemEventManager::kSentToBackground:
            mHasFocus = false;
        mState.mHasFocus = appState.mHasFocus = mHasFocus;
        break;
        case SystemEventManager::kMadeForeground:
            mHasFocus = true;
        mState.mHasFocus = appState.mHasFocus = mHasFocus;
        break;
    }
}

void NativeEngine::LifecycleEvent(const SystemEventManager::LifecycleEvent lifecycle_event,
                                  void *user_data) {
    switch (lifecycle_event) {
        case SystemEventManager::kLifecycleStart:
        break;
        case SystemEventManager::kLifecycleResume:
            SceneManager::GetInstance()->OnResume();
        break;
        case SystemEventManager::kLifecyclePause:
            SceneManager::GetInstance()->OnPause();
        break;
        case SystemEventManager::kLifecycleStop:
            mHasStarted = false;
        break;
        case SystemEventManager::kLifecycleQuit:
            mQuitting = true;
        break;
        case SystemEventManager::kLifecycleSaveState:
        {
            SystemEventManager::SaveState save_state {reinterpret_cast<void*>(&mState),
                                                      sizeof(mState)};
            SystemEventManager::GetInstance().WriteSaveState(save_state);
        }
        break;
    }
}

void NativeEngine::MemoryWarningEvent(const SystemEventManager::MemoryWarningEvent memory_event,
                                      void *user_data) {

}

void NativeEngine::ReadSaveStateEvent(const SystemEventManager::SaveState& save_state,
                                      void *user_data) {
    const NativeEngineSavedState* saved_state =
        reinterpret_cast<const NativeEngineSavedState*>(save_state.state_data);
    mHasFocus = saved_state->mHasFocus;
    mState.mHasFocus = appState.mHasFocus = mHasFocus;
}

bool NativeEngine::KeyEventCallback(const KeyEvent &key_event, void *user_data) {
    return false;
}

bool NativeEngine::TouchEventCallback(const TouchEvent &touch_event, void *user_data) {
    struct CookedEvent cooked_event;
    switch (touch_event.touch_action) {
        case kTouch_Down:
            cooked_event.type = COOKED_EVENT_TYPE_POINTER_DOWN;
        break;
        case kTouch_Up:
            cooked_event.type = COOKED_EVENT_TYPE_POINTER_UP;
        break;
        case kTouch_Moved:
            cooked_event.type = COOKED_EVENT_TYPE_POINTER_MOVE;
        break;
    }
    cooked_event.motionPointerId = touch_event.touch_id;
    cooked_event.motionIsOnScreen = true;
    cooked_event.motionX = touch_event.touch_x;
    cooked_event.motionY = touch_event.touch_y;
    cooked_event.motionMinX = 0.0f;
    cooked_event.motionMaxX = SceneManager::GetInstance()->GetScreenWidth();
    cooked_event.motionMinY = 0.0f;
    cooked_event.motionMaxY = SceneManager::GetInstance()->GetScreenHeight();
    ProcessCookedEvent(&cooked_event);
    return true;
}