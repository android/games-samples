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

#pragma once

#include "common.hpp"

class ImGuiManager;

struct NativeEngineSavedState {
    bool mHasFocus;
};

class NativeEngine {
public:
    // create an engine
    NativeEngine(struct android_app *app);

    ~NativeEngine();

    // runs application until it dies
    void GameLoop();

    // returns the JNI environment
    JNIEnv *GetJniEnv();

    // returns the Android app object
    android_app *GetAndroidApp();

    // returns the imgui manager instance
    ImGuiManager *GetImGuiManager() { return mImGuiManager; }

    // returns the (singleton) instance
    static NativeEngine *GetInstance();

    // This is the env for the app thread. It's different to the main thread.
    JNIEnv *GetAppJniEnv();

    // Return the current system bar offset
    int GetSystemBarOffset() { return mSystemBarOffset; }

private:
    // variables to track Android lifecycle:
    bool mHasFocus, mIsVisible, mHasWindow;

    // are our OpenGL objects (textures, etc) currently loaded?
    bool mHasGLObjects;

    // android API version (0 if not yet queried)
    int mApiVersion;

    // Screen density
    int mScreenDensity;

    // EGL stuff
    EGLDisplay mEglDisplay;
    EGLSurface mEglSurface;
    EGLContext mEglContext;
    EGLConfig mEglConfig;

    // known surface size
    int mSurfWidth, mSurfHeight;

    // system bar offset
    int mSystemBarOffset;

    // known active motion axis ids (bitfield)
    uint64_t mActiveAxisIds;

    // android_app structure
    struct android_app *mApp;

    // additional saved state
    struct NativeEngineSavedState mState;

    // JNI environment
    JNIEnv *mJniEnv;

    // JNI env for the app native glue thread
    JNIEnv *mAppJniEnv;

    // ImGui manager instance
    ImGuiManager *mImGuiManager;

    // is this the first frame we're drawing?
    bool mIsFirstFrame;

    // initialize the display
    bool InitDisplay();

    // initialize surface. Requires display to have been initialized first.
    bool InitSurface();

    // initialize context. Requires display to have been initialized first.
    bool InitContext();

    // kill context
    void KillContext();

    void KillSurface();

    void KillDisplay(); // also causes context and surface to get killed

    bool HandleEglError(EGLint error);

    bool InitGLObjects();

    void KillGLObjects();

    void ConfigureOpenGL();

    bool PrepareToRender();

    void DoFrame();

    bool IsAnimating();

    void HandleGameActivityInput();

    void CheckForNewAxis();

    void UpdateSystemBarOffset();
public:
    // these are public for simplicity because we have internal static callbacks
    void HandleCommand(int32_t cmd);

    bool HandleInput(AInputEvent *event);
};
