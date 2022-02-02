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

namespace {
    ImVec2 CalcPosFromCenter(const ImVec2 &basePos, const ImVec2 &texSize) {
        return ImVec2(basePos.x - (texSize.x * 0.5f), basePos.y - (texSize.y * 0.5f));
    }

    ImVec2 GetScaledBasePos(const ControllerUIPanelParams &panelParams, const ImVec2 &basePos) {
        return ImVec2((basePos.x * panelParams.panelImageScale) + panelParams.panelBaseX,
                      (basePos.y * panelParams.panelImageScale) + panelParams.panelBaseY);
    }

    ImVec2
    GetScaledTextureSize(const ControllerUIPanelParams &panelParams,
                         const UITextureInfo &texInfo) {
        return ImVec2(static_cast<float>(texInfo.textureWidth) * panelParams.panelImageScale,
                      static_cast<float>(texInfo.textureHeight) * panelParams.panelImageScale);
    }
}

void ControllerUIUtil::Button(const ControllerUIPanelParams &panelParams,
                              const ControllerUIButtons buttonId,
                              const ControllerButtonInfo &buttonInfo) {
    const UITextureInfo &buttonTextures = ControllerUIData::getUIButtonTextures(buttonId);
    if (buttonTextures.textureHandles[buttonInfo.buttonState] !=
        TextureAssetLoader::INVALID_TEXTURE) {
        const ImVec2 buttonBasePos = GetScaledBasePos(panelParams, buttonInfo.basePosition);
        const ImVec2 buttonTextureSize = GetScaledTextureSize(panelParams, buttonTextures);
        ImGui::SetCursorPos(CalcPosFromCenter(buttonBasePos, buttonTextureSize));
        ImGui::Image(
                reinterpret_cast<ImTextureID>(buttonTextures.textureHandles[buttonInfo.buttonState]),
                buttonTextureSize);
    }
}

void ControllerUIUtil::Thumbstick(const ControllerUIPanelParams &panelParams,
                                  const ImVec2 &basePos,
                                  const ImVec2 &stickVals,
                                  const ControllerUIStickStates stickState) {
    const UITextureInfo &stickRegionTexture = ControllerUIData::getUIStickRegionTexture();
    if (stickRegionTexture.textureHandles[0] != TextureAssetLoader::INVALID_TEXTURE) {
        const ImVec2 stickRegionBasePos = GetScaledBasePos(panelParams, basePos);
        const ImVec2 stickRegionTextureSize = GetScaledTextureSize(panelParams,
                                                                   stickRegionTexture);
        ImGui::SetCursorPos(CalcPosFromCenter(stickRegionBasePos, stickRegionTextureSize));
        ImGui::Image(reinterpret_cast<ImTextureID>(stickRegionTexture.textureHandles[0]),
                     stickRegionTextureSize);
    }

    const UITextureInfo stickTextures = ControllerUIData::getUIStickTextures();
    if (stickTextures.textureHandles[stickState] != TextureAssetLoader::INVALID_TEXTURE) {
        const ImVec2 stickTextureSize = GetScaledTextureSize(panelParams, stickTextures);
        const ImVec2 adjustedStickPos = GetScaledBasePos(panelParams,
                                                         ImVec2(basePos.x + stickVals.x,
                                                                basePos.y + stickVals.y));
        ImGui::SetCursorPos(CalcPosFromCenter(adjustedStickPos, stickTextureSize));
        ImGui::Image(reinterpret_cast<ImTextureID>(stickTextures.textureHandles[stickState]),
                     stickTextureSize);
    }
}

void ControllerUIUtil::TriggerBar(const ControllerUIPanelParams &panelParams,
                                  ImDrawList *draw_list,
                                  const ControllerUIButtons buttonId,
                                  const float triggerValue,
                                  const float offsetY) {
    const ImU32 frameColor = ImColor(255, 255, 255, 255);
    const ImU32 fillColor = ImColor(20, 255, 20, 255);
    ImVec2 rawFrameMin, rawFrameMax;
    ControllerUIData::getTriggerRectExtents(buttonId, &rawFrameMin, &rawFrameMax);
    ImVec2 frameMin = GetScaledBasePos(panelParams, rawFrameMin);
    ImVec2 frameMax = GetScaledBasePos(panelParams, rawFrameMax);
    frameMin.y += offsetY;
    frameMax.y += offsetY;
    const float frameWidth = frameMax.x - frameMin.x;
    const float fillAdjust = frameWidth * triggerValue;
    ImVec2 fillMin, fillMax;
    fillMin.y = frameMin.y;
    fillMax.y = frameMax.y;
    if (buttonId == UIBUTTON_L1 || buttonId == UIBUTTON_L2) {
        // Fill left to right
        fillMin.x = frameMin.x;
        fillMax.x = fillMin.x + fillAdjust;
        draw_list->AddRectFilled(fillMin, fillMax, fillColor);
        draw_list->AddRect(frameMin, frameMax, frameColor);
    } else if (buttonId == UIBUTTON_R1 || buttonId == UIBUTTON_R2) {
        // Fill right to left
        fillMax.x = frameMax.x;
        fillMin.x = fillMax.x - fillAdjust;
        draw_list->AddRectFilled(fillMin, fillMax, fillColor);
        draw_list->AddRect(frameMin, frameMax, frameColor);
    }
}
