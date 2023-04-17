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

#ifndef WELCOME_SCENE_H_
#define WELCOME_SCENE_H_

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
class WelcomeScene : public Scene {
 public:

  // Singleton function.
  static WelcomeScene* getInstance() {
    return instance_;
  }

  WelcomeScene();

  virtual ~WelcomeScene();

  virtual void OnStartGraphics();

  virtual void OnKillGraphics();

  virtual void OnInstall();

  virtual void OnUninstall();

  virtual void DoFrame();

  virtual void OnPointerDown(int pointerId, const struct PointerCoords* coords);

  virtual void OnPointerMove(int pointerId, const struct PointerCoords* coords);

  virtual void OnPointerUp(int pointerId, const struct PointerCoords* coords);

  virtual void OnScreenResized(int width, int height);

 private:

    static WelcomeScene* instance_;

  // UI rendering functions
  void RenderUI();
  void SetupUIWindow();
  void RenderPanel();
};

#endif  // WELCOME_SCENE_H_
