/*
 * Copyright 2022 The Android Open Source Project
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

#include "welcome_scene.h"

#include <time.h>

#include <cassert>
#include <functional>

#include "Log.h"
#include "adpf_manager.h"
#include "demo_scene.h"
#include "game_mode_manager.h"
#include "imgui.h"
#include "imgui_manager.h"
#include "native_engine.h"

extern "C" {
#include <GLES2/gl2.h>
}

WelcomeScene* WelcomeScene::instance_ = NULL;

//--------------------------------------------------------------------------------
// Ctor
//--------------------------------------------------------------------------------
WelcomeScene::WelcomeScene() { instance_ = this; }

//--------------------------------------------------------------------------------
// Dtor
//--------------------------------------------------------------------------------
WelcomeScene::~WelcomeScene() {}

//--------------------------------------------------------------------------------
// Callbacks that manage welcome scene's events.
//--------------------------------------------------------------------------------
// Install => StartGraphics => KillGraphics => Uninstall
void WelcomeScene::OnInstall() {
  // 1. Game State: Start Loading
  GameModeManager::getInstance().SetGameState(true, GAME_STATE_NONE);
}

void WelcomeScene::OnStartGraphics() {
  // 2. Game State: Finish Loading, showing the attract screen which is
  // interruptible
  GameModeManager::getInstance().SetGameState(
      false, GAME_STATE_GAMEPLAY_INTERRUPTIBLE);
}

void WelcomeScene::OnKillGraphics() {
  // 3. Game State: exiting, cleaning up and preparing to load the next scene
  GameModeManager::getInstance().SetGameState(true, GAME_STATE_NONE);
}

void WelcomeScene::OnUninstall() {
  // 4. Game State: Finished unloading this scene, it will be immediately
  // followed by loading the next scene
  GameModeManager::getInstance().SetGameState(false, GAME_STATE_UNKNOWN);
}

void WelcomeScene::OnScreenResized(int width, int height) {}

//--------------------------------------------------------------------------------
// Process each frame's status updates.
// - Initiate the OpenGL rendering.
// - Monitor the device's thermal staus using ADPF API.
// - Update physics using BulletPhysics.
// - Render cubes.
// - Render UI using ImGUI (Show device's thermal status).
// - Tell the system of the samples workload using ADPF API.
//--------------------------------------------------------------------------------
void WelcomeScene::DoFrame() {
  // clear screen
  glClearColor(0.8588f, 0.2666f, 0.2156f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);

  ImGuiManager* imguiManager = NativeEngine::GetInstance()->GetImGuiManager();
  imguiManager->BeginImGuiFrame();
  RenderUI();
  imguiManager->EndImGuiFrame();

  glEnable(GL_DEPTH_TEST);
}

//--------------------------------------------------------------------------------
// Pointer and touch related implementations.
//--------------------------------------------------------------------------------
void WelcomeScene::OnPointerDown(int pointerId,
                                 const struct PointerCoords* coords) {
  SceneManager* mgr = SceneManager::GetInstance();
  mgr->RequestNewScene(new DemoScene());
}

void WelcomeScene::OnPointerMove(int pointerId,
                                 const struct PointerCoords* coords) {}

void WelcomeScene::OnPointerUp(int pointerId,
                               const struct PointerCoords* coords) {}

//--------------------------------------------------------------------------------
// ImGUI related UI rendering.
//--------------------------------------------------------------------------------
void WelcomeScene::RenderUI() {
  SetupUIWindow();

  ImGui::End();
  ImGui::PopStyleVar();
}

void WelcomeScene::SetupUIWindow() {
  ImGuiIO& io = ImGui::GetIO();
  const float windowStartY = NativeEngine::GetInstance()->GetSystemBarOffset();
  ImVec2 windowPosition(0.0f, windowStartY);
  ImVec2 minWindowSize(io.DisplaySize.x * 0.95f, io.DisplaySize.y);
  ImVec2 maxWindowSize = io.DisplaySize;
  ImGui::SetNextWindowPos(windowPosition);
  ImGui::SetNextWindowSizeConstraints(minWindowSize, maxWindowSize, NULL, NULL);
  ImGuiWindowFlags windowFlags =
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 32.0f);
  char titleString[64];
  snprintf(titleString, 64, "Game Mode Sample");
  ImGui::Begin(titleString, NULL, windowFlags);

  RenderPanel();
}

void WelcomeScene::RenderPanel() {
  NativeEngine* native_engine = NativeEngine::GetInstance();
  SceneManager* scene_manager = SceneManager::GetInstance();
  GameModeManager& game_mode_manager = GameModeManager::getInstance();

  // Show the stat changes according to selected Game Mode
  ImGui::Text("Game Mode: %s", game_mode_manager.GetGameModeString());
  ImGui::Text("Surface size: %d x %d", native_engine->GetSurfaceWidth(),
              native_engine->GetSurfaceHeight());
  ImGui::Text("Preferred size: %d x %d", scene_manager->GetPreferredWidth(),
              scene_manager->GetPreferredHeight());

  const char* instruction_text = "Touch Anywhere To Continue";
  ImVec2 text_size = ImGui::CalcTextSize(instruction_text);
  ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - text_size.x) * 0.5f,
                             ImGui::GetWindowSize().y * 0.5f));
  ImGui::TextUnformatted(instruction_text);
}
