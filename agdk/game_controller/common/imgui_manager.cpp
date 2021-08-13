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

extern "C" {
#include <EGL/egl.h>
#include <GLES2/gl2.h>
}

#include "imgui_manager.hpp"
#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"

namespace {
    const float GUI_LOWDPI_FONT_SCALE = 2.0f;
    const float GUI_DEFAULT_FONT_SCALE = 5.0f;
    const float GUI_MINIMUM_FRAME_TIME = (1.0f / 60.0f);
    float currentFontScale = 1.0f;
    bool overrideFontScale = false;
}

ImGuiManager::ImGuiManager() : mDeltaClock() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplOpenGL3_Init(NULL);
}

ImGuiManager::~ImGuiManager() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiManager::SetDisplaySize(const int displayWidth, const int displayHeight,
                                  const int displayDpi) {
    // Make sure the internal display size matches current
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float) displayWidth, (float) displayHeight);
    if (!overrideFontScale) {
        const float displayScale = (displayWidth >= 1920 && displayDpi >= 400)
                                   ? GUI_DEFAULT_FONT_SCALE : GUI_LOWDPI_FONT_SCALE;
        currentFontScale = displayScale;
        io.FontGlobalScale = displayScale;
    }
}

void ImGuiManager::BeginImGuiFrame() {
    // Update the delta time since the last frame
    float deltaTime = mDeltaClock.ReadDelta();
    // Don't return a delta time of less than a 60Hz tick
    if (deltaTime < GUI_MINIMUM_FRAME_TIME) {
        deltaTime = GUI_MINIMUM_FRAME_TIME;
    }
    ImGuiIO &io = ImGui::GetIO();
    io.DeltaTime = deltaTime;

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::EndImGuiFrame() {
    ImGui::Render();
    ImGuiIO &io = ImGui::GetIO();
    glViewport(0, 0, (int) io.DisplaySize.x, (int) io.DisplaySize.y);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

float ImGuiManager::GetFontScale() {
    return currentFontScale;
}

void ImGuiManager::SetFontScale(const float fontScale) {
    currentFontScale = fontScale;
    overrideFontScale = true;
    ImGuiIO &io = ImGui::GetIO();
    io.FontGlobalScale = fontScale;
}
