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
#include "scene.hpp"
#include "scene_manager.hpp"

static SceneManager _sceneManager;

SceneManager::SceneManager() {
    mCurScene = NULL;

    // start with non-bogus (though not accurate) values
    mScreenWidth = 320;
    mScreenHeight = 240;
    mRotationMatrix = glm::mat4(1.0f);

    mSceneToInstall = NULL;

    mHasGraphics = false;

    for (int i = 0; i < OURKEY_COUNT; ++i) {
        mStickKeyDelivered[i] = false;
    }
}

void SceneManager::PrepareShutdown() {
    if (mSceneToInstall) {
        delete mSceneToInstall;
        mSceneToInstall = NULL;
    }
    InstallScene(NULL);
}

void SceneManager::RequestNewScene(Scene *newScene) {
    ALOGI("SceneManager: requesting new scene %p", newScene);
    if (mSceneToInstall != NULL && mSceneToInstall != newScene) {
        // The last request wins; don't leak the one it replaces
        ALOGW("SceneManager: replacing pending scene %p", mSceneToInstall);
        delete mSceneToInstall;
    }
    mSceneToInstall = newScene;
}

void SceneManager::InstallScene(Scene *newScene) {
    ALOGI("SceneManager: installing scene %p.", newScene);

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

    // install the new scene; it hasn't seen any stick key downs
    mCurScene = newScene;
    for (int i = 0; i < OURKEY_COUNT; ++i) {
        mStickKeyDelivered[i] = false;
    }
    if (mCurScene) {
        // Specify the controls to use to the Input SDK
        mCurScene->SetInputSdkContext();
        mCurScene->OnInstall();
    }

    // if we had graphics before, start them again.
    if (hadGraphics) {
        StartGraphics();
    }
}

Scene *SceneManager::GetScene() {
    return mCurScene;
}

void SceneManager::DoFrame() {
    if (mSceneToInstall) {
        // Clear the request first, so a scene requested while installing isn't lost (or,
        // see RequestNewScene, deleted as a replaced request)
        Scene *scene = mSceneToInstall;
        mSceneToInstall = NULL;
        InstallScene(scene);
    }

    if (mHasGraphics && mCurScene) {
        mCurScene->DoFrame();
    }
}

void SceneManager::KillGraphics() {
    if (mHasGraphics) {
        ALOGI("SceneManager: killing graphics.");
        mHasGraphics = false;
        if (mCurScene) {
            mCurScene->OnKillGraphics();
        }
    }
}

void SceneManager::StartGraphics() {
    if (!mHasGraphics) {
        ALOGI("SceneManager: starting graphics.");
        mHasGraphics = true;
        if (mCurScene) {
            ALOGI("SceneManager: calling mCurScene->OnStartGraphics.");
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

SceneManager *SceneManager::GetInstance() {
    return &_sceneManager;
}

void SceneManager::OnPointerDown(int pointerId, const struct PointerCoords *coords) {
    // Once a scene change is pending, the current scene gets no more input (it could
    // otherwise request another scene, e.g. on a repeated key)
    if (mHasGraphics && mCurScene && !mSceneToInstall) {
        mCurScene->OnPointerDown(pointerId, coords);
    }
}

void SceneManager::OnPointerUp(int pointerId, const struct PointerCoords *coords) {
    if (mHasGraphics && mCurScene && !mSceneToInstall) {
        mCurScene->OnPointerUp(pointerId, coords);
    }
}

void SceneManager::OnPointerMove(int pointerId, const struct PointerCoords *coords) {
    if (mHasGraphics && mCurScene && !mSceneToInstall) {
        mCurScene->OnPointerMove(pointerId, coords);
    }
}

bool SceneManager::OnBackKeyPressed() {
    if (mSceneToInstall) {
        // A scene change is pending: consume the press (don't leave the game)
        return true;
    }
    if (mHasGraphics && mCurScene) {
        return mCurScene->OnBackKeyPressed();
    }
    return false;
}

bool SceneManager::HandlesBack() {
    return mHasGraphics && mCurScene && mCurScene->HandlesBack();
}

void SceneManager::OnKeyDown(int ourKeyCode, bool fromStick) {
    if ((ourKeyCode >= 0 && ourKeyCode < OURKEY_COUNT &&
            mHasGraphics && mCurScene && !mSceneToInstall)) {
        if (fromStick) {
            if (!mCurScene->WantsStickDirectionKeys()) {
                return;
            }
            mStickKeyDelivered[ourKeyCode] = true;
        }
        mCurScene->OnKeyDown(ourKeyCode);

        // if our "escape" key (normally corresponding to joystick button B or Y)
        // was pressed, handle it as a back key
        if (ourKeyCode == OURKEY_ESCAPE) {
            mCurScene->OnBackKeyPressed();
        }
    }
}

void SceneManager::OnKeyUp(int ourKeyCode, bool fromStick) {
    if ((ourKeyCode >= 0 && ourKeyCode < OURKEY_COUNT &&
            mHasGraphics && mCurScene)) {
        if (fromStick) {
            if (!mStickKeyDelivered[ourKeyCode]) {
                return;
            }
            mStickKeyDelivered[ourKeyCode] = false;
        }
        mCurScene->OnKeyUp(ourKeyCode);
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
    if (mCurScene) {
        mCurScene->OnResume();
    }
}

void SceneManager::OnTextInput() {
    if (mHasGraphics && mCurScene) {
        mCurScene->OnTextInput();
    }
}
