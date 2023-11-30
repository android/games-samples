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

#ifndef agdktunnel_native_engine_hpp
#define agdktunnel_native_engine_hpp

#include "common.hpp"
#include "display_manager.h"
#include "system_event_manager.h"
#include "user_input_manager.h"

using namespace base_game_framework;

struct NativeEngineSavedState {
    bool mHasFocus;
};

// This is meant as a base class to be derived by the
// indivdual sample for sample-specific code
class NativeEngine {
public:
    NativeEngine(struct android_app *app);

    virtual ~NativeEngine();

    virtual void GameLoop();

    virtual bool CheckRenderPrerequisites() = 0;

    virtual void DoFirstFrameSetup() = 0;

    virtual void ScreenSizeChanged() = 0;

    // returns the JNI environment
    JNIEnv *GetJniEnv();

    // returns the Android app object
    android_app *GetAndroidApp();

    // returns the (singleton) instance
    static NativeEngine *GetInstance();

    // This is the env for the app thread. It's different to the main thread.
    JNIEnv *GetAppJniEnv();

protected:
    bool ProcessCookedEvent(struct CookedEvent *event);

    // variables to track Android lifecycle:
    // variables to track Android lifecycle:
    bool mHasFocus, mHasStarted, mDisplayInitialized;

    // has active swapchain
    bool mHasSwapchain;

    // True if we are to exit main loop and shutdown
    bool mQuitting;

    // known surface size
    int mSurfWidth, mSurfHeight;

    // Screen density
    int mScreenDensity;

    // android_app structure
    struct android_app *mApp;

    // additional saved state
    struct NativeEngineSavedState mState;

    // JNI environment
    JNIEnv *mJniEnv;

    // JNI env for the app native glue thread
    JNIEnv *mAppJniEnv;

    base_game_framework::DisplayManager::SwapchainFrameHandle mSwapchainFrameHandle;

    base_game_framework::DisplayManager::SwapchainHandle mSwapchainHandle;

    base_game_framework::DisplayManager::DisplayFormat mDisplayFormat;

    int mSwapchainImageCount;

    // Are we using Vulkan?
    bool mIsVulkan;

    // is this the first frame we're drawing?
    bool mIsFirstFrame;

    // Initial display and graphics API setup
    bool AttemptDisplayInitialization();

    bool CreateSwapchain();

    // BaseGameFramework callbacks

    // Display Manager
    void SwapchainChanged(const base_game_framework::DisplayManager::SwapchainChangeMessage reason,
                          void *user_data);

    void DisplayResolutionChanged(const base_game_framework::DisplayManager::DisplayChangeInfo
                                  &display_change_info, void *user_data);

    // System Event Manager
    void FocusEvent(const SystemEventManager::FocusEvent focus_event, void *user_data);

    void LifecycleEvent(const SystemEventManager::LifecycleEvent lifecycle_event,
                        void *user_data);

    void MemoryWarningEvent(const SystemEventManager::MemoryWarningEvent memory_event,
                            void *user_data);

    void ReadSaveStateEvent(const SystemEventManager::SaveState &save_state, void *user_data);

    // User Input Manager
    bool KeyEventCallback(const KeyEvent &key_event, void *user_data);

    bool TouchEventCallback(const TouchEvent &touch_event, void *user_data);

    bool PrepareToRender();

    void DoFrame();

    bool IsAnimating();
};

#endif
