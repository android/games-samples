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
#include "texture_asset_loader.h"

enum ControllerUIButtons {
    UIBUTTON_A = 0,
    UIBUTTON_B,
    UIBUTTON_X,
    UIBUTTON_Y,
    UIBUTTON_CROSS,
    UIBUTTON_CIRCLE,
    UIBUTTON_SQUARE,
    UIBUTTON_TRIANGLE,
    UIBUTTON_L1,
    UIBUTTON_L2,
    UIBUTTON_R1,
    UIBUTTON_R2,
    UIBUTTON_MODE,
    UIBUTTON_SELECT,
    UIBUTTON_START,
    UIBUTTON_DPAD_UP,
    UIBUTTON_DPAD_LEFT,
    UIBUTTON_DPAD_DOWN,
    UIBUTTON_DPAD_RIGHT,
    UIBUTTON_COUNT
};

enum ControllerUIButtonStates {
    UIBUTTON_STATE_IDLE = 0,
    UIBUTTON_STATE_ACTIVE,
    UIBUTTON_STATE_COUNT
};

enum ControllerUIStickStates {
    UISTICK_STATE_IDLE = 0,
    UISTICK_STATE_ACTIVE,
    UISTICK_STATE_DEPRESSED,
    UISTICK_STATE_COUNT
};

constexpr uint32_t MAX_UITEXTURE_STATES = 4;

struct ControllerUIButtonTextureHandles {
    TextureAssetHandle textureHandles[UIBUTTON_STATE_COUNT];
};

struct ControllerUIStickTextureHandles {
    TextureAssetHandle textureHandles[UISTICK_STATE_COUNT];
};

struct ControllerUIButtonDefinition {
    const char *assetName_Active;
    const char *assetName_Idle;
};

struct ControllerStickUIDefinition {
    const char *assetName_Active;
    const char *assetName_Depressed;
    const char *assetName_Idle;
};

struct UITextureInfo {
    TextureAssetHandle textureHandles[MAX_UITEXTURE_STATES];
    uint32_t textureWidth;
    uint32_t textureHeight;
};

struct ControllerButtonInfo {
    ImVec2 basePosition;
    uint32_t buttonMask;
    ControllerUIButtonStates buttonState;
    bool enabled;
};

class ControllerUIData {
public:
    static void LoadControllerUIData();

    static void UnloadControllerUIData();

    static const UITextureInfo &getUIButtonTextures(const ControllerUIButtons uiButton);

    static const UITextureInfo &getUIStickTextures();

    static const UITextureInfo &getUIStickRegionTexture();

    static ControllerButtonInfo &getControllerButtonInfo(const ControllerUIButtons uiButton);

    static ControllerButtonInfo &getControllerButtonInfo_ArcadeStick(
            const ControllerUIButtons uiButton);

    static ImVec2 getStickPosition(const bool isLeftStick);

    // Use the dpad enum to specify which position on the *quad* button layout
    static ImVec2 getButtonQuadPosition(const ControllerUIButtons dpadButton);

    static float getStickScale();

    static void
    getTriggerRectExtents(const ControllerUIButtons uiButton, ImVec2 *rectMin, ImVec2 *rectMax);
};
