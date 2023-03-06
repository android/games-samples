/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "common.hpp"
#include "scene.hpp"
#include "scene_manager.hpp"

// Include the LLVM "write profile to disk" function. If we are not
// instrumented, we need to include a stubbed-in version as LLVM will not
// include the instrumentation version.
//
// If we were to wrap the call to __llvm_profile_write_file with 
// #ifdef PGO_INSTRUMENT, the optimized build's version of the function would
// not match the instrumented version, and the compiler would then treat it as
// if it had no profile information for it.
#ifdef PGO_INSTRUMENT
extern "C" int __llvm_profile_write_file(void);
#else
extern "C" int __llvm_profile_write_file(void) { return 0; }
#endif

static SceneManager _sceneManager;

SceneManager::SceneManager() {
    mCurScene = NULL;

    // start with non-bogus (though not accurate) values
    mScreenWidth = 320;
    mScreenHeight = 240;

    mSceneToInstall = NULL;

    mHasGraphics = false;
}

void SceneManager::RequestNewScene(Scene *newScene) {
    LOGD("SceneManager: requesting new scene %p", newScene);
    mSceneToInstall = newScene;
}

void SceneManager::InstallScene(Scene *newScene) {
    LOGD("SceneManager: installing scene %p.", newScene);

    // kill graphics, if we have them.
    bool hadGraphics = mHasGraphics;
    if (mHasGraphics) {
        KillGraphics();
    }

    // If we have an existing scene, uninstall it.
    if (mCurScene) {
        mCurScene->OnUninstall();
        delete mCurScene;
        mCurScene = NULL;
    }

    // install the new scene
    mCurScene = newScene;
    if (mCurScene) {
        mCurScene->OnInstall();
    }

    // if we had graphics before, start them again.
    if (hadGraphics) {
        StartGraphics();
    }

    // Write the PGO profile to disk whenever we change scenes. If there is
    // already a profile on disk, it will be updated.
    __llvm_profile_write_file();
}

Scene* SceneManager::GetScene() {
    return mCurScene;
}

void SceneManager::DoFrame() {
    if (mSceneToInstall) {
        InstallScene(mSceneToInstall);
        mSceneToInstall = NULL;
    }

    if (mHasGraphics && mCurScene) {
        mCurScene->DoFrame();
    }
}

void SceneManager::KillGraphics() {
    if (mHasGraphics) {
        LOGD("SceneManager: killing graphics.");
        mHasGraphics = false;
        if (mCurScene) {
            mCurScene->OnKillGraphics();
        }
    }
}

void SceneManager::StartGraphics() {
    if (!mHasGraphics) {
        LOGD("SceneManager: starting graphics.");
        mHasGraphics = true;
        if (mCurScene) {
            LOGD("SceneManager: calling mCurScene->OnStartGraphics.");
            mCurScene->OnStartGraphics();
        }
    }
}


void SceneManager::SetScreenSize(int width, int height) {
    if (mScreenWidth != width || mScreenHeight != height) {
        mScreenWidth = width;
        mScreenHeight = height;

        if (mCurScene && mHasGraphics) {
            mCurScene->OnScreenResized(width, height);
        }
    }
}

SceneManager* SceneManager::GetInstance() {
    return &_sceneManager;
}

void SceneManager::OnPointerDown(int pointerId, const struct PointerCoords *coords) {
    if (mHasGraphics && mCurScene) {
        mCurScene->OnPointerDown(pointerId, coords);
    }
}

void SceneManager::OnPointerUp(int pointerId, const struct PointerCoords *coords) {
    if (mHasGraphics && mCurScene) {
        mCurScene->OnPointerUp(pointerId, coords);
    }
}

void SceneManager::OnPointerMove(int pointerId, const struct PointerCoords *coords) {
    if (mHasGraphics && mCurScene) {
        mCurScene->OnPointerMove(pointerId, coords);
    }
}

bool SceneManager::OnBackKeyPressed() {
    if (mHasGraphics && mCurScene) {
        return mCurScene->OnBackKeyPressed();
    }
    return false;
}

void SceneManager::OnKeyDown(int ourKeycode) {
    MY_ASSERT(ourKeycode >= 0 && ourKeycode < OURKEY_COUNT);
    if (mHasGraphics && mCurScene) {
        mCurScene->OnKeyDown(ourKeycode);

        // if our "escape" key (normally corresponding to joystick button B or Y)
        // was pressed, handle it as a back key
        if (ourKeycode == OURKEY_ESCAPE) {
            mCurScene->OnBackKeyPressed();
        }
    }
}

void SceneManager::OnKeyUp(int ourKeycode) {
    MY_ASSERT(ourKeycode >= 0 && ourKeycode < OURKEY_COUNT);
    if (mHasGraphics && mCurScene) {
        mCurScene->OnKeyUp(ourKeycode);
    }
}

void SceneManager::UpdateJoy(float joyX, float joyY) {
    if (mHasGraphics && mCurScene) {
        mCurScene->OnJoy(joyX, joyY);
    }
}

void SceneManager::OnPause() {
    if (mHasGraphics && mCurScene) {
        mCurScene->OnPause();
    }
}

void SceneManager::OnResume() {
    if (mHasGraphics && mCurScene) {
        mCurScene->OnResume();
    }
}
