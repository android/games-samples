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

#include "display_manager.h"
#if defined BGF_DISPLAY_MANAGER_GLES
#include "gles/graphics_api_gles.h"
#endif
#if defined BGF_DISPLAY_MANAGER_VULKAN
#include "vulkan/graphics_api_vulkan.h"
#endif

namespace base_game_framework {

// We currently only support one swapchain, but we still provide a handle,
// to facilitate multiple swapchain support
static constexpr DisplayManager::SwapchainHandle kDefault_swapchain_handle = 123;

static constexpr uint64_t kSwap_interval_constants[] = {
    DisplayManager::kDisplay_Swap_Interval_165FPS,
    DisplayManager::kDisplay_Swap_Interval_120FPS,
    DisplayManager::kDisplay_Swap_Interval_90FPS,
    DisplayManager::kDisplay_Swap_Interval_60FPS,
    DisplayManager::kDisplay_Swap_Interval_45FPS,
    DisplayManager::kDisplay_Swap_Interval_30FPS,
    0L
};

std::unique_ptr <DisplayManager> DisplayManager::instance_ = nullptr;

const uint64_t *DisplayManager::GetSwapIntervalConstants() {
  return kSwap_interval_constants;
}

DisplayManager& DisplayManager::GetInstance() {
  if (!instance_) {
    instance_ = std::unique_ptr<DisplayManager>(new DisplayManager());
  }
  return *instance_;
}

void DisplayManager::ShutdownInstance() {
  DisplayManager::instance_.reset();
}

DisplayManager::DisplayManager()
    : active_api_(kGraphicsAPI_None)
    , api_(nullptr)
    , buffer_mode_(kDisplay_Double_Buffer) {
#if defined BGF_DISPLAY_MANAGER_GLES
  api_gles_ = std::make_shared<GraphicsAPIGLES>();
#endif
#if defined BGF_DISPLAY_MANAGER_VULKAN
  api_vulkan_ = std::make_shared<GraphicsAPIVulkan>();
#endif
}

DisplayManager::~DisplayManager() {

}

uint32_t DisplayManager::GetGraphicsAPISupportFlags(const GraphicsAPI api) {
  uint32_t returnFlags = kGraphics_API_Unsupported;

#if defined BGF_DISPLAY_MANAGER_GLES
  if (api == kGraphicsAPI_GLES) {
    switch (api_gles_->GetAPIStatus()) {
      case kGraphicsAPI_Uninitialized:
        break;
      case kGraphicsAPI_ObtainingAvailability:
        api_gles_->QueryAvailability();
        returnFlags = kGraphics_API_Waiting;
        break;
      case kGraphicsAPI_AvailabilityReady:
      case kGraphicsAPI_Active:
        returnFlags = api_gles_->GetFeatureFlags();
        break;
      default: break;
    }
  }
#endif
#if defined BGF_DISPLAY_MANAGER_VULKAN
  if (api == kGraphicsAPI_Vulkan) {
    switch (api_vulkan_->GetAPIStatus()) {
      case kGraphicsAPI_Uninitialized:break;
      case kGraphicsAPI_ObtainingAvailability:api_vulkan_->QueryAvailability();
        returnFlags = kGraphics_API_Waiting;
        break;
      case kGraphicsAPI_AvailabilityReady:
      case kGraphicsAPI_Active:returnFlags = api_vulkan_->GetFeatureFlags();
        break;
      default: break;
    }
  }
#endif


  return returnFlags;
}

DisplayManager::InitGraphicsAPIResult DisplayManager::InitGraphicsAPI(const GraphicsAPI api,
                                                      const uint32_t requested_features) {
  if (active_api_ != kGraphicsAPI_None) {
    return kInit_GraphicsAPI_Failure_Already_Initialized;
  }

  switch (api) {
#if defined BGF_DISPLAY_MANAGER_GLES
    case kGraphicsAPI_GLES:
      api_ = api_gles_;
      break;
#endif // BGF_DISPLAY_MANAGER_GLES
#if defined BGF_DISPLAY_MANAGER_VULKAN
    case kGraphicsAPI_Vulkan:
      api_ = api_vulkan_;
      break;
#endif // BGF_DISPLAY_MANAGER_VULKAN
    default:
      return kInit_GraphicsAPI_Failure_Unsupported;
  }
  const uint32_t api_features = GetGraphicsAPISupportFlags(api);
  DisplayManager::InitGraphicsAPIResult result = kInit_GraphicsAPI_Failure_Unsupported;

  if (api_features == kGraphics_API_Waiting) {
    result = kInit_GraphicsAPI_Failure_Not_Ready;
  } else if (api_features != kGraphics_API_Unsupported) {
    if (api_->GetAPIStatus() == kGraphicsAPI_Active) {
      result = kInit_GraphicsAPI_Failure_Already_Initialized;
    } else {
      if ((requested_features & api_features) != requested_features) {
        result = kInit_GraphicsAPI_Failure_Missing_Feature;
      } else {
        if (api_->InitializeGraphicsAPI()) {
          result = kInit_GraphicsAPI_Success;
          active_api_ = api;
        } else {
          result = kInit_GraphicsAPI_Failure_Initialization;
          api_ = nullptr;
        }
      }
    }
  }
  return result;
}

void DisplayManager::ShutdownGraphicsAPI() {
  if (active_api_ != kGraphicsAPI_None && api_->GetAPIStatus() == kGraphicsAPI_Active) {
    api_->ShutdownGraphicsAPI();
    active_api_ = kGraphicsAPI_None;
    api_ = nullptr;
  }
}

const GraphicsAPIFeatures& DisplayManager::GetGraphicsAPIFeatures() {
  if (active_api_ != kGraphicsAPI_None && api_->GetAPIStatus() == kGraphicsAPI_Active) {
    return api_->GetAPIFeatures();
  }
  return null_features_;
}

uint32_t DisplayManager::GetDisplayCount() {
  // TODO: only one display right now
  return 1;
}

DisplayManager::DisplayId DisplayManager::GetDisplayId(const uint32_t /*display_index*/) {
  return DisplayManager::kDefault_Display;
}

std::unique_ptr <DisplayManager::SwapchainConfigurations>
    DisplayManager::GetSwapchainConfigurations(const DisplayId /*display_id*/) {
  if (active_api_ != kGraphicsAPI_None && api_->GetAPIStatus() == kGraphicsAPI_Active) {
    return std::unique_ptr<DisplayManager::SwapchainConfigurations>(
        api_->GenerateSwapchainConfigurations());
  }
  return nullptr;
}

DisplayManager::InitSwapchainResult DisplayManager::InitSwapchain(
                                                  const DisplayFormat& display_format,
                                                  const DisplayResolution& display_resolution,
                                                  const DisplaySwapInterval display_swap_interval,
                                                  const uint32_t swapchain_frame_count,
                                                  const SwapchainPresentMode present_mode,
                                                  const DisplayId display_id,
                                                  SwapchainHandle* swapchain_handle) {
  InitSwapchainResult result = kInit_Swapchain_Failure;
  if (display_id != kDefault_Display) {
    // Multiple display support is a TODO: require the default constant right now
    return kInit_Swapchain_Invalid_DisplayId;
  }
  if (active_api_ != kGraphicsAPI_None && api_->GetAPIStatus() == kGraphicsAPI_Active &&
      swapchain_handle != nullptr) {
    result = api_->InitSwapchain(display_format, display_resolution, display_swap_interval,
                                 swapchain_frame_count, present_mode);
    if (result == kInit_Swapchain_Success) {
      *swapchain_handle = kDefault_swapchain_handle;
    }
  }
  return result;
}

bool DisplayManager::GetSwapchainValid(const SwapchainHandle swapchain_handle) {
  if (active_api_ != kGraphicsAPI_None && api_->GetAPIStatus() == kGraphicsAPI_Active &&
      swapchain_handle == kDefault_swapchain_handle) {
    return api_->GetSwapchainValid();
  }
  return false;
}

void DisplayManager::ShutdownSwapchain(const SwapchainHandle swapchain_handle) {
  if (GetSwapchainValid(swapchain_handle)) {
    api_->ShutdownSwapchain();
  }
}

DisplayManager::SwapchainFrameHandle DisplayManager::GetCurrentSwapchainFrame(
    const SwapchainHandle swapchain_handle) {
  if (GetSwapchainValid(swapchain_handle)) {
    return api_->GetCurrentSwapchainFrame();
  }
  return kInvalid_swapchain_handle;
}

DisplayManager::SwapchainFrameHandle DisplayManager::PresentCurrentSwapchainFrame(
    const SwapchainHandle swapchain_handle) {
  if (GetSwapchainValid(swapchain_handle)) {
    return api_->PresentCurrentSwapchainFrame();
  }
  return kInvalid_swapchain_handle;
}

#if defined BGF_DISPLAY_MANAGER_GLES
bool DisplayManager::GetGraphicsAPIResourcesGLES(GraphicsAPIResourcesGLES& api_resources_gles) {
  if (active_api_ == kGraphicsAPI_GLES) {
    if (api_gles_->GetAPIStatus() == kGraphicsAPI_Active) {
      return api_gles_->GetGraphicsAPIResourcesGLES(api_resources_gles);
    }
  }
  return false;
}

bool DisplayManager::GetSwapchainFrameResourcesGLES(const SwapchainFrameHandle frame_handle,
    SwapchainFrameResourcesGLES& frame_resources) {
  if (frame_handle != kInvalid_swapchain_handle) {
    if (active_api_ == kGraphicsAPI_GLES) {
      if (api_gles_->GetAPIStatus() == kGraphicsAPI_Active) {
        return api_gles_->GetSwapchainFrameResourcesGLES(frame_handle, frame_resources);
      }
    }
  }
  return false;
}
#endif // BGF_DISPLAY_MANAGER_GLES

#if defined BGF_DISPLAY_MANAGER_VULKAN
bool DisplayManager::GetGraphicsAPIResourcesVk(GraphicsAPIResourcesVk& api_resources_vk) {
  if (active_api_ == kGraphicsAPI_Vulkan) {
    if (api_vulkan_->GetAPIStatus() == kGraphicsAPI_Active) {
      return api_vulkan_->GetGraphicsAPIResourcesVk(api_resources_vk);
    }
  }
  return false;
}

bool DisplayManager::GetSwapchainFrameResourcesVk(const SwapchainFrameHandle frame_handle,
    SwapchainFrameResourcesVk& frame_resources, bool acquire_frame_image) {
  if (frame_handle != kInvalid_swapchain_handle) {
    if (active_api_ == kGraphicsAPI_Vulkan) {
      if (api_vulkan_->GetAPIStatus() == kGraphicsAPI_Active) {
        return api_vulkan_->GetSwapchainFrameResourcesVk(frame_handle, frame_resources,
                                                         acquire_frame_image);
      }
    }
  }
  return false;
}
#endif // BGF_DISPLAY_MANAGER_VULKAN

DisplayManager::SwapchainRotationMode DisplayManager::GetSwapchainRotationMode(
    const SwapchainHandle swapchain_handle) {
#if defined BGF_DISPLAY_MANAGER_VULKAN
  if (swapchain_handle != kInvalid_swapchain_handle) {
    if (active_api_ == kGraphicsAPI_Vulkan) {
      if (api_vulkan_->GetAPIStatus() == kGraphicsAPI_Active) {
        return api_vulkan_->GetSwapchainRotationMode();
      }
    }
  }
#endif
  return DisplayManager::kSwapchain_Rotation_None;
}

bool DisplayManager::SetDisplayChangedCallback(DisplayChangedCallback callback,
                                               void* user_data) {
  if (active_api_ != kGraphicsAPI_None && api_->GetAPIStatus() == kGraphicsAPI_Active) {
    return api_->SetDisplayChangedCallback(callback, user_data);
  }
  return false;
}

bool DisplayManager::SetSwapchainChangedCallback(SwapchainChangedCallback callback,
                                                 void* user_data) {
  if (active_api_ != kGraphicsAPI_None && api_->GetAPIStatus() == kGraphicsAPI_Active) {
    return api_->SetSwapchainChangedCallback(callback, user_data);
  }
  return false;
}

} // namespace base_game_framework
