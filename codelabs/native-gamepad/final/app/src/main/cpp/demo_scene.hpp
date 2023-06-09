/*
 * Copyright 2020 Google LLC
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

#ifndef nativegamepad_demo_scene_hpp
#define nativegamepad_demo_scene_hpp

#include "engine.hpp"
#include "util.hpp"

class GameAssetManager;

/* Basic scene implentation for our demo UI display */
class DemoScene : public Scene {
protected:
    // is a touch pointer (a.k.a. finger) down at the moment?
    bool mPointerDown;

    // if true, shows a "please wait" screen instead of the interface
    bool mWaitScreen;

    // if true, shows an "error" screen instead of the interface
    bool mErrorScreen;

    // have textures been loaded for an asset pack
    bool mInstallTimeTexturesLoaded;
    bool mFastFollowTexturesLoaded;
    bool mOnDemandTexturesLoaded;

    // touch pointer current X
    float mPointerX;

    // touch pointer current Y
    float mPointerY;

    // current image texture reference
    uint64_t mCurrentTextureReference;

    // must be implemented by subclass
    virtual void OnButtonClicked(int buttonId);

    virtual void RenderBackground();

    // transition start time
    float mTransitionStart;

    // Determine if any must-have data is still pending
    bool DetermineWaitStatus();

    // Display the status of a downloading asset pack
    void DisplayAssetPackDownloadStatus(GameAssetManager *gameAssetManager,
                                        const char *assetPackName);

    // Display the status of a needs-download asset pack
    void DisplayAssetPackNeedsDownloadStatus(GameAssetManager *gameAssetManager,
                                             const char *assetPackName);

    // Display the status of a needs-mobile-authorization asset pack
    void DisplayAssetPackNeedsMobileAuthStatus(GameAssetManager *gameAssetManager,
                                               const char *assetPackName);

    // Display the status of a ready asset pack
    void DisplayAssetPackReadyStatus(GameAssetManager *gameAssetManager,
                                     const char *assetPackName, bool *texturesLoaded);

    // Display the UI for an asset pack
    void DisplayAssetPackUI(const char *assetPackName, bool *texturesLoaded);

    // Display the UI if an asset availability error occurred
    void DisplayErrorUI();

    // Display the main UI
    void DisplayMainUI();

    // Display the common UI 'header'
    void DisplayUIHeader();

    // Display the UI for waiting on must-have data to become available
    void DisplayWaitUI();

    void SetErrorScreen(bool b) {
        mErrorScreen = b;
        if (mErrorScreen) {
            mTransitionStart = Clock();
        }
    }

    void SetWaitScreen(bool b) {
        mWaitScreen = b;
        if (mWaitScreen) {
            mTransitionStart = Clock();
        }
    }

    // Pass current input status to ImGui
    void UpdateUIInput();


public:
    DemoScene();

    virtual ~DemoScene();


    virtual void OnStartGraphics();

    virtual void OnKillGraphics();

    virtual void DoFrame();

    virtual void OnPointerDown(int pointerId, const struct PointerCoords *coords);

    virtual void OnPointerMove(int pointerId, const struct PointerCoords *coords);

    virtual void OnPointerUp(int pointerId, const struct PointerCoords *coords);

    virtual void OnScreenResized(int width, int height);
};

#endif

