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

#include "controllerui_data.h"
#include "controllerui_util.h"
#include "demo_scene.hpp"
#include "imgui.h"
#include "imgui_manager.hpp"
#include "native_engine.hpp"

#include "Log.h"

#include <functional>

extern "C" {
#include <GLES2/gl2.h>
}

#define ARRAY_COUNTOF(array) (sizeof(array) / sizeof(array[0]))

namespace {

    const char *controllerTabNames[PADDLEBOAT_MAX_CONTROLLERS] = {" #1 ", " #2 ", " #3 ", " #4 ",
                                                                  " #5 ", " #6 ", " #7 ", " #8 "};
    const char *integratedTabName = " #I ";
    typedef void(DemoScene::*ControllerCategoryRenderFunction)(const int32_t,
            const Paddleboat_Controller_Data&, const Paddleboat_Controller_Info&);

    typedef struct ControllerCategoryTab {
        int32_t mTabIndex;
        const char *mTabTitle;
        ControllerCategoryRenderFunction mRenderFunction;
    } ControllerCategoryTab;

    const ImVec4 TEXTCOLOR_WHITE = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    const ImVec4 TEXTCOLOR_GREY = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
    const ImVec4 TEXTCOLOR_RED = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
    const ImVec4 TEXTCOLOR_GREEN = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);

    const float UI_SCALE_MIN = 1.0f;
    const float UI_SCALE_MAX = 9.0f;
    const float UI_SCALE_STEP = 0.5f;
    const float FONT_SCALE_MIN = 1.0f;
    const float FONT_SCALE_MAX = 9.0f;
    const float FONT_SCALE_STEP = 1.0f;

    const float VIBRATION_DURATION_MIN = 0.0;
    const float VIBRATION_DURATION_MAX = 2000.0;
    const float VIBRATION_DURATION_STEP = 100.0;
    const float VIBRATION_INTENSITY_MIN = 0.0;
    const float VIBRATION_INTENSITY_MAX = 1.0f;
    const float VIBRATION_INTENSITY_STEP = 0.05f;

    void GameControllerCallback(const int32_t controllerIndex,
                                const Paddleboat_ControllerStatus status, void *userData) {
        if (userData != nullptr) {
            DemoScene *scene = reinterpret_cast<DemoScene*>(userData);
            scene->GameControllerStatusEvent(controllerIndex, status);
        }
    }

    void MotionDataCallback(const int32_t controllerIndex,
                            const Paddleboat_Motion_Data *motionData, void *userData) {
        if (userData != nullptr) {
            DemoScene *scene = reinterpret_cast<DemoScene *>(userData);
            scene->MotionDataEvent(controllerIndex, motionData);
        }
    }

    void KeyboardCallback(bool keyboardConnected, void *userData) {
        if (userData != nullptr) {
            DemoScene *scene = reinterpret_cast<DemoScene *>(userData);
            scene->KeyboardStatusEvent(keyboardConnected);
        }

    }

    void UpdateMotionData(const Paddleboat_Motion_Data *motionData, uint64_t *previousTimestamp,
                          uint32_t *timestampDelta, float *destMotionData) {
        if (*previousTimestamp > 0) {
            uint64_t deltaTime = motionData->timestamp - (*previousTimestamp);
            // nanoseconds to milliseconds
            *timestampDelta = static_cast<uint32_t>(deltaTime / 1000000);
        }
        destMotionData[0] = motionData->motionX;
        destMotionData[1] = motionData->motionY;
        destMotionData[2] = motionData->motionZ;
        *previousTimestamp = motionData->timestamp;
    }

    void VibrationParameters(const char *labelText, const char *labelTag, const float vMin,
                             const float vMax, const float vStep, float *vValue) {
        char plusString[16];
        char minusString[16];
        snprintf(plusString, 16, " + ##%s", labelTag);
        snprintf(minusString, 16, " - ##%s", labelTag);
        ImGui::Spacing();
        ImGui::Text("%s", labelText);
        ImGui::SameLine();
        if (ImGui::Button(plusString)) {
            *vValue += vStep;
            if (*vValue > vMax) {
                *vValue = vMax;
            }
        }
        ImGui::SameLine();
        if (vMax > 1.0f) {
            ImGui::Text("%6.1f", *vValue);
        } else {
            ImGui::Text("   %1.1f", *vValue);
        }
        ImGui::SameLine();
        if (ImGui::Button(minusString)) {
            *vValue -= vStep;
            if (*vValue < vMin) {
                *vValue = vMin;
            }
        }
    }
}

DemoScene::DemoScene() {
    mSimulatedClickState = SIMULATED_CLICK_NONE;
    mPointerDown = false;
    mPointerX = 0.0f;
    mPointerY = 0.0f;
    mTransitionStart = 0.0f;
    for (int i = 0; i < PADDLEBOAT_MAX_CONTROLLERS; ++i) {
        mActiveControllers[i] = false;
    }
    mControllerPanelBaseX = 0.0f;
    mControllerPanelBaseY = 0.0f;
    mControllerPanelScale = 2.0f;
    mCurrentControllerIndex = 0;
    mMostRecentConnectedControllerIndex = -1;
    mActiveControllerPanelTab = 0;
    mPreviousControllerDataTimestamp = 0;
    mPreviousMouseDataTimestamp = 0;
    mIntegratedSensorFlags = PADDLEBOAT_INTEGRATED_SENSOR_NONE;
    mPreviousAccelerometerTimestamp = 0;
    mAccelerometerTimestampDelta = 0;
    mPreviousGyroscopeTimestamp = 0;
    mGyroscopeTimestampDelta = 0;
    mPreviousIntegratedAccelerometerTimestamp = 0;
    mIntegratedAccelerometerTimestampDelta = 0;
    mPreviousIntegratedGyroscopeTimestamp = 0;
    mIntegratedGyroscopeTimestampDelta = 0;
    for (size_t i = 0; i < DemoScene::MOTION_AXIS_COUNT; ++i) {
        mAccelerometerData[i] = 0.0f;
        mGyroscopeData[i] = 0.0f;
        mIntegratedAccelerometerData[i] = 0.0f;
        mIntegratedGyroscopeData[i] = 0.0f;
    }
    mDontTrimDeadzone = false;
    mPreferencesActive = false;
    mRegisteredStatusCallback = false;
    mKeyboardConnected = false;
}

DemoScene::~DemoScene() {
}

void DemoScene::OnStartGraphics() {
    mTransitionStart = Clock();
}

void DemoScene::OnKillGraphics() {
}

void DemoScene::OnScreenResized(int /*width*/, int /*height*/) {
}

void DemoScene::DoFrame() {
    // Make sure the controller status callback is registered before calling Paddleboat_update
    if (!mRegisteredStatusCallback) {
        return;
    }
    Paddleboat_update(NativeEngine::GetInstance()->GetJniEnv());

    // clear screen
    glClearColor(0.0f, 0.0f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    // Update UI inputs to ImGui before beginning a new frame
    UpdateUIInput();
    ImGuiManager *imguiManager = NativeEngine::GetInstance()->GetImGuiManager();
    imguiManager->BeginImGuiFrame();
    RenderUI();
    imguiManager->EndImGuiFrame();

    glEnable(GL_DEPTH_TEST);
}

void DemoScene::RenderBackground() {
    // base classes override this to draw background
}

void DemoScene::OnButtonClicked(int /*buttonId*/) {
    // base classes override this to react to button clicks
}

void DemoScene::OnPointerDown(int /*pointerId*/, const struct PointerCoords *coords) {
    // If this event was generated by something that's not associated to the screen,
    // (like a trackpad), ignore it, because our UI is not driven that way.
    if (coords->isScreen) {
        mPointerDown = true;
        mPointerX = coords->x;
        mPointerY = coords->y;
    }
}

void DemoScene::OnPointerMove(int /*pointerId*/, const struct PointerCoords *coords) {
    if (coords->isScreen && mPointerDown) {
        mPointerX = coords->x;
        mPointerY = coords->y;
    }
}

void DemoScene::OnPointerUp(int /*pointerId*/, const struct PointerCoords *coords) {
    if (coords->isScreen) {
        mPointerX = coords->x;
        mPointerY = coords->y;
        mPointerDown = false;
        mSimulatedClickState = SIMULATED_CLICK_NONE;
    }
}

void DemoScene::UpdateUIInput() {
    ImGuiIO &io = ImGui::GetIO();
    io.MousePos = ImVec2(mPointerX, mPointerY);
    bool pointerDown = false;
    // To make a touch work like a mouse click we need to sequence the following:
    // 1) Position cursor at touch spot with mouse button still up
    // 2) Signal mouse button down for a frame
    // 3) Release mouse button (even if touch is still held down)
    // 4) Reset to allow another 'click' once the touch is released
    if (mSimulatedClickState == SIMULATED_CLICK_NONE && mPointerDown) {
        mSimulatedClickState = SIMULATED_CLICK_DOWN;
    } else if (mSimulatedClickState == SIMULATED_CLICK_DOWN) {
        pointerDown = true;
        mSimulatedClickState = SIMULATED_CLICK_UP;
    }
    io.MouseDown[0] = pointerDown;
}


void DemoScene::RenderUI() {
    SetupUIWindow();

    if (!RenderPreferences()) {
        // Display keyboard/mouse data on the same line as the preferences button
        ImGui::SameLine(0.0f, 48.0f);
        RenderKeyboardData();
        RenderMouseData();
        RenderControllerTabs();
    }

    // --
    ImGui::End();
    ImGui::PopStyleVar();
}

void DemoScene::SetupUIWindow() {
    ImGuiIO &io = ImGui::GetIO();
    const float windowStartY = NativeEngine::GetInstance()->GetSystemBarOffset();
    ImVec2 windowPosition(0.0f, windowStartY);
    ImVec2 minWindowSize(io.DisplaySize.x * 0.95f, io.DisplaySize.y);
    ImVec2 maxWindowSize = io.DisplaySize;
    ImGui::SetNextWindowPos(windowPosition);
    ImGui::SetNextWindowSizeConstraints(minWindowSize, maxWindowSize, NULL, NULL);
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 32.0f);
    char titleString[64];
    snprintf(titleString, 64, "Game Controller Library Sample");
    ImGui::Begin(titleString, NULL, windowFlags);
}

bool DemoScene::RenderPreferences() {
    if (!mPreferencesActive) {
        if (ImGui::Button("Preferences...")) {
            mPreferencesActive = true;
        }
    }

    if (mPreferencesActive) {
        ImGuiManager *imguiManager = NativeEngine::GetInstance()->GetImGuiManager();
        float fontScale = imguiManager->GetFontScale();
        float uiScale = mControllerPanelScale;

        ImGui::Text("Preferences");

        ImGui::Spacing();
        ImGui::Text("Font scale:  ");
        ImGui::SameLine();
        const float alignLeftX = ImGui::GetCursorPosX();
        if (ImGui::Button(" + ##font")) {
            fontScale += FONT_SCALE_STEP;
            if (fontScale > FONT_SCALE_MAX) {
                fontScale = FONT_SCALE_MAX;
            }
            imguiManager->SetFontScale(fontScale);
        }
        ImGui::SameLine();
        ImGui::Text("%2.1f", fontScale);
        ImGui::SameLine();
        if (ImGui::Button(" - ##font")) {
            fontScale -= FONT_SCALE_STEP;
            if (fontScale < FONT_SCALE_MIN) {
                fontScale = FONT_SCALE_MIN;
            }
            imguiManager->SetFontScale(fontScale);
        }

        ImGui::Spacing();
        ImGui::Text("UI scale:  ");
        ImGui::SameLine();
        ImGui::SetCursorPosX(alignLeftX);
        if (ImGui::Button(" + ##ui")) {
            uiScale += UI_SCALE_STEP;
            if (uiScale > UI_SCALE_MAX) {
                uiScale = UI_SCALE_MAX;
            }
            mControllerPanelScale = uiScale;
        }
        ImGui::SameLine();
        ImGui::Text("%2.1f", uiScale);
        ImGui::SameLine();
        if (ImGui::Button(" - ##ui")) {
            uiScale -= UI_SCALE_STEP;
            if (uiScale < UI_SCALE_MIN) {
                uiScale = UI_SCALE_MIN;
            }
            mControllerPanelScale = uiScale;
        }

        ImGui::Spacing();
        ImGui::Checkbox("Raw deadzone", &mDontTrimDeadzone);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 16.0f);
        if (ImGui::Button("    OK    ")) {
            mPreferencesActive = false;
        }
    }
    return mPreferencesActive;
}

void DemoScene::RenderControllerTabs() {
    if (ImGui::BeginTabBar("ControllerTabBar", ImGuiTabBarFlags_NoTooltip)) {
        for (size_t index = 0; index < PADDLEBOAT_MAX_CONTROLLERS; ++index) {
            ImGuiTabItemFlags tabItemFlags = ImGuiTabItemFlags_None;
            if (mMostRecentConnectedControllerIndex >= 0) {
                // Switch active tab to newly connected controller
                if (index == static_cast<size_t>(mMostRecentConnectedControllerIndex)) {
                    tabItemFlags |= ImGuiTabItemFlags_SetSelected;
                    mMostRecentConnectedControllerIndex = -1;
                }
            }
            const ImVec4 tabTextColor =
                mActiveControllers[index] ? TEXTCOLOR_WHITE : TEXTCOLOR_GREY;

            ImGui::PushStyleColor(ImGuiCol_Text, tabTextColor);
            if (ImGui::BeginTabItem(controllerTabNames[index], NULL, tabItemFlags)) {
                mCurrentControllerIndex = index;
                ImGui::PopStyleColor(1);
                if (mActiveControllers[index] && mCurrentControllerIndex >= 0 &&
                    mCurrentControllerIndex < PADDLEBOAT_MAX_CONTROLLERS) {
                    RenderPanel(mCurrentControllerIndex);
                } else {
                    ImGui::Text("Not connected");
                }
                ImGui::EndTabItem();
            } else {
                ImGui::PopStyleColor(1);
            }
        }

        // Add an integrated device stats tab item
        RenderIntegratedTab();
        ImGui::EndTabBar();
    }
}

void DemoScene::RenderIntegratedTab() {
    if (ImGui::BeginTabItem(integratedTabName, NULL, ImGuiTabItemFlags_None)) {
        mCurrentControllerIndex = PADDLEBOAT_MAX_CONTROLLERS;
        RenderIntegratedPanel();
        ImGui::EndTabItem();
    }
}

void DemoScene::RenderKeyboardData() {
    if (mKeyboardConnected) {
        ImGui::Text("K:Y ");
    } else {
        ImGui::Text("K:N ");
    }
    ImGui::SameLine();
}

void DemoScene::RenderMouseData() {
    const Paddleboat_MouseStatus mouseStatus = Paddleboat_getMouseStatus();
    if (mouseStatus == PADDLEBOAT_MOUSE_NONE) {
        ImGui::Text("M:No");
        return;
    } else if (mouseStatus == PADDLEBOAT_MOUSE_CONTROLLER_EMULATED) {
        ImGui::Text("M:Vi ");
    } else if (mouseStatus == PADDLEBOAT_MOUSE_PHYSICAL) {
        ImGui::Text("M:Ph ");
    }
    ImGui::SameLine();
    Paddleboat_Mouse_Data mouseData;
    if (Paddleboat_getMouseData(&mouseData) == PADDLEBOAT_NO_ERROR) {
        const int mouseX = static_cast<int>(mouseData.mouseX);
        const int mouseY = static_cast<int>(mouseData.mouseY);
        ImGui::Text("X: %d Y: %d Btns: ", mouseX, mouseY);

        ImVec4 buttonColor = ((mouseData.buttonsDown & PADDLEBOAT_MOUSE_BUTTON_LEFT) != 0) ?
                TEXTCOLOR_GREEN : TEXTCOLOR_WHITE;
        ImGui::SameLine();
        ImGui::TextColored(buttonColor, "L");

        buttonColor = ((mouseData.buttonsDown & PADDLEBOAT_MOUSE_BUTTON_MIDDLE) != 0)
                ? TEXTCOLOR_GREEN : TEXTCOLOR_WHITE;
        ImGui::SameLine();
        ImGui::TextColored(buttonColor, "M");

        buttonColor = ((mouseData.buttonsDown & PADDLEBOAT_MOUSE_BUTTON_RIGHT) != 0)
                ? TEXTCOLOR_GREEN : TEXTCOLOR_WHITE;
        ImGui::SameLine();
        ImGui::TextColored(buttonColor, "R");

        buttonColor = ((mouseData.buttonsDown & PADDLEBOAT_MOUSE_BUTTON_FORWARD) != 0)
                ? TEXTCOLOR_GREEN : TEXTCOLOR_WHITE;
        ImGui::SameLine();
        ImGui::TextColored(buttonColor, "F");

        buttonColor = ((mouseData.buttonsDown & PADDLEBOAT_MOUSE_BUTTON_BACK) != 0)
                      ? TEXTCOLOR_GREEN : TEXTCOLOR_WHITE;
        ImGui::SameLine();
        ImGui::TextColored(buttonColor, "B");

        if (mouseData.mouseScrollDeltaV < 0) {
            buttonColor = TEXTCOLOR_RED;
            ImGui::SameLine();
            ImGui::TextColored(buttonColor, "%d", mouseData.mouseScrollDeltaV);
        } else if (mouseData.mouseScrollDeltaV > 0) {
            buttonColor = TEXTCOLOR_GREEN;
            ImGui::SameLine();
            ImGui::TextColored(buttonColor, "%d", mouseData.mouseScrollDeltaV);
        }

        if (mouseData.mouseScrollDeltaH < 0) {
            buttonColor = TEXTCOLOR_RED;
            ImGui::SameLine();
            ImGui::TextColored(buttonColor, "%d", mouseData.mouseScrollDeltaH);
        } else if (mouseData.mouseScrollDeltaH > 0) {
            buttonColor = TEXTCOLOR_GREEN;
            ImGui::SameLine();
            ImGui::TextColored(buttonColor, "%d", mouseData.mouseScrollDeltaH);
        }
        // This is needed by the controller info tab, so we calculate it here
        // where we process the mouse data.
        mMouseDataTimestampDelta = mouseData.timestamp - mPreviousMouseDataTimestamp;
        mPreviousMouseDataTimestamp = mouseData.timestamp;
    }
}

void DemoScene::RenderIntegratedMotionData() {
    // Motion axis
    float motionData[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    const bool hasAccel =
            ((mIntegratedSensorFlags & PADDLEBOAT_INTEGRATED_SENSOR_ACCELEROMETER) != 0);
    const bool hasGyro =
            ((mIntegratedSensorFlags & PADDLEBOAT_INTEGRATED_SENSOR_GYROSCOPE) != 0);;
    if (hasAccel) {
        motionData[0] = mIntegratedAccelerometerData[0];
        motionData[2] = mIntegratedAccelerometerData[1];
        motionData[4] = mIntegratedAccelerometerData[2];
    }

    if (hasGyro) {
        motionData[1] = mIntegratedGyroscopeData[0];
        motionData[3] = mIntegratedGyroscopeData[1];
        motionData[5] = mIntegratedGyroscopeData[2];
    }
    ImGui::Text("%s", "Integrated sensors");
    RenderMotionTableData(motionData, mIntegratedAccelerometerTimestampDelta,
                          mIntegratedGyroscopeTimestampDelta, hasAccel, hasGyro);
}

void DemoScene::RenderMotionTableData(const float *motionData, const uint32_t accelTimestampDelta,
                                      const uint32_t gyroTimestampDelta,
                                      bool hasAccel, bool hasGyro) {
    if (ImGui::BeginTable("##motiontable", 2, ImGuiTableColumnFlags_WidthFixed,
                          ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 4.5f))) {
        ImGui::TableSetupColumn("Accelerometer   ", 0);
        ImGui::TableSetupColumn("Gyroscope  ", 0);
        ImGui::TableHeadersRow();
        for (size_t i = 0; i < 4; ++i) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (hasAccel) {
                if (i == 0) {
                    ImGui::Text("%4u", accelTimestampDelta);
                } else {
                    ImGui::Text("%.3f", motionData[((i - 1) * 2)]);
                }
            } else if (i == 0) {
                ImGui::Text("None");
            }
            ImGui::TableNextColumn();
            if (hasGyro) {
                if (i == 0) {
                    ImGui::Text("%4u", gyroTimestampDelta);
                } else {
                    ImGui::Text("%.3f", motionData[((i - 1) * 2) + 1]);
                }
            } else if (i == 0) {
                ImGui::Text("None");
            }
        }
        ImGui::EndTable();
    }
}

void DemoScene::RenderIntegratedPanel() {
    ImGui::Text("%s", "Integrated (non-controller) device info");
    if (mIntegratedSensorFlags != PADDLEBOAT_INTEGRATED_SENSOR_NONE) {
        ImGui::Text("%s", "Motion data:");
        RenderIntegratedMotionData();
    }
}

void DemoScene::RenderPanel(const int32_t controllerIndex) {
    if (mActiveControllers[controllerIndex]) {
        Paddleboat_Controller_Data controllerData;
        Paddleboat_Controller_Info controllerInfo;
        char controllerName[128];
        Paddleboat_getControllerData(controllerIndex, &controllerData);
        Paddleboat_getControllerInfo(controllerIndex, &controllerInfo);
        Paddleboat_getControllerName(controllerIndex, 128, controllerName);

        ImGui::Text("%s", controllerName);

        if (ImGui::BeginTabBar("DisplayTabBar", ImGuiTabBarFlags_NoTooltip)) {
            ImGuiTabItemFlags tabItemFlags = ImGuiTabItemFlags_None;
            bool activeTab = false;

            const ControllerCategoryTab categoryTabs[] = {
                    {0, " Controls ",  &DemoScene::RenderPanel_ControlsTab},
                    {1, " Info ",      &DemoScene::RenderPanel_InfoTab},
                    {2, " Vibration ", &DemoScene::RenderPanel_VibrationTab},
                    {3, " Motion ",    &DemoScene::RenderPanel_MotionTab},
                    {4, " Lights ",    &DemoScene::RenderPanel_LightsTab},
            };
            const size_t categoryTabCount = ARRAY_COUNTOF(categoryTabs);

            for (size_t i = 0; i < categoryTabCount; ++i) {
                activeTab = (mActiveControllerPanelTab == categoryTabs[i].mTabIndex);
                ImVec4 tabTextColor = activeTab ? TEXTCOLOR_WHITE : TEXTCOLOR_GREY;
                ImGui::PushStyleColor(ImGuiCol_Text, tabTextColor);
                if (ImGui::BeginTabItem(categoryTabs[i].mTabTitle, NULL, tabItemFlags)) {
                    mActiveControllerPanelTab = categoryTabs[i].mTabIndex;
                    ImGui::PopStyleColor(1);
                    std::invoke(categoryTabs[i].mRenderFunction, this, controllerIndex,
                                controllerData, controllerInfo);
                    ImGui::EndTabItem();
                } else {
                    ImGui::PopStyleColor(1);
                }
            }

            ImGui::EndTabBar();
        }

        mPreviousControllerDataTimestamp = controllerData.timestamp;
    }
}

void DemoScene::RenderPanel_ControlsTab(const int32_t controllerIndex,
                                        const Paddleboat_Controller_Data &controllerData,
                                        const Paddleboat_Controller_Info &controllerInfo) {
    const uint32_t layout = controllerInfo.controllerFlags & PADDLEBOAT_CONTROLLER_LAYOUT_MASK;
    if (layout == PADDLEBOAT_CONTROLLER_LAYOUT_ARCADE_STICK) {
        RenderPanel_ControlsTab_ArcadeStick(controllerIndex, controllerData,
                                            controllerInfo);
        return;
    }

    if ((controllerInfo.controllerFlags & PADDLEBOAT_CONTROLLER_FLAG_VIRTUAL_MOUSE) != 0) {
        ImGui::Text(" Ptr X: %.0f Y: %.0f ", controllerData.virtualPointer.pointerX,
                    controllerData.virtualPointer.pointerY);
        if ((controllerInfo.controllerFlags & PADDLEBOAT_CONTROLLER_FLAG_TOUCHPAD) != 0) {
            bool touchpadDown = ((controllerData.buttonsDown & PADDLEBOAT_BUTTON_TOUCHPAD) !=
                                 0);
            ImVec4 touchpadColor = touchpadDown ? TEXTCOLOR_GREEN : TEXTCOLOR_GREY;
            ImGui::SameLine();
            ImGui::TextColored(touchpadColor, "Touchpad down");
        }
    }
    ConfigureButtonLayout(layout);
    mControllerPanelBaseY = ImGui::GetCursorPosY();

    ControllerUIPanelParams panelParams{mControllerPanelBaseX, mControllerPanelBaseY,
                                        mControllerPanelScale};

    // Buttons
    for (uint32_t index = UIBUTTON_A; index < UIBUTTON_COUNT; ++index) {
        const ControllerUIButtons buttonId = static_cast<const ControllerUIButtons>(index);
        ControllerButtonInfo buttonInfo = ControllerUIData::getControllerButtonInfo(buttonId);
        buttonInfo.buttonState = (controllerData.buttonsDown & buttonInfo.buttonMask)
                                 ? UIBUTTON_STATE_ACTIVE
                                 : UIBUTTON_STATE_IDLE;
        if (buttonInfo.enabled) {
            ControllerUIUtil::Button(panelParams, buttonId, buttonInfo);
        }
    }

    // Thumbsticks
    const float stickFlatLX = fmax(FLT_MIN, controllerInfo.leftStickPrecision.stickFlatX);
    const float stickFlatLY = fmax(FLT_MIN, controllerInfo.leftStickPrecision.stickFlatY);
    const float stickFlatRX = fmax(FLT_MIN, controllerInfo.rightStickPrecision.stickFlatX);
    const float stickFlatRY = fmax(FLT_MIN, controllerInfo.rightStickPrecision.stickFlatY);
    ControllerUIStickStates leftStickState = UISTICK_STATE_IDLE;
    ControllerUIStickStates rightStickState = UISTICK_STATE_IDLE;

    if (controllerData.buttonsDown & PADDLEBOAT_BUTTON_L3) {
        leftStickState = UISTICK_STATE_DEPRESSED;
    } else if (fabsf(controllerData.leftStick.stickX) > stickFlatLX ||
               fabsf(controllerData.leftStick.stickY) > stickFlatLY) {
        leftStickState = UISTICK_STATE_ACTIVE;
    }

    if (controllerData.buttonsDown & PADDLEBOAT_BUTTON_R3) {
        rightStickState = UISTICK_STATE_DEPRESSED;
    } else if (fabsf(controllerData.rightStick.stickX) > stickFlatRX ||
               fabsf(controllerData.rightStick.stickY) > stickFlatRY) {
        rightStickState = UISTICK_STATE_ACTIVE;
    }

    float leftX = controllerData.leftStick.stickX;
    float leftY = controllerData.leftStick.stickY;
    float rightX = controllerData.rightStick.stickX;
    float rightY = controllerData.rightStick.stickY;
    // Unless overridden, set the stick X/Y to 0.0 if it's in the
    // center deadzone
    if (mDontTrimDeadzone == false && leftStickState == UISTICK_STATE_IDLE) {
        leftX = 0.0f;
        leftY = 0.0f;
    }
    if (mDontTrimDeadzone == false && rightStickState == UISTICK_STATE_IDLE) {
        rightX = 0.0f;
        rightY = 0.0f;
    }

    const float stickScale = ControllerUIData::getStickScale();
    ImVec2 leftStickValues(leftX * stickScale,
                           leftY * stickScale);
    ImVec2 rightStickValues(rightX * stickScale,
                            rightY * stickScale);

    ControllerUIUtil::Thumbstick(panelParams, ControllerUIData::getStickPosition(true),
                                 leftStickValues, leftStickState);
    ControllerUIUtil::Thumbstick(panelParams, ControllerUIData::getStickPosition(false),
                                 rightStickValues, rightStickState);

    // Trigger fill bars
    const float windowStartY = NativeEngine::GetInstance()->GetSystemBarOffset();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ControllerUIUtil::TriggerBar(panelParams, draw_list, UIBUTTON_L1, controllerData.triggerL1,
                                 windowStartY);
    ControllerUIUtil::TriggerBar(panelParams, draw_list, UIBUTTON_L2, controllerData.triggerL2,
                                 windowStartY);
    ControllerUIUtil::TriggerBar(panelParams, draw_list, UIBUTTON_R1, controllerData.triggerR1,
                                 windowStartY);
    ControllerUIUtil::TriggerBar(panelParams, draw_list, UIBUTTON_R2, controllerData.triggerR2,
                                 windowStartY);
}

void DemoScene::RenderPanel_ControlsTab_ArcadeStick(const int32_t /*controllerIndex*/,
        const Paddleboat_Controller_Data &controllerData,
        const Paddleboat_Controller_Info &controllerInfo) {
    const float leftX = controllerData.leftStick.stickX;
    const float leftY = controllerData.leftStick.stickY;
    const uint32_t buttonsDown = controllerData.buttonsDown;

    mControllerPanelBaseY = ImGui::GetCursorPosY();

    ControllerUIPanelParams panelParams{mControllerPanelBaseX, mControllerPanelBaseY,
                                        mControllerPanelScale};

    // Arcade stick
    const float stickFlatLX = fmax(FLT_MIN, controllerInfo.leftStickPrecision.stickFlatX);
    const float stickFlatLY = fmax(FLT_MIN, controllerInfo.leftStickPrecision.stickFlatY);
    ControllerUIStickStates leftStickState = UISTICK_STATE_IDLE;

    if (fabsf(leftX) > stickFlatLX || fabsf(leftY) > stickFlatLY) {
        leftStickState = UISTICK_STATE_ACTIVE;
    }


    const float stickScale = ControllerUIData::getStickScale();
    ImVec2 leftStickValues(leftX * stickScale,
                           leftY * stickScale);

    ControllerUIUtil::Thumbstick(panelParams, ControllerUIData::getStickPosition(true),
                                 leftStickValues, leftStickState);

    // Buttons
    for (uint32_t index = UIBUTTON_A; index < UIBUTTON_COUNT; ++index) {
        const ControllerUIButtons buttonId = static_cast<const ControllerUIButtons>(index);
        ControllerButtonInfo buttonInfo =
                ControllerUIData::getControllerButtonInfo_ArcadeStick(buttonId);
        buttonInfo.buttonState = (buttonsDown & buttonInfo.buttonMask)
                                 ? UIBUTTON_STATE_ACTIVE
                                 : UIBUTTON_STATE_IDLE;
        if (buttonInfo.enabled) {
            ControllerUIUtil::Button(panelParams, buttonId, buttonInfo);
        }
    }

}

void DemoScene::RenderPanel_InfoTab(const int32_t /*controllerIndex*/,
                                    const Paddleboat_Controller_Data &controllerData,
                                    const Paddleboat_Controller_Info &controllerInfo) {
    // Render vendorId/deviceId in green if they matched with a controller map entry,
    // white if using the generic controller map profile.
    ImVec4 deviceColor =
            ((controllerInfo.controllerFlags & PADDLEBOAT_CONTROLLER_FLAG_GENERIC_PROFILE) != 0)
            ? TEXTCOLOR_WHITE : TEXTCOLOR_GREEN;

    ImGui::TextColored(deviceColor, "Contr. Num.: %d - VendorId: 0x%x - ProductId: 0x%x",
                       controllerInfo.controllerNumber, controllerInfo.vendorId,
                       controllerInfo.productId);
    const uint32_t layout = controllerInfo.controllerFlags & PADDLEBOAT_CONTROLLER_LAYOUT_MASK;
    switch (layout) {
        case PADDLEBOAT_CONTROLLER_LAYOUT_STANDARD:
            ImGui::Text("Standard layout");
            break;
        case PADDLEBOAT_CONTROLLER_LAYOUT_SHAPES:
            ImGui::Text("Shapes layout");
            break;
        case PADDLEBOAT_CONTROLLER_LAYOUT_REVERSE:
            ImGui::Text("Reversed layout");
            break;
        case PADDLEBOAT_CONTROLLER_LAYOUT_ARCADE_STICK:
            ImGui::Text("Arcade layout");
            break;
        default:
            ImGui::Text("Unknown layout");
            break;
    }

    ImGui::Text("Left Flat X/Y %f, %f\nFuzz X/Y: %f, %f",
                controllerInfo.leftStickPrecision.stickFlatX,
                controllerInfo.leftStickPrecision.stickFlatY,
                controllerInfo.leftStickPrecision.stickFuzzX,
                controllerInfo.leftStickPrecision.stickFuzzY);

    ImGui::Text("Right Flat X/Y %f, %f\nFuzz X/Y: %f, %f",
                controllerInfo.rightStickPrecision.stickFlatX,
                controllerInfo.rightStickPrecision.stickFlatY,
                controllerInfo.rightStickPrecision.stickFuzzX,
                controllerInfo.rightStickPrecision.stickFuzzY);

    uint64_t dataTimestampDelta = controllerData.timestamp - mPreviousControllerDataTimestamp;
    // Convert to milliseconds and cast to 32 bits
    int32_t dataDeltaMS = static_cast<int32_t>((dataTimestampDelta / 1000));
    ImGui::Text("Controller data delta timestamp (ms): %d", dataDeltaMS);

    // Convert to milliseconds and cast to 32 bits
    int32_t dataMouseDeltaMS = static_cast<int32_t>((mMouseDataTimestampDelta / 1000));
    ImGui::Text("Mouse data delta timestamp (ms): %d", dataMouseDeltaMS);

    ImGui::Text("Last Keycode: %d   ", Paddleboat_getLastKeycode());
    ImGui::SameLine();

    if ((controllerInfo.controllerFlags & PADDLEBOAT_CONTROLLER_FLAG_BATTERY) != 0) {
        const char *batteryStatusString = "Unknown";
        switch (controllerData.battery.batteryStatus) {
            case PADDLEBOAT_CONTROLLER_BATTERY_UNKNOWN:
                break;
            case PADDLEBOAT_CONTROLLER_BATTERY_CHARGING:
                batteryStatusString = "Charging";
                break;
            case PADDLEBOAT_CONTROLLER_BATTERY_DISCHARGING:
                batteryStatusString = "Discharging";
                break;
            case PADDLEBOAT_CONTROLLER_BATTERY_NOT_CHARGING:
                batteryStatusString = "Not charging";
                break;
            case PADDLEBOAT_CONTROLLER_BATTERY_FULL:
                batteryStatusString = "Full";
                break;
        }
        ImGui::Text("Battery %.1f%%, %s",
                    controllerData.battery.batteryLevel * 100.0f, batteryStatusString);
    } else {
        ImGui::Text("No battery status available");
    }
}

void DemoScene::RenderPanel_VibrationTab(const int32_t controllerIndex,
                                         const Paddleboat_Controller_Data &/*controllerData*/,
                                         const Paddleboat_Controller_Info &controllerInfo) {
    if ((controllerInfo.controllerFlags & PADDLEBOAT_CONTROLLER_FLAG_VIBRATION) != 0) {
        if ((controllerInfo.controllerFlags & PADDLEBOAT_CONTROLLER_FLAG_VIBRATION_DUAL_MOTOR)
             != 0) {
            ImGui::Text("Dual motor vibration support");
        } else {
            ImGui::Text("Single vibration device");
        }
        static float leftMotorDuration = 500;
        static float rightMotorDuration = 500;
        static float leftMotorIntensity = 0.4f;
        static float rightMotorIntensity = 0.4f;

        VibrationParameters("Left Duration:   ", "ldur", VIBRATION_DURATION_MIN,
                VIBRATION_DURATION_MAX, VIBRATION_DURATION_STEP, &leftMotorDuration);
        VibrationParameters("Left Intensity:  ", "lint", VIBRATION_INTENSITY_MIN,
                VIBRATION_INTENSITY_MAX, VIBRATION_INTENSITY_STEP, &leftMotorIntensity);
        VibrationParameters("Right Duration:  ", "rdur", VIBRATION_DURATION_MIN,
                VIBRATION_DURATION_MAX, VIBRATION_DURATION_STEP, &rightMotorDuration);
        VibrationParameters("Right Intensity: ", "rint", VIBRATION_INTENSITY_MIN,
                VIBRATION_INTENSITY_MAX, VIBRATION_INTENSITY_STEP, &rightMotorIntensity);

        if (ImGui::Button(" Vibrate ")) {
            Paddleboat_Vibration_Data vibrationData = {
                    static_cast<int32_t>(leftMotorDuration),
                    static_cast<int32_t>(rightMotorDuration),
                    leftMotorIntensity,
                    rightMotorIntensity
            };
            Paddleboat_setControllerVibrationData(controllerIndex, &vibrationData,
                                                  NativeEngine::GetInstance()->GetJniEnv());
        }
    } else {
        ImGui::Text("No vibration support");
    }
}

void DemoScene::RenderPanel_MotionTab(const int32_t /*controllerIndex*/,
                                      const Paddleboat_Controller_Data &/*controllerData*/,
                                      const Paddleboat_Controller_Info &controllerInfo) {
    // Motion axis
    float motionData[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    const bool hasAccel =
            (controllerInfo.controllerFlags & PADDLEBOAT_CONTROLLER_FLAG_ACCELEROMETER) != 0;
    const bool hasGyro =
            (controllerInfo.controllerFlags & PADDLEBOAT_CONTROLLER_FLAG_GYROSCOPE) != 0;
    if (hasAccel) {
        motionData[0] = mAccelerometerData[0];
        motionData[2] = mAccelerometerData[1];
        motionData[4] = mAccelerometerData[2];
    }

    if (hasGyro) {
        motionData[1] = mGyroscopeData[0];
        motionData[3] = mGyroscopeData[1];
        motionData[5] = mGyroscopeData[2];
    }
    RenderMotionTableData(motionData, mAccelerometerTimestampDelta,
                          mGyroscopeTimestampDelta, hasAccel, hasGyro);
}

void DemoScene::RenderPanel_LightsTab(const int32_t controllerIndex,
                                      const Paddleboat_Controller_Data &/*controllerData*/,
                                      const Paddleboat_Controller_Info &controllerInfo) {
    if ((controllerInfo.controllerFlags & PADDLEBOAT_CONTROLLER_FLAG_LIGHT_PLAYER) != 0) {
        static int playerIndex = 1;
        ImGui::DragInt("Player index", &playerIndex, 1.0f, 1, 4,
                       "%d", ImGuiSliderFlags_NoInput);
        if (ImGui::Button("Set Player Index Light")) {
            Paddleboat_setControllerLight(controllerIndex, PADDLEBOAT_LIGHT_PLAYER_NUMBER,
                                          playerIndex, NativeEngine::GetInstance()->GetJniEnv());
        }
    } else {
        ImGui::Text("No player index light present");
    }

    if ((controllerInfo.controllerFlags & PADDLEBOAT_CONTROLLER_FLAG_LIGHT_RGB) != 0) {
        static float rgbLights[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        ImGui::ColorEdit4("LightColor", rgbLights);
        if (ImGui::Button("Set Light Color")) {
            uint32_t r = static_cast<uint32_t>(rgbLights[0] * 255.0f);
            uint32_t g = static_cast<uint32_t>(rgbLights[1] * 255.0f);
            uint32_t b = static_cast<uint32_t>(rgbLights[2] * 255.0f);
            uint32_t a = static_cast<uint32_t>(rgbLights[3] * 255.0f);
            uint32_t rgba = (a << 24) | (r << 16) | (g << 8) | b;
            Paddleboat_setControllerLight(controllerIndex, PADDLEBOAT_LIGHT_RGB, rgba,
                                          NativeEngine::GetInstance()->GetJniEnv());
        }
    } else {
        ImGui::Text("No RGB light present");
    }
}

void DemoScene::ConfigureButtonLayout(const uint32_t layout) {
    bool usingShapes = false;
    bool reverseButtons = false;

    switch (layout) {
        case PADDLEBOAT_CONTROLLER_LAYOUT_STANDARD:
            break;
        case PADDLEBOAT_CONTROLLER_LAYOUT_SHAPES:
            usingShapes = true;
            break;
        case PADDLEBOAT_CONTROLLER_LAYOUT_REVERSE:
            reverseButtons = true;
            break;
        case PADDLEBOAT_CONTROLLER_LAYOUT_ARCADE_STICK:
            break;
        default:
            break;
    }

    if (usingShapes) {
        ControllerUIData::getControllerButtonInfo(UIBUTTON_A).enabled = false;
        ControllerUIData::getControllerButtonInfo(UIBUTTON_B).enabled = false;
        ControllerUIData::getControllerButtonInfo(UIBUTTON_X).enabled = false;
        ControllerUIData::getControllerButtonInfo(UIBUTTON_Y).enabled = false;
        ControllerUIData::getControllerButtonInfo(UIBUTTON_CROSS).enabled = true;
        ControllerUIData::getControllerButtonInfo(UIBUTTON_CIRCLE).enabled = true;
        ControllerUIData::getControllerButtonInfo(UIBUTTON_SQUARE).enabled = true;
        ControllerUIData::getControllerButtonInfo(UIBUTTON_TRIANGLE).enabled = true;
    } else {
        ControllerUIData::getControllerButtonInfo(UIBUTTON_A).enabled = true;
        ControllerUIData::getControllerButtonInfo(UIBUTTON_B).enabled = true;
        ControllerUIData::getControllerButtonInfo(UIBUTTON_X).enabled = true;
        ControllerUIData::getControllerButtonInfo(UIBUTTON_Y).enabled = true;
        ControllerUIData::getControllerButtonInfo(UIBUTTON_CROSS).enabled = false;
        ControllerUIData::getControllerButtonInfo(UIBUTTON_CIRCLE).enabled = false;
        ControllerUIData::getControllerButtonInfo(UIBUTTON_SQUARE).enabled = false;
        ControllerUIData::getControllerButtonInfo(UIBUTTON_TRIANGLE).enabled = false;
    }

    if (reverseButtons) {
        ControllerUIData::getControllerButtonInfo(
                UIBUTTON_A).basePosition = ControllerUIData::getButtonQuadPosition(
                UIBUTTON_DPAD_RIGHT);
        ControllerUIData::getControllerButtonInfo(
                UIBUTTON_B).basePosition = ControllerUIData::getButtonQuadPosition(
                UIBUTTON_DPAD_DOWN);
        ControllerUIData::getControllerButtonInfo(
                UIBUTTON_X).basePosition = ControllerUIData::getButtonQuadPosition(
                UIBUTTON_DPAD_UP);
        ControllerUIData::getControllerButtonInfo(
                UIBUTTON_Y).basePosition = ControllerUIData::getButtonQuadPosition(
                UIBUTTON_DPAD_LEFT);
    } else {
        ControllerUIData::getControllerButtonInfo(
                UIBUTTON_A).basePosition = ControllerUIData::getButtonQuadPosition(
                UIBUTTON_DPAD_DOWN);
        ControllerUIData::getControllerButtonInfo(
                UIBUTTON_B).basePosition = ControllerUIData::getButtonQuadPosition(
                UIBUTTON_DPAD_RIGHT);
        ControllerUIData::getControllerButtonInfo(
                UIBUTTON_X).basePosition = ControllerUIData::getButtonQuadPosition(
                UIBUTTON_DPAD_LEFT);
        ControllerUIData::getControllerButtonInfo(
                UIBUTTON_Y).basePosition = ControllerUIData::getButtonQuadPosition(
                UIBUTTON_DPAD_UP);
    }
}

void DemoScene::GameControllerStatusEvent(const int32_t controllerIndex,
                                          const Paddleboat_ControllerStatus status) {
    if (controllerIndex >= 0 && controllerIndex < PADDLEBOAT_MAX_CONTROLLERS) {
        if (status == PADDLEBOAT_CONTROLLER_INACTIVE ||
            status == PADDLEBOAT_CONTROLLER_JUST_DISCONNECTED) {
            mActiveControllers[controllerIndex] = false;
        } else if (status == PADDLEBOAT_CONTROLLER_JUST_CONNECTED ||
                   status == PADDLEBOAT_CONTROLLER_ACTIVE) {
            mActiveControllers[controllerIndex] = true;
            mMostRecentConnectedControllerIndex = controllerIndex;
        }
    }

    // Log the event
    const char *statusString = "UNKNOWN";
    switch (status) {
        case PADDLEBOAT_CONTROLLER_INACTIVE:
            statusString = "PADDLEBOAT_CONTROLLER_INACTIVE";
            break;
        case PADDLEBOAT_CONTROLLER_JUST_CONNECTED:
            statusString = "PADDLEBOAT_CONTROLLER_JUST_CONNECTED";
            break;
        case PADDLEBOAT_CONTROLLER_JUST_DISCONNECTED:
            statusString = "PADDLEBOAT_CONTROLLER_JUST_DISCONNECTED";
            break;
        case PADDLEBOAT_CONTROLLER_ACTIVE:
            statusString = "PADDLEBOAT_CONTROLLER_ACTIVE";
            break;
    }
    ALOGI("Paddleboat_ControllerStatusEvent index: %d status: %s", controllerIndex, statusString);
}

void DemoScene::KeyboardStatusEvent(const bool keyboardConnected) {
    mKeyboardConnected = keyboardConnected;
}

void DemoScene::MotionDataEvent(const int32_t controllerIndex,
                                const Paddleboat_Motion_Data *motionData) {
    if (controllerIndex == PADDLEBOAT_INTEGRATED_SENSOR_INDEX) {
        if (motionData->motionType == PADDLEBOAT_MOTION_ACCELEROMETER) {
            UpdateMotionData(motionData, &mPreviousIntegratedAccelerometerTimestamp,
                             &mIntegratedAccelerometerTimestampDelta,
                             mIntegratedAccelerometerData);
        } else if (motionData->motionType == PADDLEBOAT_MOTION_GYROSCOPE) {
            UpdateMotionData(motionData, &mPreviousIntegratedGyroscopeTimestamp,
                             &mIntegratedGyroscopeTimestampDelta,
                             mIntegratedGyroscopeData);
        }
    } else if (controllerIndex == mCurrentControllerIndex) {
        if (motionData->motionType == PADDLEBOAT_MOTION_ACCELEROMETER) {
            UpdateMotionData(motionData, &mPreviousAccelerometerTimestamp,
                             &mAccelerometerTimestampDelta,
                             mAccelerometerData);
        } else if (motionData->motionType == PADDLEBOAT_MOTION_GYROSCOPE) {
            UpdateMotionData(motionData, &mPreviousGyroscopeTimestamp,
                             &mGyroscopeTimestampDelta,
                             mGyroscopeData);
        }
    }
}

void DemoScene::OnInstall() {
    mIntegratedSensorFlags = Paddleboat_getIntegratedMotionSensorFlags();

    Paddleboat_setControllerStatusCallback(GameControllerCallback, this);
    Paddleboat_setMotionDataCallbackWithIntegratedFlags(MotionDataCallback,
                                                        mIntegratedSensorFlags, this);
    Paddleboat_setPhysicalKeyboardStatusCallback(KeyboardCallback, this);
    mRegisteredStatusCallback = true;
}

void DemoScene::OnUninstall() {
    Paddleboat_setControllerStatusCallback(nullptr, nullptr);
    Paddleboat_setMotionDataCallback(nullptr, nullptr);
    Paddleboat_setPhysicalKeyboardStatusCallback(nullptr, nullptr);
    mRegisteredStatusCallback = false;
}
