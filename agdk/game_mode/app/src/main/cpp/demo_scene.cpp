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

#include "demo_scene.h"

#include <time.h>

#include <cassert>
#include <functional>

#include "Log.h"
#include "adpf_manager.h"
#include "game_mode_manager.h"
#include "imgui.h"
#include "imgui_manager.h"
#include "native_engine.h"

extern "C" {
#include <GLES2/gl2.h>
}

// String labels that represents thermal states.
const char* thermal_state_label[] = {
    "THERMAL_STATUS_NONE",     "THERMAL_STATUS_LIGHT",
    "THERMAL_STATUS_MODERATE", "THERMAL_STATUS_SEVERE",
    "THERMAL_STATUS_CRITICAL", "THERMAL_STATUS_EMERGENCY",
    "THERMAL_STATUS_SHUTDOWN"};

const int32_t thermal_state_physics_steps[] = {
    16,
    12,
    8,
    4,
};
const int32_t thermal_state_array_size[] = {
    8,
    6,
    4,
    2,
};

const int32_t kPhysicsResetTime = 10000;  // 10 sec

DemoScene* DemoScene::instance_ = NULL;

//--------------------------------------------------------------------------------
// Ctor
//--------------------------------------------------------------------------------
DemoScene::DemoScene() {
  simulated_click_state_ = SIMULATED_CLICK_NONE;
  pointer_down_ = false;
  point_x_ = 0.0f;
  pointer_y_ = 0.0f;
  transition_start_ = 0.0f;
  target_frame_period_ = SWAPPY_SWAP_60FPS;
  current_frame_period_ = SWAPPY_SWAP_60FPS;

  last_physics_reset_tick_ = currentTimeMillis();

  // Physics Dynamic adjustments
  current_physics_step_ = kPhysicsStep;
  array_size_ = kArraySize;
  box_size_ = kBoxSize;

  box_.Init();
  InitializePhysics();

  instance_ = this;

  ADPFManager::getInstance().SetThermalListener(on_thermal_state_changed);

  current_thermal_index_ = ADPFManager::getInstance().GetThermalStatus();
  AdaptThermalLevel(current_thermal_index_);
}

//--------------------------------------------------------------------------------
// Dtor
//--------------------------------------------------------------------------------
DemoScene::~DemoScene() {
  box_.Unload();
  CleanupPhysics();

  instance_ = NULL;

  ADPFManager::getInstance().SetThermalListener(NULL);
}

//--------------------------------------------------------------------------------
// Callbacks that manage demo scene's events.
//--------------------------------------------------------------------------------
void DemoScene::OnStartGraphics() {
  transition_start_ = Clock();

  // 2. Game State: Finish Loading, showing the attract screen which is not
  // interruptible
  GameModeManager::getInstance().SetGameState(
      false, GAME_STATE_GAMEPLAY_UNINTERRUPTIBLE);
}

void DemoScene::OnKillGraphics() {
  // 3. Game State: exiting, cleaning up and preparing to load the next scene
  GameModeManager::getInstance().SetGameState(true, GAME_STATE_NONE);
}

void DemoScene::OnInstall() {
  // 1. Game State: Start Loading
  GameModeManager::getInstance().SetGameState(true, GAME_STATE_NONE);
}

void DemoScene::OnUninstall() {
  // 4. Game State: Finished unloading this scene, it will be immediately
  // followed by loading the next scene
  GameModeManager::getInstance().SetGameState(false, GAME_STATE_UNKNOWN);
}

void DemoScene::OnScreenResized(int width, int height) {}

void DemoScene::on_thermal_state_changed(int32_t last_state,
                                         int32_t current_state) {
  if (last_state != current_state) {
    getInstance()->AdaptThermalLevel(current_state);
  }
}

void DemoScene::AdaptThermalLevel(int32_t index) {
  int32_t current_index = index;
  int32_t array_size = sizeof(thermal_state_physics_steps) /
                       sizeof(thermal_state_physics_steps[0]);
  if (current_index < 0) {
    current_index = 0;
  } else if (current_index >= array_size) {
    current_index = array_size - 1;
  }

  current_physics_step_ = thermal_state_physics_steps[current_index];
  array_size_ = thermal_state_array_size[current_index];

  recreate_physics_obj_ = true;
}

//--------------------------------------------------------------------------------
// Control the simulation parameters
//--------------------------------------------------------------------------------
void DemoScene::ControlStep(bool step_up) {
  if (step_up) {
    current_physics_step_ += kPhysicsStep;
    if (current_physics_step_ >= kPhysicsStepMax) {
      current_physics_step_ = kPhysicsStepMax;
    }
  } else {
    current_physics_step_ -= kPhysicsStep;
    if (current_physics_step_ < kPhysicsStep) {
      current_physics_step_ = kPhysicsStep;
    }
  }
}

void DemoScene::ControlBoxCount(bool count_up) {
  int32_t min_count = kBoxSizeMin;
  int32_t max_count = kBoxSizeMax;
  bool changed = false;

  if (count_up) {
    int32_t new_count = array_size_ + 1;
    if (new_count <= max_count) {
      changed = true;
      array_size_ = new_count;
    }
  } else {
    int32_t new_count = array_size_ - 1;
    if (new_count >= min_count) {
      changed = true;
      array_size_ = new_count;
    }
  }

  if (changed) {
    recreate_physics_obj_ = true;
  }
}

void DemoScene::ControlResetToDefaultSettings() {
  current_physics_step_ = kPhysicsStep;
  array_size_ = kArraySize;

  recreate_physics_obj_ = true;
}

//--------------------------------------------------------------------------------
// Process each frame's status updates.
// - Initiate the OpenGL rendering.
// - Monitor the device's thermal staus using ADPF API.
// - Update physics using BulletPhysics.
// - Render cubes.
// - Render UI using ImGUI (Show device's thermal status).
// - Tell the system of the samples workload using ADPF API.
//--------------------------------------------------------------------------------
void DemoScene::DoFrame() {
  // Tell ADPF manager beginning of the perf intensive task.
  ADPFManager::getInstance().BeginPerfHintSession();

  // clear screen
  glClearColor(0.0f, 0.0f, 0.25f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);

  // Update ADPF status.
  ADPFManager::getInstance().Monitor();

  UpdatePhysics();

  // Update UI inputs to ImGui before beginning a new frame
  UpdateUIInput();
  ImGuiManager* imguiManager = NativeEngine::GetInstance()->GetImGuiManager();
  imguiManager->BeginImGuiFrame();
  RenderUI();
  imguiManager->EndImGuiFrame();

  glEnable(GL_DEPTH_TEST);

  // Tell ADPF manager end of the perf intensive task.
  // The ADPF manager update PerfHintManager's session using
  // reportActualWorkDuration() and updateTargetWorkDuration() API.
  ADPFManager::getInstance().EndPerfHintSession(target_frame_period_);
}

//--------------------------------------------------------------------------------
// Render Background.
//--------------------------------------------------------------------------------
void DemoScene::RenderBackground() {
  // base classes override this to draw background
}

//--------------------------------------------------------------------------------
// Pointer and touch related implementations.
//--------------------------------------------------------------------------------
void DemoScene::OnPointerDown(int pointerId,
                              const struct PointerCoords* coords) {
  // If this event was generated by something that's not associated to the
  // screen, (like a trackpad), ignore it, because our UI is not driven that
  // way.
  if (coords->is_screen_) {
    pointer_down_ = true;
    point_x_ = coords->x_;
    pointer_y_ = coords->y_;
  }

  // Uncomment this block for debug control of the simulation
  /**
  // x: left to right
  // y: top to bottom
  ALOGI("PointerDown: %f, %f : (%f, %f)", point_x_, pointer_y_, coords->max_x_,
  coords->max_y_);
  //===============
  //   ||   ||   ||
  //===============
  //   ||   ||   ||
  //===============
  bool upper_touch = coords->y_ < coords->max_y_ / 2.0f;
  if ( coords->x_ < coords->max_x_ / 3.0f ) {
    // left, control steps
    ControlStep(upper_touch);
  } else if ( coords->x_ > ((2.0f * coords->max_x_) / 3.0f) ) {
    // right, control box count
    ControlBoxCount(upper_touch);
  } else {
    // middle, control recreate/respawn
    if ( upper_touch ) {
      ResetPhysics(); // respawn boxes
    } else {
      ControlResetToDefaultSettings();
    }
  }
  **/
}

void DemoScene::OnPointerMove(int pointerId,
                              const struct PointerCoords* coords) {
  if (coords->is_screen_ && pointer_down_) {
    point_x_ = coords->x_;
    pointer_y_ = coords->y_;
  }
}

void DemoScene::OnPointerUp(int pointerId, const struct PointerCoords* coords) {
  if (coords->is_screen_) {
    point_x_ = coords->x_;
    pointer_y_ = coords->y_;
    pointer_down_ = false;
    simulated_click_state_ = SIMULATED_CLICK_NONE;
  }
}

void DemoScene::UpdateUIInput() {
  ImGuiIO& io = ImGui::GetIO();
  io.MousePos = ImVec2(point_x_, pointer_y_);
  bool pointer_down = false;
  // To make a touch work like a mouse click we need to sequence the following:
  // 1) Position cursor at touch spot with mouse button still up
  // 2) Signal mouse button down for a frame
  // 3) Release mouse button (even if touch is still held down)
  // 4) Reset to allow another 'click' once the touch is released
  if (simulated_click_state_ == SIMULATED_CLICK_NONE && pointer_down_) {
    simulated_click_state_ = SIMULATED_CLICK_DOWN;
  } else if (simulated_click_state_ == SIMULATED_CLICK_DOWN) {
    pointer_down = true;
    simulated_click_state_ = SIMULATED_CLICK_UP;
  }
  io.MouseDown[0] = pointer_down;
}

//--------------------------------------------------------------------------------
// ImGUI related UI rendering.
//--------------------------------------------------------------------------------
void DemoScene::RenderUI() {
  SetupUIWindow();

  ImGui::End();
  ImGui::PopStyleVar();
}

void DemoScene::SetupUIWindow() {
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

void DemoScene::RenderPanel() {
  NativeEngine* native_engine = NativeEngine::GetInstance();
  SceneManager* scene_manager = SceneManager::GetInstance();
  GameModeManager& game_mode_manager = GameModeManager::getInstance();
  int32_t swap_interval = scene_manager->GetPreferredSwapInterval();
  int32_t thermal_state = ADPFManager::getInstance().GetThermalStatus();
  assert(thermal_state >= 0 &&
         thermal_state <
             sizeof(thermal_state_label) / sizeof(thermal_state_label[0]));

  // Show current thermal state on screen.
  // In this sample, no dynamic performance adjustment based on Thermal State
  // To see dynamic performance adjustment based on Thermal State see the ADPF
  // Sample
  ImGui::Text("Thermal State:%s", thermal_state_label[thermal_state]);
  ImGui::Text("Thermal Headroom:%f",
              ADPFManager::getInstance().GetThermalHeadroom());
  ImGui::Text("Physics Steps:%d", current_physics_step_);
  ImGui::Text("Array Size: %d", array_size_);

  // Show the stat changes according to selected Game Mode
  ImGui::Text("Game Mode: %s", game_mode_manager.GetGameModeString());
  ImGui::Text("FPS target: %s", game_mode_manager.GetFPSString(swap_interval));
  ImGui::Text("Surface size: %d x %d", native_engine->GetSurfaceWidth(),
              native_engine->GetSurfaceHeight());
  ImGui::Text("Preferred size: %d x %d", scene_manager->GetPreferredWidth(),
              scene_manager->GetPreferredHeight());
}

void DemoScene::OnButtonClicked(int buttonId) {
  // base classes override this to react to button clicks
}

//--------------------------------------------------------------------------------
// Initialize BulletPhysics engine.
//--------------------------------------------------------------------------------
void DemoScene::InitializePhysics() {
  // Initialize physics world.
  collision_configuration_ = new btDefaultCollisionConfiguration();
  dispatcher_ = new btCollisionDispatcher(collision_configuration_);
  overlapping_pair_cache_ = new btDbvtBroadphase();
  solver_ = new btSequentialImpulseConstraintSolver;
  dynamics_world_ = new btDiscreteDynamicsWorld(
      dispatcher_, overlapping_pair_cache_, solver_, collision_configuration_);
  dynamics_world_->setGravity(btVector3(0, -10, 0));

  /// create a few basic rigid bodies
  CreateRigidBodies();
}

//--------------------------------------------------------------------------------
// Create some RigidBodies (Ground and Boxes)
//--------------------------------------------------------------------------------
void DemoScene::CreateRigidBodies() {
  /// Create Ground
  // the ground is a cube of side 100 at position y = -56.
  // the sphere will hit it at y = -6, with center at -5
  btCollisionShape* ground_shape =
      new btBoxShape(btVector3(btScalar(50.), btScalar(50.), btScalar(50.)));

  collision_shapes_.push_back(ground_shape);

  btTransform groundTransform;
  groundTransform.setIdentity();
  groundTransform.setOrigin(btVector3(0, -56, 0));

  btScalar mass(0.);
  btVector3 local_inertia(0, 0, 0);

  // using motionstate is optional, it provides interpolation capabilities, and
  // only synchronizes 'active' objects
  btDefaultMotionState* myMotionState =
      new btDefaultMotionState(groundTransform);
  btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState,
                                                  ground_shape, local_inertia);
  btRigidBody* body = new btRigidBody(rbInfo);

  // add the body to the dynamics world
  dynamics_world_->addRigidBody(body);

  // create a dynamic rigidbody
  box_collision_shape_ =
      new btBoxShape(btVector3(box_size_, box_size_, box_size_));
  collision_shapes_.push_back(box_collision_shape_);

  /// Create Dynamic Objects
  int32_t array_size_x = array_size_;
  int32_t array_size_y = array_size_;
  int32_t array_size_z = array_size_;
  for (auto k = 0; k < array_size_y; k++) {
    for (auto i = 0; i < array_size_x; i++) {
      for (auto j = 0; j < array_size_z; j++) {
        // rigidbody is dynamic if and only if mass is non zero, otherwise
        // static
        btScalar mass_dynamic(1.f);
        box_collision_shape_->calculateLocalInertia(mass_dynamic,
                                                    local_inertia);

        btTransform start_transform;
        start_transform.setIdentity();
        start_transform.setOrigin(btVector3(
            btScalar((-box_size_ * array_size_x / 2) + box_size_ * 2.0 * i),
            btScalar(10 + box_size_ * k),
            btScalar((-box_size_ * array_size_z / 2) + box_size_ * 2.0 * j)));
        float angle = random();
        btQuaternion qt(btVector3(1, 1, 0), angle);
        start_transform.setRotation(qt);

        // using motionstate is recommended, it provides interpolation
        // capabilities, and only synchronizes 'active' objects
        btDefaultMotionState* motionState_dynamic =
            new btDefaultMotionState(start_transform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo_dynamic(
            mass_dynamic, motionState_dynamic, box_collision_shape_,
            local_inertia);
        btRigidBody* body_dynamic = new btRigidBody(rbInfo_dynamic);
        dynamics_world_->addRigidBody(body_dynamic);
      }
    }
  }

  recreate_physics_obj_ = false;
}

//--------------------------------------------------------------------------------
// Delete the RigidBodies (Ground and Boxes)
//--------------------------------------------------------------------------------
void DemoScene::DeleteRigidBodies() {
  // remove the rigidbodies from the dynamics world and delete them
  for (auto i = dynamics_world_->getNumCollisionObjects() - 1; i >= 0; i--) {
    btCollisionObject* obj = dynamics_world_->getCollisionObjectArray()[i];
    btRigidBody* body = btRigidBody::upcast(obj);
    if (body && body->getMotionState()) {
      delete body->getMotionState();
    }
    dynamics_world_->removeCollisionObject(obj);
    delete obj;
  }

  // delete collision shapes (2 elements: Ground shape & Box shape)
  for (auto j = 0; j < collision_shapes_.size(); j++) {
    btCollisionShape* shape = collision_shapes_[j];
    collision_shapes_[j] = 0;
    delete shape;
  }
}

//--------------------------------------------------------------------------------
// Update phycis world and render boxes.
//--------------------------------------------------------------------------------
void DemoScene::UpdatePhysics() {
  if (recreate_physics_obj_) {
    DeleteRigidBodies();
    CreateRigidBodies();
    ResetPhysics();
  }

  // In the sample, it's looping physics update here.
  // It's intended to add more CPU load to the system to achieve thermal
  // throttling status easily.
  // In the future version of the sample, this part will be update to
  // dynamically adjust the load.
  auto max_steps = current_physics_step_;
  for (auto steps = 0; steps < max_steps; ++steps) {
    dynamics_world_->stepSimulation(1.f / (60.f * max_steps), 10);
  }

  // Update box renderer.
  box_.BeginMultipleRender();

  // print positions of all objects
  auto box_color = 1;
  for (auto j = dynamics_world_->getNumCollisionObjects() - 1; j >= 0; j--) {
    btCollisionObject* obj = dynamics_world_->getCollisionObjectArray()[j];
    btRigidBody* body = btRigidBody::upcast(obj);
    btTransform trans;
    if (body && body->getMotionState()) {
      body->getMotionState()->getWorldTransform(trans);
    } else {
      trans = obj->getWorldTransform();
    }
    auto shape = body->getCollisionShape();
    auto name = shape->getName();
    if (!strcmp(name, "Box")) {
      auto p = reinterpret_cast<btBoxShape*>(shape);
      auto size = p->getHalfExtentsWithoutMargin();

      // Render boxes
      btScalar m[16];
      trans.getOpenGLMatrix(m);

      // Change the box color (expecting BulletPhysics's object iteration order
      // doesn't change).
      auto c = (box_color % 7 + 1);
      float color[3] = {((c & 0x1) != 0) * 1.f, ((c & 0x2) != 0) * 1.f,
                        ((c & 0x4) != 0) * 1.f};
      box_.RenderMultiple(m, size.getX() * 2, size.getY() * 2, size.getZ() * 2,
                          color);
      box_color++;
    }
  }

  box_.EndMultipleRender();

  // Reset a physics each kPhysicsResetTime sec (independent of frame rate)
  int32_t currentTime = currentTimeMillis();
  int32_t elapsedTime = currentTime - last_physics_reset_tick_;
  if (elapsedTime > kPhysicsResetTime) {
    ResetPhysics();
  }
}

int32_t DemoScene::currentTimeMillis() {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return 1000.0 * t.tv_sec + (double)t.tv_nsec / 1e6;
}

//--------------------------------------------------------------------------------
// Helper to reset the physics world. (RespawnBoxes)
//--------------------------------------------------------------------------------
void DemoScene::ResetPhysics() {
  // keep track of last reset time
  int32_t currentTime = currentTimeMillis();
  last_physics_reset_tick_ = currentTime;

  // Reset physics state.
  auto k = 0;
  auto i = 0;
  auto j = 0;
  int32_t array_size_x = array_size_;
  int32_t array_size_y = array_size_;
  int32_t array_size_z = array_size_;
  for (auto index = dynamics_world_->getNumCollisionObjects() - 1; index >= 0;
       index--) {
    btCollisionObject* obj = dynamics_world_->getCollisionObjectArray()[index];
    btRigidBody* body = btRigidBody::upcast(obj);
    if (body) {
      auto shape = body->getCollisionShape();
      auto name = shape->getName();
      if (!strcmp(name, "Box")) {
        auto p = reinterpret_cast<btBoxShape*>(shape);
        auto size = p->getHalfExtentsWithoutMargin();
        auto stage_size_threshold = 30.f;
        if (size.getX() < stage_size_threshold) {
          // Reset position.
          btTransform transform;
          transform.setIdentity();
          transform.setOrigin(btVector3(
              btScalar((-box_size_ * box_size_ / 2) + box_size_ * 2.0 * i),
              btScalar(10 + box_size_ * k),
              btScalar((-box_size_ * array_size_z / 2) + box_size_ * 2.0 * j)));
          float angle = random();
          btQuaternion qt(btVector3(1, 1, 0), angle);
          transform.setRotation(qt);

          body->setWorldTransform(transform);
          body->setActivationState(DISABLE_DEACTIVATION);

          // Update cube's index.
          j = (j + 1) % array_size_z;
          if (!j) {
            i = (i + 1) % array_size_x;
            if (!i) {
              k = (k + 1) % array_size_y;
            }
          }
        }
      }
    }
  }
  dynamics_world_->setForceUpdateAllAabbs(true);
}

//--------------------------------------------------------------------------------
// Clean up bullet physics data.
//--------------------------------------------------------------------------------
void DemoScene::CleanupPhysics() {
  DeleteRigidBodies();

  // delete dynamics world
  delete dynamics_world_;

  // delete solver
  delete solver_;

  // delete broadphase
  delete overlapping_pair_cache_;

  // delete dispatcher
  delete dispatcher_;

  delete collision_configuration_;

  // next line is optional: it will be cleared by the destructor when the array
  // goes out of scope
  collision_shapes_.clear();
}
