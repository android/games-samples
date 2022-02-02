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

#include "imgui.h"

struct ControllerUIPanelParams {
    float panelBaseX;
    float panelBaseY;
    float panelImageScale;
};

class ControllerUIUtil {
public:
    static void
    Button(const ControllerUIPanelParams &panelParams, const ControllerUIButtons buttonId,
           const ControllerButtonInfo &buttonInfo);

    static void Thumbstick(const ControllerUIPanelParams &panelParams, const ImVec2 &basePos,
                           const ImVec2 &stickVals,
                           const ControllerUIStickStates stickState);

    // function outputs to draw_list parameter, must not be null
    static void TriggerBar(const ControllerUIPanelParams &panelParams, ImDrawList *draw_list,
                           const ControllerUIButtons buttonId,
                           const float triggerValue,
                           const float offsetY);
};
