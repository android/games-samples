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

#include "../vulkan/platform_util_vulkan.h"
#include "debug_manager.h"
#include "platform_util_android.h"
#include "swappy/swappyVk.h"
#include <android/api-level.h>

namespace base_game_framework {

// Minimum Android API levels for Vulkan 1.3/1.1 version support
static constexpr int kMinimum_vk13_api_level = 33;
static constexpr int kMinimum_vk11_api_level = 29;

// This is a bit of a hack because of how swappy reports extensions
static char *swappy_extension_strings = nullptr;

bool PlatformUtilVulkan::ActivateSwapchain(VkPhysicalDevice physical_device,
                                           VkDevice device,
                                           VkSwapchainKHR swapchain,
                                           VkQueue queue,
                                           uint32_t queue_family_index,
                                           DisplayManager::DisplaySwapInterval swap_interval) {
  SwappyVk_setQueueFamilyIndex(device, queue, queue_family_index);
  uint64_t refresh_duration = 0;
  bool success = SwappyVk_initAndGetRefreshCycleDuration(
      PlatformUtilAndroid::GetMainThreadJNIEnv(),
      PlatformUtilAndroid::GetActivityClassObject(),
      physical_device, device, swapchain, &refresh_duration);
  if (success) {
    SwappyVk_setWindow(device, swapchain, PlatformUtilAndroid::GetNativeWindow());
    SwappyVk_setSwapIntervalNS(device, swapchain, swap_interval);
  }
  return success;
}

void PlatformUtilVulkan::DeactivateSwapchain(VkDevice device, VkSwapchainKHR swapchain) {
  SwappyVk_destroySwapchain(device, swapchain);
}

VkResult PlatformUtilVulkan::PresentSwapchain(VkQueue queue, const VkPresentInfoKHR *present_info) {
  return SwappyVk_queuePresent(queue, present_info);
}

VkSurfaceKHR PlatformUtilVulkan::CreateSurface(VkInstance instance) {
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkAndroidSurfaceCreateInfoKHR create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
  create_info.pNext = nullptr;
  create_info.flags = 0;
  create_info.window = PlatformUtilAndroid::GetNativeWindow();

  const VkResult result = vkCreateAndroidSurfaceKHR(instance, &create_info,
                                                    nullptr, &surface);
  if (result != VK_SUCCESS) {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "vkCreateAndroidSurfaceKHR failed: %d", result);
  }
  return surface;
}

void PlatformUtilVulkan::DestroySurface(VkInstance instance, VkSurfaceKHR surface) {
  if (surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(instance, surface, nullptr);
  }
  // Take this opportunity to clean up the swappy string alloc, if it exists
  if (swappy_extension_strings != nullptr) {
    free(swappy_extension_strings);
    swappy_extension_strings = nullptr;
  }
}

void PlatformUtilVulkan::InitMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &create_info) {
  create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
}

VkPhysicalDevice PlatformUtilVulkan::GetBestDevice(
    const std::vector<VkPhysicalDevice> &rendering_devices) {
  // TODO: proper ranking if multiple devices
  return rendering_devices[0];
}

void PlatformUtilVulkan::GetRefreshRates(VkPhysicalDevice physical_device,
                                         VkDevice device,
                                         VkSwapchainKHR swapchain,
                                         VkQueue queue,
                                         uint32_t queue_family_index,
                                         std::vector<DisplayManager::DisplaySwapInterval> &
                                         swap_intervals) {
  SwappyVk_setQueueFamilyIndex(device, queue, queue_family_index);

  uint64_t refresh_duration = 0;
  bool success = SwappyVk_initAndGetRefreshCycleDuration(PlatformUtilAndroid::GetMainThreadJNIEnv(),
                                                         PlatformUtilAndroid::GetActivityClassObject(),
                                                         physical_device,
                                                         device,
                                                         swapchain,
                                                         &refresh_duration);
  if (!success) {
    return;
  }
  SwappyVk_setWindow(device, swapchain, PlatformUtilAndroid::GetNativeWindow());

  int refresh_rate_count = SwappyVk_getSupportedRefreshPeriodsNS(nullptr, 0, swapchain);
  std::vector<uint64_t> refresh_rates(refresh_rate_count);
  if (refresh_rate_count == 0) {
    refresh_rates.push_back(refresh_duration);
  } else {
    SwappyVk_getSupportedRefreshPeriodsNS(refresh_rates.data(), refresh_rate_count, swapchain);
  }

  uint64_t closest_interval;
  for (const uint64_t refresh_rate : refresh_rates) {
    // Find the closest match between our internal constants, and what swappy returned
    closest_interval = 0;
    uint64_t closest_delta = UINT64_MAX;
    const uint64_t *current_interval = DisplayManager::GetSwapIntervalConstants();
    while (*current_interval > 0) {
      const uint64_t delta = ((*current_interval) > refresh_rate) ?
                             ((*current_interval) - refresh_rate) :
                             (refresh_rate - (*current_interval));
      if (delta < closest_delta) {
        closest_delta = delta;
        closest_interval = *current_interval;
      }
      ++current_interval;
    }
    swap_intervals.push_back(static_cast<DisplayManager::DisplaySwapInterval>(closest_interval));
  }

  // If we didn't get a list, generate 'step-down' refresh rates
  if (refresh_rate_count == 0) {
    if (closest_interval == DisplayManager::kDisplay_Swap_Interval_120FPS) {
      swap_intervals.push_back(DisplayManager::kDisplay_Swap_Interval_60FPS);
      swap_intervals.push_back(DisplayManager::kDisplay_Swap_Interval_30FPS);
    } else if (closest_interval == DisplayManager::kDisplay_Swap_Interval_90FPS) {
      swap_intervals.push_back(DisplayManager::kDisplay_Swap_Interval_45FPS);
    } else if (closest_interval == DisplayManager::kDisplay_Swap_Interval_60FPS) {
      swap_intervals.push_back(DisplayManager::kDisplay_Swap_Interval_30FPS);
    }
  }

  // This doesn't actually destroy the swapchain, just the swappy instance attached to it
  SwappyVk_destroySwapchain(device, swapchain);
}

std::vector<const char *> PlatformUtilVulkan::GetRequiredDeviceExtensions(
    VkPhysicalDevice physical_device) {

  uint32_t extension_count = 0;
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count,
                                       nullptr);

  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count,
                                       available_extensions.data());

  // Add any available extensions Swappy wants
  uint32_t swappy_extension_count = 0;
  SwappyVk_determineDeviceExtensions(physical_device, extension_count, available_extensions.data(),
                                     &swappy_extension_count, nullptr);

  std::vector<const char *> device_extensions;

  // Swappy expects valid string buffers for its extension names, rather than just copying
  // pointers to its constants. Make a buffer for it to strcpy into
  const size_t swappy_string_size = VK_MAX_EXTENSION_NAME_SIZE * swappy_extension_count;
  if (swappy_extension_count > 0) {
    if (swappy_extension_strings != nullptr) {
      free(swappy_extension_strings);
    }
    swappy_extension_strings = (char *) malloc(swappy_string_size);
    memset(swappy_extension_strings, 0, swappy_string_size);
  }
  char *base = swappy_extension_strings;
  for (uint32_t i = 0; i < swappy_extension_count; ++i) {
    device_extensions.push_back(base);
    base += VK_MAX_EXTENSION_NAME_SIZE;
  }

  SwappyVk_determineDeviceExtensions(physical_device, extension_count, available_extensions.data(),
                                     &swappy_extension_count,
                                     const_cast<char **>(device_extensions.data()));

  device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  // If we are on a Vulkan 1.0 device, require the VK_KHR_maintenance1 extension, which is
  // core from Vulkan 1.1+
  VkPhysicalDeviceProperties device_properties{};
  vkGetPhysicalDeviceProperties(physical_device, &device_properties);
  const uint32_t device_major_version = VK_VERSION_MAJOR(device_properties.apiVersion);
  const uint32_t device_minor_version = VK_VERSION_MINOR(device_properties.apiVersion);
  if (device_major_version == 1 && device_minor_version == 0) {
    device_extensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
  }

  return device_extensions;
}

std::vector<const char *> PlatformUtilVulkan::GetRequiredInstanceExtensions(
    bool enable_validation_layers, bool use_physical2) {
  std::vector<const char *> instance_extensions;

  instance_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
  instance_extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
  if (enable_validation_layers) {
    instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
  // Request the physical device properties 2 extension on 1.0, is core from
  // 1.1+
  if (GetVulkanApiVersion() == VK_API_VERSION_1_0 && use_physical2) {
    instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  }

  return instance_extensions;
}

void PlatformUtilVulkan::GetScreenResolutions(const VkSurfaceCapabilitiesKHR &capabilities,
                                              std::vector<DisplayManager::DisplayResolution> &display_resolutions) {
  const bool rotated = (capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ||
      capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR);
  const int32_t display_width = capabilities.currentExtent.width;
  const int32_t display_height = capabilities.currentExtent.height;
  const DisplayManager::DisplayOrientation orientation = rotated ?
                                                         DisplayManager::kDisplay_Orientation_Landscape
                                                                 :
                                                         DisplayManager::kDisplay_Orientation_Portrait;

  display_resolutions.push_back(DisplayManager::DisplayResolution(display_width,
                                                                  display_height,
                                                                  PlatformUtilAndroid::GetDisplayDPI(),
                                                                  orientation));
}

std::vector<const char *> PlatformUtilVulkan::GetValidationLayers() {
  std::vector<const char *> validation_layers;
  validation_layers.push_back("VK_LAYER_KHRONOS_validation");
  // TODO: vendor layer support
  return validation_layers;
}

bool PlatformUtilVulkan::GetValidationLayersAvailable() {
  std::vector<const char *> validation_layers =
      PlatformUtilVulkan::GetValidationLayers();

  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
  std::vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  for (const char *layerName : validation_layers) {
    bool layer_found = false;
    for (const auto &layerProperties : available_layers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layer_found = true;
        break;
      }
    }

    if (!layer_found) {
      return false;
    }
  }
  return true;
}

uint32_t PlatformUtilVulkan::GetVulkanApiVersion() {
  const int device_level = android_get_device_api_level();
  if (device_level >= kMinimum_vk13_api_level) {
    return VK_API_VERSION_1_3;
  } else if (device_level >= kMinimum_vk11_api_level) {
    return VK_API_VERSION_1_1;
  }
  return VK_API_VERSION_1_0;
}

bool PlatformUtilVulkan::HasNativeWindow() {
  return (PlatformUtilAndroid::GetNativeWindow() != nullptr);
}

}