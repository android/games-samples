/*
 * Copyright 2023 Google LLC
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

#ifndef BASEGAMEFRAMEWORK_GRAPHICSAPI_BASE_H_
#define BASEGAMEFRAMEWORK_GRAPHICSAPI_BASE_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include "display_manager.h"
#include "graphics_api_features.h"
#include "graphics_api_status.h"

namespace base_game_framework {

class GraphicsAPIBase {
 public:
  virtual DisplayManager::GraphicsAPI GetAPI() const = 0;
  virtual GraphicsAPIStatus GetAPIStatus() const { return kGraphicsAPI_Uninitialized; }
  virtual const GraphicsAPIFeatures& GetAPIFeatures() const = 0;

  virtual void QueryAvailability() = 0;

  virtual uint32_t GetFeatureFlags() const = 0;

  virtual bool InitializeGraphicsAPI() = 0;
  virtual void ShutdownGraphicsAPI() = 0;

  virtual bool SetDisplayChangedCallback(DisplayManager::DisplayChangedCallback callback,
                                         void *user_data) = 0;

  virtual bool SetSwapchainChangedCallback(DisplayManager::SwapchainChangedCallback callback,
                                                void* user_data) = 0;

  virtual void SwapchainChanged(const DisplayManager::SwapchainChangeMessage message) = 0;

  virtual DisplayManager::SwapchainConfigurations *GenerateSwapchainConfigurations() = 0;
  virtual DisplayManager::InitSwapchainResult InitSwapchain(
      const DisplayManager::DisplayFormat& display_format,
      const DisplayManager::DisplayResolution& display_resolution,
      const DisplayManager::DisplaySwapInterval display_swap_interval,
      const uint32_t swapchain_frame_count,
      const DisplayManager::SwapchainPresentMode present_mode) = 0;
  virtual void ShutdownSwapchain() = 0;
  virtual bool GetSwapchainValid() = 0;
  virtual DisplayManager::SwapchainFrameHandle GetCurrentSwapchainFrame() = 0;
  virtual DisplayManager::SwapchainFrameHandle PresentCurrentSwapchainFrame() = 0;

 protected:
  GraphicsAPIBase() {}
};

} // base_game_framework

#endif // BASEGAMEFRAMEWORK_GRAPHICSAPI_H_