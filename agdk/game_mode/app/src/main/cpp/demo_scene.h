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

// Header file for bullet physics.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra-semi"
#include "btBulletDynamicsCommon.h"
#pragma GCC diagnostic pop

#include "box_renderer.h"
#include "engine.h"
#include "swappy/swappyGL.h"
#include "swappy/swappyGL_extra.h"
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

  // Singleton function.
  static DemoScene* getInstance() {
    return instance_;
  }

  DemoScene();

  virtual ~DemoScene();

  virtual void OnStartGraphics();

  virtual void OnKillGraphics();

  virtual void OnInstall();

  virtual void OnUninstall();

  virtual void DoFrame();

  virtual void OnPointerDown(int pointerId, const struct PointerCoords* coords);

  virtual void OnPointerMove(int pointerId, const struct PointerCoords* coords);

  virtual void OnPointerUp(int pointerId, const struct PointerCoords* coords);

  virtual void OnScreenResized(int width, int height);

  static DemoScene* GetInstance();

  void AdaptThermalLevel(int32_t index);

  void ControlStep(bool step_up);
  void ControlBoxCount(bool count_up);
  void ControlResetToDefaultSettings();

 private:
  // # of cubes managed in bullet physics. Defaulted to 8
  static constexpr int32_t kArraySize = 8;
  static constexpr int32_t kArraySizeY = 8;
  static constexpr int32_t kArraySizeZ = 8;
  static constexpr int32_t kPhysicsStep = 8;

  static constexpr int32_t kPhysicsStepMax = 24;
  static constexpr int32_t kBoxSizeMin = 4;
  static constexpr int32_t kBoxSizeMax = 12;

  // Size of the box in the bullet physics.
  static constexpr float kBoxSize = 0.5f;

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
  void CreateRigidBodies();
  void DeleteRigidBodies();
  void CleanupPhysics();
  void UpdatePhysics();
  void ResetPhysics();

  int32_t currentTimeMillis();

  static void on_thermal_state_changed(int32_t last_state, int32_t current_state);

  // We want to register a touch down as the equivalent of
  // a button click to ImGui, so we send it an up event
  // without waiting for the touch to end
  enum SimulatedClickState {
    SIMULATED_CLICK_NONE = 0,
    SIMULATED_CLICK_DOWN,
    SIMULATED_CLICK_UP
  };

  static constexpr size_t MOTION_AXIS_COUNT = 3;

  static DemoScene* instance_;

  // Did we simulate a click for ImGui?
  SimulatedClickState simulated_click_state_;

  bool recreate_physics_obj_; // need to create obj on next tick

  // Is a touch pointer (a.k.a. finger) down at the moment?
  bool pointer_down_;

  // Touch pointer current X
  float point_x_;

  // Touch pointer current Y
  float pointer_y_;

  // Transition start time
  float transition_start_;

  // Renderer to render cubes.
  BoxRenderer box_;

  int32_t current_thermal_index_;

  // Current and target frame rate period.
  int32_t target_frame_period_;
  int32_t current_frame_period_;

  // time of last physics reset
  int32_t last_physics_reset_tick_;

  int32_t current_physics_step_;

  int32_t array_size_;

  float box_size_;

  // Bullet physics data members.
  btDefaultCollisionConfiguration* collision_configuration_;
  btCollisionDispatcher* dispatcher_;
  btDiscreteDynamicsWorld* dynamics_world_;
  btAlignedObjectArray<btCollisionShape*> collision_shapes_;
  btSequentialImpulseConstraintSolver* solver_;
  btBroadphaseInterface* overlapping_pair_cache_;
  btCollisionShape* box_collision_shape_;
};

#endif  // DEMO_SCENE_H_
