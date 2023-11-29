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

#include "paddleboat/paddleboat.h"

#include "controllerui_data.h"
#include "texture_asset_loader.h"

namespace {
    bool uiDataInitialized = false;

    UITextureInfo buttonTextures[UIBUTTON_COUNT];
    UITextureInfo stickTextures;
    UITextureInfo stickRegionTexture;

    const ControllerUIButtonDefinition buttonDefinitions[UIBUTTON_COUNT] = {
            {"gamecontroller/Button_A_Active.png",
                    "gamecontroller/Button_A_Idle.png"
            },
            {"gamecontroller/Button_B_Active.png",
                    "gamecontroller/Button_B_Idle.png"
            },
            {"gamecontroller/Button_X_Active.png",
                    "gamecontroller/Button_X_Idle.png"
            },
            {"gamecontroller/Button_Y_Active.png",
                    "gamecontroller/Button_Y_Idle.png"
            },
            {"gamecontroller/Button_ShapeX_Active.png",
                    "gamecontroller/Button_ShapeX_Idle.png"
            },
            {"gamecontroller/Button_Circle_Active.png",
                    "gamecontroller/Button_Circle_Idle.png"
            },
            {"gamecontroller/Button_Square_Active.png",
                    "gamecontroller/Button_Square_Idle.png"
            },
            {"gamecontroller/Button_Triangle_Active.png",
                    "gamecontroller/Button_Triangle_Idle.png"
            },
            {"gamecontroller/Button_L1_Active.png",
                    "gamecontroller/Button_L1_Idle.png"
            },
            {"gamecontroller/Button_L2_Active.png",
                    "gamecontroller/Button_L2_Idle.png"
            },
            {"gamecontroller/Button_R1_Active.png",
                    "gamecontroller/Button_R1_Idle.png"
            },
            {"gamecontroller/Button_R2_Active.png",
                    "gamecontroller/Button_R2_Idle.png"
            },
            {"gamecontroller/Button_Mode_Active.png",
                    "gamecontroller/Button_Mode_Idle.png"
            },
            {"gamecontroller/Button_Select_Active.png",
                    "gamecontroller/Button_Select_Idle.png"
            },
            {"gamecontroller/Button_Start_Active.png",
                    "gamecontroller/Button_Start_Idle.png"
            },
            {"gamecontroller/Button_DPad_Up_Active.png",
                    "gamecontroller/Button_DPad_Up_Idle.png"
            },
            {"gamecontroller/Button_DPad_Left_Active.png",
                    "gamecontroller/Button_DPad_Left_Idle.png"
            },
            {"gamecontroller/Button_DPad_Down_Active.png",
                    "gamecontroller/Button_DPad_Down_Idle.png"
            },
            {"gamecontroller/Button_DPad_Right_Active.png",
                    "gamecontroller/Button_DPad_Right_Idle.png"
            }
    };

    const ControllerStickUIDefinition stickDefinition = {
            "gamecontroller/Thumbstick_Active.png",
            "gamecontroller/Thumbstick_Depressed.png",
            "gamecontroller/Thumbstick_Idle.png"
    };

    const char *stickRegionAssetName = "gamecontroller/Thumbstick_Region.png";

    // UI Layout constants
    // Base position constants
    // Row base Y
    constexpr float ROW_QUAD_AND_STICKS_BASE_Y = 160.0f;
    constexpr float ROW_BUTTON_SYSTEM_BASE_Y = ROW_QUAD_AND_STICKS_BASE_Y - 128.0f; //144.0f;
    constexpr float ROW_BUTTON_TRIGGER_BASE_Y = ROW_QUAD_AND_STICKS_BASE_Y - 112.0f; //128.0f;

    constexpr float STICK_LEFT_X = 128.0f;
    constexpr float STICK_BETWEEN_WIDTH = 224.0f;
    constexpr float STICK_RIGHT_X = STICK_LEFT_X + STICK_BETWEEN_WIDTH;
    constexpr float STICK_BASE_Y = ROW_QUAD_AND_STICKS_BASE_Y;
    constexpr float STICK_SCALE = 64.0f;

    constexpr float BUTTON_QUAD_BASE_X = STICK_LEFT_X + 384.0f;
    constexpr float BUTTON_QUAD_BASE_Y = ROW_QUAD_AND_STICKS_BASE_Y;
    constexpr float BUTTON_QUAD_X_ADJUST = 32.0f;
    constexpr float BUTTON_QUAD_Y_ADJUST = 32.0f;

    constexpr float BUTTON_TRIGGER_LEFT_X = 48.0f;
    constexpr float BUTTON_TRIGGER_RIGHT_X = 720.0f;
    constexpr float BUTTON_TRIGGER_ADJUST_Y = 32.0f;

    constexpr float TRIGGER_BAR_ADJUST_X = 54.0f;
    constexpr float TRIGGER_BAR_ADJUST_Y = -16.0f;
    constexpr float TRIGGER_BAR_WIDTH = 128.0f;
    constexpr float TRIGGER_BAR_HEIGHT = 32.0f;

    constexpr float BUTTON_SYSTEM_BASE_X = 380.0f;
    constexpr float BUTTON_SYSTEM_X_ADJUST = 96.0f;

    constexpr float DPAD_BASE_X = BUTTON_QUAD_BASE_X + 144.0f;
    constexpr float DPAD_BASE_Y = ROW_QUAD_AND_STICKS_BASE_Y;
    constexpr float DPAD_CENTER_ADJUST = 32.0f;

    // Button/dpad element constants
    constexpr float BUTTON_QUAD_TOP_X = BUTTON_QUAD_BASE_X;
    constexpr float BUTTON_QUAD_TOP_Y = BUTTON_QUAD_BASE_Y - BUTTON_QUAD_Y_ADJUST;
    constexpr float BUTTON_QUAD_LEFT_X = BUTTON_QUAD_BASE_X - BUTTON_QUAD_X_ADJUST;
    constexpr float BUTTON_QUAD_LEFT_Y = BUTTON_QUAD_BASE_Y;
    constexpr float BUTTON_QUAD_BOTTOM_X = BUTTON_QUAD_BASE_X;
    constexpr float BUTTON_QUAD_BOTTOM_Y = BUTTON_QUAD_BASE_Y + BUTTON_QUAD_Y_ADJUST;
    constexpr float BUTTON_QUAD_RIGHT_X = BUTTON_QUAD_BASE_X + BUTTON_QUAD_X_ADJUST;
    constexpr float BUTTON_QUAD_RIGHT_Y = BUTTON_QUAD_BASE_Y;

    constexpr float BUTTON_L1_X = BUTTON_TRIGGER_LEFT_X;
    constexpr float BUTTON_L1_Y = ROW_BUTTON_TRIGGER_BASE_Y;
    constexpr float BUTTON_L2_X = BUTTON_TRIGGER_LEFT_X;
    constexpr float BUTTON_L2_Y = ROW_BUTTON_TRIGGER_BASE_Y - BUTTON_TRIGGER_ADJUST_Y;

    constexpr float BUTTON_R1_X = BUTTON_TRIGGER_RIGHT_X;
    constexpr float BUTTON_R1_Y = ROW_BUTTON_TRIGGER_BASE_Y;
    constexpr float BUTTON_R2_X = BUTTON_TRIGGER_RIGHT_X;
    constexpr float BUTTON_R2_Y = ROW_BUTTON_TRIGGER_BASE_Y - BUTTON_TRIGGER_ADJUST_Y;

    constexpr float BUTTON_SELECT_X = BUTTON_SYSTEM_BASE_X - BUTTON_SYSTEM_X_ADJUST;
    constexpr float BUTTON_SELECT_Y = ROW_BUTTON_SYSTEM_BASE_Y;
    constexpr float BUTTON_MODE_X = BUTTON_SYSTEM_BASE_X;
    constexpr float BUTTON_MODE_Y = ROW_BUTTON_SYSTEM_BASE_Y;
    constexpr float BUTTON_START_X = BUTTON_SYSTEM_BASE_X + BUTTON_SYSTEM_X_ADJUST;
    constexpr float BUTTON_START_Y = ROW_BUTTON_SYSTEM_BASE_Y;

    constexpr float DPAD_UP_X = DPAD_BASE_X;
    constexpr float DPAD_UP_Y = DPAD_BASE_Y - DPAD_CENTER_ADJUST;
    constexpr float DPAD_LEFT_X = DPAD_BASE_X - DPAD_CENTER_ADJUST;
    constexpr float DPAD_LEFT_Y = DPAD_BASE_Y;
    constexpr float DPAD_DOWN_X = DPAD_BASE_X;
    constexpr float DPAD_DOWN_Y = DPAD_BASE_Y + DPAD_CENTER_ADJUST;
    constexpr float DPAD_RIGHT_X = DPAD_BASE_X + DPAD_CENTER_ADJUST;
    constexpr float DPAD_RIGHT_Y = DPAD_BASE_Y;

    // Arcade stick variants
    constexpr float AS_DPAD_BASE_X = STICK_LEFT_X + 144.0f;
    constexpr float AS_DPAD_BASE_Y = ROW_QUAD_AND_STICKS_BASE_Y;

    constexpr float AS_DPAD_UP_X = AS_DPAD_BASE_X;
    constexpr float AS_DPAD_UP_Y = AS_DPAD_BASE_Y - DPAD_CENTER_ADJUST;
    constexpr float AS_DPAD_LEFT_X = AS_DPAD_BASE_X - DPAD_CENTER_ADJUST;
    constexpr float AS_DPAD_LEFT_Y = AS_DPAD_BASE_Y;
    constexpr float AS_DPAD_DOWN_X = AS_DPAD_BASE_X;
    constexpr float AS_DPAD_DOWN_Y = AS_DPAD_BASE_Y + DPAD_CENTER_ADJUST;
    constexpr float AS_DPAD_RIGHT_X = AS_DPAD_BASE_X + DPAD_CENTER_ADJUST;
    constexpr float AS_DPAD_RIGHT_Y = AS_DPAD_BASE_Y;

    constexpr float AS_BUTTONS_BASE_X = AS_DPAD_BASE_X + 96.0f;
    constexpr float AS_BUTTONS_BASE_Y = ROW_QUAD_AND_STICKS_BASE_Y + 32.0f;

    constexpr float AS_BUTTON_A_X = AS_BUTTONS_BASE_X;
    constexpr float AS_BUTTON_A_Y = AS_BUTTONS_BASE_Y;
    constexpr float AS_BUTTON_B_X = AS_BUTTON_A_X + 40.0f;
    constexpr float AS_BUTTON_B_Y = AS_BUTTON_A_Y - 16.0f;
    constexpr float AS_BUTTON_R2_X = AS_BUTTON_B_X + 56.0f;
    constexpr float AS_BUTTON_R2_Y = AS_BUTTON_B_Y;
    constexpr float AS_BUTTON_L2_X = AS_BUTTON_R2_X + 64.0f;
    constexpr float AS_BUTTON_L2_Y = AS_BUTTON_R2_Y;

    constexpr float AS_BUTTON_X_X = AS_BUTTONS_BASE_X + 16.0f;
    constexpr float AS_BUTTON_X_Y = AS_BUTTONS_BASE_Y - 72.0f;
    constexpr float AS_BUTTON_Y_X = AS_BUTTON_X_X + 40.0f;
    constexpr float AS_BUTTON_Y_Y = AS_BUTTON_X_Y - 16.0f;
    constexpr float AS_BUTTON_R1_X = AS_BUTTON_Y_X + 56.0f;
    constexpr float AS_BUTTON_R1_Y = AS_BUTTON_Y_Y;
    constexpr float AS_BUTTON_L1_X = AS_BUTTON_R1_X + 64.0f;
    constexpr float AS_BUTTON_L1_Y = AS_BUTTON_R1_Y;

    ControllerButtonInfo controllerButtonInfo[UIBUTTON_COUNT] = {
            {{BUTTON_QUAD_BOTTOM_X, BUTTON_QUAD_BOTTOM_Y}, PADDLEBOAT_BUTTON_A,
                    UIBUTTON_STATE_IDLE, true},
            {{BUTTON_QUAD_RIGHT_X,  BUTTON_QUAD_RIGHT_Y},  PADDLEBOAT_BUTTON_B,
                    UIBUTTON_STATE_IDLE, true},
            {{BUTTON_QUAD_LEFT_X,   BUTTON_QUAD_LEFT_Y},   PADDLEBOAT_BUTTON_X,
                    UIBUTTON_STATE_IDLE, true},
            {{BUTTON_QUAD_TOP_X,    BUTTON_QUAD_TOP_Y},    PADDLEBOAT_BUTTON_Y,
                    UIBUTTON_STATE_IDLE, true},
            {{BUTTON_QUAD_BOTTOM_X, BUTTON_QUAD_BOTTOM_Y}, PADDLEBOAT_BUTTON_A,
                    UIBUTTON_STATE_IDLE, false},
            {{BUTTON_QUAD_RIGHT_X,  BUTTON_QUAD_RIGHT_Y},  PADDLEBOAT_BUTTON_B,
                    UIBUTTON_STATE_IDLE, false},
            {{BUTTON_QUAD_LEFT_X,   BUTTON_QUAD_LEFT_Y},   PADDLEBOAT_BUTTON_X,
                    UIBUTTON_STATE_IDLE, false},
            {{BUTTON_QUAD_TOP_X,    BUTTON_QUAD_TOP_Y},    PADDLEBOAT_BUTTON_Y,
                    UIBUTTON_STATE_IDLE, false},
            {{BUTTON_L1_X,          BUTTON_L1_Y},          PADDLEBOAT_BUTTON_L1,
                    UIBUTTON_STATE_IDLE, true},
            {{BUTTON_L2_X,          BUTTON_L2_Y},          PADDLEBOAT_BUTTON_L2,
                    UIBUTTON_STATE_IDLE, true},
            {{BUTTON_R1_X,          BUTTON_R1_Y},          PADDLEBOAT_BUTTON_R1,
                    UIBUTTON_STATE_IDLE, true},
            {{BUTTON_R2_X,          BUTTON_R2_Y},          PADDLEBOAT_BUTTON_R2,
                    UIBUTTON_STATE_IDLE, true},
            {{BUTTON_SELECT_X,      BUTTON_SELECT_Y},      PADDLEBOAT_BUTTON_SYSTEM,
                    UIBUTTON_STATE_IDLE, true},
            {{BUTTON_MODE_X,        BUTTON_MODE_Y},        PADDLEBOAT_BUTTON_SELECT,
                    UIBUTTON_STATE_IDLE, true},
            {{BUTTON_START_X,       BUTTON_START_Y},       PADDLEBOAT_BUTTON_START,
                    UIBUTTON_STATE_IDLE, true},
            {{DPAD_UP_X,            DPAD_UP_Y},            PADDLEBOAT_BUTTON_DPAD_UP,
                    UIBUTTON_STATE_IDLE, true},
            {{DPAD_LEFT_X,          DPAD_LEFT_Y},          PADDLEBOAT_BUTTON_DPAD_LEFT,
                    UIBUTTON_STATE_IDLE, true},
            {{DPAD_DOWN_X,          DPAD_DOWN_Y},          PADDLEBOAT_BUTTON_DPAD_DOWN,
                    UIBUTTON_STATE_IDLE, true},
            {{DPAD_RIGHT_X,         DPAD_RIGHT_Y},         PADDLEBOAT_BUTTON_DPAD_RIGHT,
                    UIBUTTON_STATE_IDLE, true}
    };

    ControllerButtonInfo controllerButtonInfo_ArcadeStick[UIBUTTON_COUNT] = {
            {{AS_BUTTON_A_X,        AS_BUTTON_A_Y},        PADDLEBOAT_BUTTON_A,
                    UIBUTTON_STATE_IDLE, true},
            {{AS_BUTTON_B_X,        AS_BUTTON_B_Y},        PADDLEBOAT_BUTTON_B,
                    UIBUTTON_STATE_IDLE, true},
            {{AS_BUTTON_X_X,        AS_BUTTON_X_Y},        PADDLEBOAT_BUTTON_X,
                    UIBUTTON_STATE_IDLE, true},
            {{AS_BUTTON_Y_X,        AS_BUTTON_Y_Y},        PADDLEBOAT_BUTTON_Y,
                    UIBUTTON_STATE_IDLE, true},
            {{BUTTON_QUAD_BOTTOM_X, BUTTON_QUAD_BOTTOM_Y}, PADDLEBOAT_BUTTON_A,
                    UIBUTTON_STATE_IDLE, false},
            {{BUTTON_QUAD_RIGHT_X,  BUTTON_QUAD_RIGHT_Y},  PADDLEBOAT_BUTTON_B,
                    UIBUTTON_STATE_IDLE, false},
            {{BUTTON_QUAD_LEFT_X,   BUTTON_QUAD_LEFT_Y},   PADDLEBOAT_BUTTON_X,
                    UIBUTTON_STATE_IDLE, false},
            {{BUTTON_QUAD_TOP_X,    BUTTON_QUAD_TOP_Y},    PADDLEBOAT_BUTTON_Y,
                    UIBUTTON_STATE_IDLE, false},
            {{AS_BUTTON_L1_X,       AS_BUTTON_L1_Y},       PADDLEBOAT_BUTTON_L1,
                    UIBUTTON_STATE_IDLE, true},
            {{AS_BUTTON_L2_X,       AS_BUTTON_L2_Y},       PADDLEBOAT_BUTTON_L2,
                    UIBUTTON_STATE_IDLE, true},
            {{AS_BUTTON_R1_X,       AS_BUTTON_R1_Y},       PADDLEBOAT_BUTTON_R1,
                    UIBUTTON_STATE_IDLE, true},
            {{AS_BUTTON_R2_X,       AS_BUTTON_R2_Y},       PADDLEBOAT_BUTTON_R2,
                    UIBUTTON_STATE_IDLE, true},
            {{BUTTON_SELECT_X,      BUTTON_SELECT_Y},      PADDLEBOAT_BUTTON_SYSTEM,
                    UIBUTTON_STATE_IDLE, true},
            {{BUTTON_MODE_X,        BUTTON_MODE_Y},        PADDLEBOAT_BUTTON_SELECT,
                    UIBUTTON_STATE_IDLE, true},
            {{BUTTON_START_X,       BUTTON_START_Y},       PADDLEBOAT_BUTTON_START,
                    UIBUTTON_STATE_IDLE, true},
            {{AS_DPAD_UP_X,         AS_DPAD_UP_Y},         PADDLEBOAT_BUTTON_DPAD_UP,
                    UIBUTTON_STATE_IDLE, true},
            {{AS_DPAD_LEFT_X,       AS_DPAD_LEFT_Y},       PADDLEBOAT_BUTTON_DPAD_LEFT,
                    UIBUTTON_STATE_IDLE, true},
            {{AS_DPAD_DOWN_X,       AS_DPAD_DOWN_Y},       PADDLEBOAT_BUTTON_DPAD_DOWN,
                    UIBUTTON_STATE_IDLE, true},
            {{AS_DPAD_RIGHT_X,      AS_DPAD_RIGHT_Y},      PADDLEBOAT_BUTTON_DPAD_RIGHT,
                    UIBUTTON_STATE_IDLE, true}
    };
}

ImVec2 ControllerUIData::getButtonQuadPosition(const ControllerUIButtons dpadButton) {
    switch (dpadButton) {
        case UIBUTTON_DPAD_UP:
            return ImVec2(BUTTON_QUAD_TOP_X, BUTTON_QUAD_TOP_Y);
        case UIBUTTON_DPAD_LEFT:
            return ImVec2(BUTTON_QUAD_LEFT_X, BUTTON_QUAD_LEFT_Y);
        case UIBUTTON_DPAD_DOWN:
            return ImVec2(BUTTON_QUAD_BOTTOM_X, BUTTON_QUAD_BOTTOM_Y);
        case UIBUTTON_DPAD_RIGHT:
            return ImVec2(BUTTON_QUAD_RIGHT_X, BUTTON_QUAD_RIGHT_Y);
        default:
            break;
    }
    return ImVec2(0.0f, 0.0f);
}

void ControllerUIData::LoadControllerUIData() {
    if (!uiDataInitialized) {
        for (int index = 0; index < UIBUTTON_COUNT; ++index) {
            buttonTextures[index].textureHandles[UIBUTTON_STATE_ACTIVE] =
                    TextureAssetLoader::loadTextureAsset(
                            buttonDefinitions[index].assetName_Active,
                            &buttonTextures[index].textureWidth,
                            &buttonTextures[index].textureHeight);
            buttonTextures[index].textureHandles[UIBUTTON_STATE_IDLE] =
                    TextureAssetLoader::loadTextureAsset(
                            buttonDefinitions[index].assetName_Idle, nullptr, nullptr);
        }

        stickTextures.textureHandles[UISTICK_STATE_ACTIVE] =
                TextureAssetLoader::loadTextureAsset(
                        stickDefinition.assetName_Active, &stickTextures.textureWidth,
                        &stickTextures.textureHeight);
        stickTextures.textureHandles[UISTICK_STATE_DEPRESSED] =
                TextureAssetLoader::loadTextureAsset(
                        stickDefinition.assetName_Depressed, nullptr, nullptr);
        stickTextures.textureHandles[UISTICK_STATE_IDLE] =
                TextureAssetLoader::loadTextureAsset(
                        stickDefinition.assetName_Idle, nullptr, nullptr);

        stickRegionTexture.textureHandles[0] = TextureAssetLoader::loadTextureAsset(
                stickRegionAssetName, &stickRegionTexture.textureWidth,
                &stickRegionTexture.textureHeight);
        uiDataInitialized = true;
    }
}

void ControllerUIData::UnloadControllerUIData() {
    for (int index = 0; index < UIBUTTON_COUNT; ++index) {
        TextureAssetLoader::unloadTextureAsset(
                buttonTextures[index].textureHandles[UIBUTTON_STATE_ACTIVE]);
        TextureAssetLoader::unloadTextureAsset(
                buttonTextures[index].textureHandles[UIBUTTON_STATE_IDLE]);
    }

    TextureAssetLoader::unloadTextureAsset(stickTextures.textureHandles[UISTICK_STATE_ACTIVE]);
    TextureAssetLoader::unloadTextureAsset(stickTextures.textureHandles[UISTICK_STATE_DEPRESSED]);
    TextureAssetLoader::unloadTextureAsset(stickTextures.textureHandles[UISTICK_STATE_IDLE]);

    TextureAssetLoader::unloadTextureAsset(stickRegionTexture.textureHandles[0]);
    uiDataInitialized = false;
}

const UITextureInfo &ControllerUIData::getUIButtonTextures(const ControllerUIButtons uiButton) {
    return buttonTextures[uiButton];
}

const UITextureInfo &ControllerUIData::getUIStickTextures() {
    return stickTextures;
}

const UITextureInfo &ControllerUIData::getUIStickRegionTexture() {
    return stickRegionTexture;
}

ControllerButtonInfo &
ControllerUIData::getControllerButtonInfo(const ControllerUIButtons uiButton) {
    return controllerButtonInfo[uiButton];
}

ControllerButtonInfo &
ControllerUIData::getControllerButtonInfo_ArcadeStick(const ControllerUIButtons uiButton) {
    return controllerButtonInfo_ArcadeStick[uiButton];
}

ImVec2 ControllerUIData::getStickPosition(bool isLeftStick) {
    if (isLeftStick) {
        return ImVec2(STICK_LEFT_X, STICK_BASE_Y);
    }
    return ImVec2(STICK_RIGHT_X, STICK_BASE_Y);
}

float ControllerUIData::getStickScale() {
    return STICK_SCALE;
}

void ControllerUIData::getTriggerRectExtents(const ControllerUIButtons uiButton, ImVec2 *rectMin,
                                             ImVec2 *rectMax) {
    if (rectMin != nullptr && rectMax != nullptr) {
        if (uiButton == UIBUTTON_L1) {
            rectMin->x = BUTTON_L1_X + TRIGGER_BAR_ADJUST_X;
            rectMin->y = BUTTON_L1_Y + TRIGGER_BAR_ADJUST_Y;
            rectMax->x = BUTTON_L1_X + TRIGGER_BAR_ADJUST_X + TRIGGER_BAR_WIDTH;
            rectMax->y = BUTTON_L1_Y + TRIGGER_BAR_ADJUST_Y + TRIGGER_BAR_HEIGHT;
        } else if (uiButton == UIBUTTON_L2) {
            rectMin->x = BUTTON_L2_X + TRIGGER_BAR_ADJUST_X;
            rectMin->y = BUTTON_L2_Y + TRIGGER_BAR_ADJUST_Y;
            rectMax->x = BUTTON_L2_X + TRIGGER_BAR_ADJUST_X + TRIGGER_BAR_WIDTH;
            rectMax->y = BUTTON_L2_Y + TRIGGER_BAR_ADJUST_Y + TRIGGER_BAR_HEIGHT;
        } else if (uiButton == UIBUTTON_R1) {
            rectMin->x = BUTTON_R1_X - TRIGGER_BAR_ADJUST_X - TRIGGER_BAR_WIDTH;
            rectMin->y = BUTTON_R1_Y + TRIGGER_BAR_ADJUST_Y;
            rectMax->x = BUTTON_R1_X - TRIGGER_BAR_ADJUST_X;
            rectMax->y = BUTTON_R1_Y + TRIGGER_BAR_ADJUST_Y + TRIGGER_BAR_HEIGHT;
        } else if (uiButton == UIBUTTON_R2) {
            rectMin->x = BUTTON_R2_X - TRIGGER_BAR_ADJUST_X - TRIGGER_BAR_WIDTH;
            rectMin->y = BUTTON_R2_Y + TRIGGER_BAR_ADJUST_Y;
            rectMax->x = BUTTON_R2_X - TRIGGER_BAR_ADJUST_X;
            rectMax->y = BUTTON_R2_Y + TRIGGER_BAR_ADJUST_Y + TRIGGER_BAR_HEIGHT;
        } else {
            rectMin->x = 0.0f;
            rectMin->y = 0.0f;
            rectMax->x = 0.0f;
            rectMax->y = 0.0f;
        }
    }
}
