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

#ifndef BASEGAMEFRAMEWORK_GRAPHICSAPI_GLES_H_
#define BASEGAMEFRAMEWORK_GRAPHICSAPI_GLES_H_

#include <cstdint>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <functional>
#include <memory>
#include <vector>

#include "graphics_api_base.h"
#include "graphics_api_status.h"
#include "graphics_api_gles_resources.h"

namespace base_game_framework {

class GraphicsAPIGLES : public GraphicsAPIBase {
 public:
  GraphicsAPIGLES();
  virtual ~GraphicsAPIGLES();

  GraphicsAPIGLES(const GraphicsAPIGLES &) = delete;
  GraphicsAPIGLES& operator=(const GraphicsAPIGLES &) = delete;

  virtual DisplayManager::GraphicsAPI GetAPI() const { return DisplayManager::kGraphicsAPI_GLES; }
  virtual GraphicsAPIStatus GetAPIStatus() const { return api_status_; }
  virtual const GraphicsAPIFeatures& GetAPIFeatures() const { return api_features_; }

  virtual uint32_t GetFeatureFlags() const { return feature_flags_; }

  virtual void QueryAvailability();

  virtual bool InitializeGraphicsAPI();
  virtual void ShutdownGraphicsAPI();

  virtual bool SetDisplayChangedCallback(DisplayManager::DisplayChangedCallback callback,
                                         void* user_data);

  virtual bool SetSwapchainChangedCallback(DisplayManager::SwapchainChangedCallback callback,
                                           void* user_data);
  virtual void SwapchainChanged(const DisplayManager::SwapchainChangeMessage message);

  virtual DisplayManager::SwapchainConfigurations *GenerateSwapchainConfigurations();
  virtual DisplayManager::InitSwapchainResult InitSwapchain(
                                    const DisplayManager::DisplayFormat& display_format,
                                    const DisplayManager::DisplayResolution& display_resolution,
                                    const DisplayManager::DisplaySwapInterval display_swap_interval,
                                    const uint32_t swapchain_frame_count,
                                    const DisplayManager::SwapchainPresentMode present_mode);
  virtual void ShutdownSwapchain();
  virtual bool GetSwapchainValid();
  virtual DisplayManager::SwapchainFrameHandle GetCurrentSwapchainFrame();
  virtual DisplayManager::SwapchainFrameHandle PresentCurrentSwapchainFrame();

  bool GetGraphicsAPIResourcesGLES(GraphicsAPIResourcesGLES& api_resources_gles);

  bool GetSwapchainFrameResourcesGLES(const DisplayManager::SwapchainFrameHandle frame_handle,
                                      SwapchainFrameResourcesGLES& frame_resources);

  void LostSurfaceGLES();
  void RestoreSurfaceGLES();
 private:
  void QueryCapabilities();
  void ParseEGLConfigs();
  EGLDisplay InitializeEGLDisplay();
  EGLSurface InitializeEGLSurface(const DisplayManager::DisplayFormat& display_format);
  EGLContext InitializeEGLContext();
  bool DisplayFormatExists(const DisplayManager::DisplayFormat& display_format);
  const EGLint *GenerateEGLConfigAttribList(const DisplayManager::DisplayFormat& display_format);

  std::vector<DisplayManager::DisplayFormat> display_formats_;
  std::vector<DisplayManager::DisplayResolution> display_resolutions_;
  std::vector<DisplayManager::DisplaySwapInterval> swap_intervals_;

  GraphicsAPIStatus api_status_;
  GraphicsAPIFeatures api_features_;
  DisplayManager::DisplayChangedCallback display_changed_callback_;
  void *display_changed_user_data_;
  DisplayManager::SwapchainChangedCallback swapchain_changed_callback_ = nullptr;
  void* swapchain_changed_user_data_ = nullptr;
  DisplayManager::GLESFeatureFlags feature_flags_;
  DisplayManager::DisplayFormat swapchain_format_;
  DisplayManager::DisplayResolution swapchain_resolution_;
  DisplayManager::DisplaySwapInterval swapchain_interval_;
  uint32_t swapchain_frame_count_;
  DisplayManager::SwapchainPresentMode swapchain_present_mode_;
  EGLConfig egl_config_;
  EGLDisplay egl_display_;
  EGLSurface egl_surface_;
  EGLContext egl_context_;
  bool srgb_framebuffer_support_;

  static constexpr const char* BGM_CLASS_TAG = "BGF::GraphicsAPIGLES";
};

}

#endif //BASEGAMEFRAMEWORK_GRAPHICSAPI_GLES_H_
