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

#ifndef BASEGAMEFRAMEWORK_GRAPHICSAPI_VULKAN_UTILS_H_
#define BASEGAMEFRAMEWORK_GRAPHICSAPI_VULKAN_UTILS_H_

#include "graphics_api_vulkan.h"
#include "display_manager.h"

namespace base_game_framework {
// Non-platform specific Vulkan utility functions
class VulkanAPIUtils {
 public:

  static bool CheckVkResult(const VkResult result, const char *message);
  static GraphicsAPIVulkan::QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device,
                                                                 VkSurfaceKHR surface);
  static void GetDepthStencilFormats(VkPhysicalDevice physical_device,
                                     std::vector<DisplayManager::DisplayFormat> &depth_stencil_formats);
  static void GetDisplayFormats(VkPhysicalDevice physical_device, VkSurfaceKHR surface,
                                std::vector<DisplayManager::DisplayFormat> &display_formats);
  static bool GetImageFormatSupported(VkPhysicalDevice physical_device, const VkFormat format,
                                      const VkImageTiling tiling,
                                      const VkFormatFeatureFlags feature_flags);
  static const char *GetMessageSeverityString(VkDebugUtilsMessageSeverityFlagBitsEXT s);
  static const char *GetMessageTypeString(VkDebugUtilsMessageTypeFlagsEXT s);
  static bool GetUsePhysicalDeviceProperties2();
  static VkFormat GetVkDepthFormat(const DisplayManager::DisplayDepthFormat depth_format);
  static VkFormat GetVkPixelFormat(const DisplayManager::DisplayPixelFormat pixel_format);
  static VkFormat GetVkStencilFormat(const DisplayManager::DisplayStencilFormat stencil_format);
  static VkColorSpaceKHR GetVkColorSpace(const DisplayManager::DisplayColorSpace color_space);
  static bool HasDeviceExtension(const std::vector<VkExtensionProperties> &available_extensions,
                                 const char *extension_name);
  // Can this physical device be used for graphics rendering, with presentation and swapchain?
  static bool IsRenderingDevice(VkPhysicalDevice device, VkSurfaceKHR surface,
                                const std::vector<const char *> &required_device_extensions);
};
} // namespace base_game_framework

#endif //BASEGAMEFRAMEWORK_GRAPHICSAPI_VULKAN_UTILS_H_
