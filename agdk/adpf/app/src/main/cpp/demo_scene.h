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

#ifndef DEMO_SCENE_H_
#define DEMO_SCENE_H_

#include "imgui.h"
#include "imgui_manager.h"
#include "implot.h"

// Header file for bullet physics.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra-semi"
#include "btBulletDynamicsCommon.h"
#pragma GCC diagnostic pop

#include "box_renderer.h"
#include "engine.h"
#include "swappy/swappyGL.h"
#include "util.h"

class GameAssetManager;

// Basic scene implementation for our demo UI display.
// In the scene, it's using BulletPhysics to update bulk cubes dynamically
// and render them using BoxRenderer.
// It also monitors the device's thermal throttling status using ADPF APIs.
// In the future version, the sample is update to adjust the CPU/GPU load
// based on the thermal status of the device.
class DemoScene : public Scene {
 public:
  DemoScene();

  virtual ~DemoScene();

  virtual void OnStartGraphics();

  virtual void OnKillGraphics();

  virtual void DoFrame();

  virtual void OnPointerDown(int pointerId, const struct PointerCoords* coords);

  virtual void OnPointerMove(int pointerId, const struct PointerCoords* coords);

  virtual void OnPointerUp(int pointerId, const struct PointerCoords* coords);

  virtual void OnScreenResized(int width, int height);

 private:
  // # of cubes managed in bullet physics.
  static constexpr int32_t kArraySizeX = 6;
  static constexpr int32_t kArraySizeY = 6;
  static constexpr int32_t kArraySizeZ = 6;

  // Size of the box in the bullet physics.
  static constexpr float kBoxSize = 1.0f;

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

  // Bullet Physics related methods.
  void InitializePhysics();
  void CleanupPhysics();
  void UpdatePhysics();
  void ResetPhysics();

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
  SimulatedClickState simulated_click_state_;

  // Is a touch pointer (a.k.a. finger) down at the moment?
  bool pointer_down_;

  // Touch pointer current X
  float pointer_x_;

  // Touch pointer current Y
  float pointer_y_;

  // Transition start time
  float transition_start_;

  // Renderer to render cubes.
  BoxRenderer box_;

  // Current and target frame rate period.
  int32_t target_frame_period_;
  int32_t current_frame_period_;

  // ImGUI buffer
  ScrollingBuffer graph_buffer_;
  ScrollingBuffer graph_buffer_forecast1_;
  ScrollingBuffer graph_buffer_forecast2_;
  ScrollingBuffer graph_buffer_forecast3_;

  ScrollingBuffer graph_buffer_power1_;
  ScrollingBuffer graph_buffer_power2_;

  // Bullet physics data members.
  btDefaultCollisionConfiguration* collision_configuration_;
  btCollisionDispatcher* dispatcher_;
  btDiscreteDynamicsWorld* dynamics_world_;
  btAlignedObjectArray<btCollisionShape*> collision_shapes_;
  btSequentialImpulseConstraintSolver* solver_;
  btBroadphaseInterface* overlapping_pair_cache_;
};

#endif  // DEMO_SCENE_H_
