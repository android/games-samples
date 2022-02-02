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

#include "paddleboat/paddleboat.h"

#include "engine.hpp"
#include "util.hpp"

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

    // Whether a given controller index is active
    bool mActiveControllers[PADDLEBOAT_MAX_CONTROLLERS];

    // Transition start time
    float mTransitionStart;

    // Controller Panel base X position
    float mControllerPanelBaseX;

    // Controller Panel base Y position
    float mControllerPanelBaseY;

    // Controller Panel element scale factor
    float mControllerPanelScale;

    // Currently active controller index (for UI display)
    int32_t mCurrentControllerIndex;

    // Newly connected controller index (for autoswitching current UI tab)
    int32_t mMostRecentConnectedControllerIndex;

    // Active controller details category tab
    int32_t mActiveControllerPanelTab;

    // Previous timestamp of controller data
    uint64_t mPreviousControllerDataTimestamp;

    // Previous timestamp of mouse data
    uint64_t mPreviousMouseDataTimestamp;

    // Current mouse data timestamp delta
    uint64_t mMouseDataTimestampDelta;

    // Accelerometer data previous timestamp and delta
    uint64_t mPreviousAccelerometerTimestamp;
    uint32_t mAccelerometerTimestampDelta;

    // Gyroscope data previous timestamp and delta
    uint64_t mPreviousGyroscopeTimestamp;
    uint32_t mGyroscopeTimestampDelta;

    // Most recently reported accelerometer data
    float mAccelerometerData[MOTION_AXIS_COUNT];

    // Most recently reported gyroscope data
    float mGyroscopeData[MOTION_AXIS_COUNT];

    // Preferences display is active
    bool mPreferencesActive;

    // Don't trim to 0/0 if inside the controller 'deadzone'
    bool mDontTrimDeadzone;

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

    void RenderMouseData();

    void RenderControllerTabs();

    void RenderPanel(const int32_t controllerIndex);

    void RenderPanel_ControlsTab(const int32_t controllerIndex,
                                 const Paddleboat_Controller_Data &controllerData,
                                 const Paddleboat_Controller_Info &controllerInfo);

    void RenderPanel_ControlsTab_ArcadeStick(const int32_t controllerIndex,
                                             const Paddleboat_Controller_Data &controllerData,
                                             const Paddleboat_Controller_Info &controllerInfo);

    void RenderPanel_InfoTab(const int32_t controllerIndex,
                             const Paddleboat_Controller_Data &controllerData,
                             const Paddleboat_Controller_Info &controllerInfo);

    void RenderPanel_VibrationTab(const int32_t controllerIndex,
                                  const Paddleboat_Controller_Data &controllerData,
                                  const Paddleboat_Controller_Info &controllerInfo);

    void RenderPanel_MotionTab(const int32_t controllerIndex,
                               const Paddleboat_Controller_Data &controllerData,
                               const Paddleboat_Controller_Info &controllerInfo);

    void RenderPanel_LightsTab(const int32_t controllerIndex,
                               const Paddleboat_Controller_Data &controllerData,
                               const Paddleboat_Controller_Info &controllerInfo);

    void ConfigureButtonLayout(const uint32_t layout);

public:
    DemoScene();

    virtual ~DemoScene();

    void GameControllerStatusEvent(const int32_t controllerIndex,
                                   const Paddleboat_ControllerStatus status);

    void MotionDataEvent(const int32_t controllerIndex,
                         const Paddleboat_Motion_Data *motionData);

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
