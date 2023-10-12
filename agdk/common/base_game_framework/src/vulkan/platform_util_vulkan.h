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

#ifndef BASEGAMEFRAMEWORK_PLATFORM_UTIL_VULKAN_H_
#define BASEGAMEFRAMEWORK_PLATFORM_UTIL_VULKAN_H_

#include <cstdint>
#include <vector>
#include "platform_defines.h"
#include "display_manager.h"
#include "volk.h"

namespace base_game_framework {

// This class implements utility functions for configuration and initialization
// that required platform-specific code (i.e. specific to Android)
class PlatformUtilVulkan {
 public:
  static bool ActivateSwapchain(VkPhysicalDevice physical_device,
                                VkDevice device,
                                VkSwapchainKHR swapchain,
                                VkQueue queue,
                                uint32_t queue_family_index,
                                DisplayManager::DisplaySwapInterval swap_interval);

  static VkSurfaceKHR CreateSurface(VkInstance instance);

  static void DeactivateSwapchain(VkDevice device, VkSwapchainKHR swapchain);

  static void DestroySurface(VkInstance instance, VkSurfaceKHR surface);

  static void InitMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &create_info);

  static VkPhysicalDevice GetBestDevice(const std::vector<VkPhysicalDevice> &rendering_devices);

  static void GetRefreshRates(VkPhysicalDevice physical_device,
                              VkDevice device,
                              VkSwapchainKHR swapchain,
                              VkQueue queue,
                              uint32_t queue_family_index,
                              std::vector<DisplayManager::DisplaySwapInterval> &swap_intervals);

  static std::vector<const char *> GetRequiredDeviceExtensions(VkPhysicalDevice physical_device);

  static std::vector<const char *> GetRequiredInstanceExtensions(bool enable_validation_layers,
                                                                 bool use_physical2);

  static void GetScreenResolutions(const VkSurfaceCapabilitiesKHR &capabilities,
                                   std::vector<DisplayManager::DisplayResolution> &display_resolutions);

  static std::vector<const char *> GetValidationLayers();

  static bool GetValidationLayersAvailable();

  static uint32_t GetVulkanApiVersion();

  static bool HasNativeWindow();

  static VkResult PresentSwapchain(VkQueue queue, const VkPresentInfoKHR *present_info);

 private:
  static constexpr const char *BGM_CLASS_TAG = "BGF::PlatformUtilVulkan";
};

} // namespace base_game_framework

#endif //BASEGAMEFRAMEWORK_PLATFORM_UTIL_VULKAN_H_
