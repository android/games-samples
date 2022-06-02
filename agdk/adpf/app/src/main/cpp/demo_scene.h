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

#ifndef DEMO_SCENE_H_
#define DEMO_SCENE_H_

#include "engine.h"
#include "util.h"

class GameAssetManager;

/* Basic scene implentation for our demo UI display */
class DemoScene : public Scene {
private:
    // We want to register a touch down as the equivalent of
    // a button click to ImGui, so we send it an up event
    // without waiting for the touch to end
    enum SimulatedClickState {
        SIMULATED_CLICK_NONE = 0,
        SIMULATED_CLICK_DOWN,
        SIMULATED_CLICK_UP
    };

    static constexpr size_t MOTION_AXIS_COUNT = 3;

protected:
    // Did we simulate a click for ImGui?
    SimulatedClickState mSimulatedClickState;

    // Is a touch pointer (a.k.a. finger) down at the moment?
    bool mPointerDown;

    // Touch pointer current X
    float mPointerX;

    // Touch pointer current Y
    float mPointerY;

    // Transition start time
    float mTransitionStart;

    // Registered controller status callback
    bool mRegisteredStatusCallback;

    // must be implemented by subclass
    virtual void OnButtonClicked(int buttonId);

    // must be implemented by subclass
    virtual void RenderBackground();

    // Pass current input status to ImGui
    void UpdateUIInput();

    // UI rendering functions
    void RenderUI();

    void SetupUIWindow();

    bool RenderPreferences();

    void RenderPanel();

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

    virtual void OnInstall();

    virtual void OnUninstall();
};

#endif // DEMO_SCENE_H_
