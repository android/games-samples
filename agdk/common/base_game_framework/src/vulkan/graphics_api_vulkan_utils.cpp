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

#include "graphics_api_vulkan_utils.h"
#include "platform_util_vulkan.h"
#include "debug_manager.h"
#include <set>
#include <string>

namespace base_game_framework {

static constexpr VkFormat kColorFormatTableVk[DisplayManager::kDisplay_Pixel_Format_Count] = {
    VK_FORMAT_R8G8B8A8_SRGB,           // kDisplay_Pixel_Format_RGBA8
    VK_FORMAT_B8G8R8A8_SRGB,           // kDisplay_Pixel_Format_BGRA8
    VK_FORMAT_R16G16B16A16_SFLOAT,     // kDisplay_Pixel_Format_RGBA_16F
    VK_FORMAT_R32G32B32A32_SFLOAT,     // kDisplay_Pixel_Format_RGBA_32F
    VK_FORMAT_R4G4B4A4_UNORM_PACK16,   // kDisplay_Pixel_Format_RGBA4
    VK_FORMAT_R5G5B5A1_UNORM_PACK16,   // kDisplay_Pixel_Format_RGBA5551
    VK_FORMAT_R5G6B5_UNORM_PACK16,     // kDisplay_Pixel_Format_RGBA565
    VK_FORMAT_A2B10G10R10_UNORM_PACK32 // kDisplay_Pixel_Format_ABGR2_10_10_10
};

static constexpr VkFormat kDepthFormatTableVk[DisplayManager::kDisplay_Depth_Format_Count] = {
    VK_FORMAT_D24_UNORM_S8_UINT,         // kDisplay_Depth_Format_D24S8_Packed
    VK_FORMAT_D16_UNORM,                 // kDisplay_Depth_Format_16U
    VK_FORMAT_UNDEFINED,                 // kDisplay_Depth_Format_16F
    VK_FORMAT_D32_SFLOAT                 // kDisplay_Depth_Format_32F
};

static constexpr VkFormat kStencilFormatTableVk[DisplayManager::kDisplay_Stencil_Format_Count] = {
    VK_FORMAT_D24_UNORM_S8_UINT,       // kDisplay_Stencil_Format_D24S8_Packed
    VK_FORMAT_S8_UINT                  // kDisplay_Stencil_Format_S8
};

bool VulkanAPIUtils::CheckVkResult(const VkResult result, const char *message) {
  if (result != VK_SUCCESS) {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      "VulkanAPIUtils",
                      "CheckVkResult failed: %d (%s)", result);

  }
  return (result == VK_SUCCESS);
}

GraphicsAPIVulkan::QueueFamilyIndices VulkanAPIUtils::FindQueueFamilies(VkPhysicalDevice device,
                                                                        VkSurfaceKHR surface) {
  GraphicsAPIVulkan::QueueFamilyIndices indices;

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                           queue_families.data());

  int i = 0;
  for (const auto &queue_family : queue_families) {
    if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphics_family = i;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    if (presentSupport) {
      indices.present_family = i;
    }

    if (indices.isComplete()) {
      break;
    }
    i++;
  }
  return indices;
}

bool VulkanAPIUtils::GetImageFormatSupported(VkPhysicalDevice physical_device,
                                             const VkFormat format, const VkImageTiling tiling,
                                             const VkFormatFeatureFlags feature_flags) {

  VkFormatProperties format_properties;
  vkGetPhysicalDeviceFormatProperties(physical_device, format, &format_properties);

  if ((tiling == VK_IMAGE_TILING_LINEAR) &&
      ((format_properties.linearTilingFeatures & feature_flags) == feature_flags)) {
    return true;
  } else if ((tiling == VK_IMAGE_TILING_OPTIMAL) &&
      ((format_properties.optimalTilingFeatures & feature_flags) == feature_flags)) {
    return true;
  }
  return false;
}

void VulkanAPIUtils::GetDepthStencilFormats(VkPhysicalDevice physical_device,
                                            std::vector<DisplayManager::DisplayFormat> &depth_stencil_formats) {
  // Color format and color space are placeholder for our depth/stencil permutations
  DisplayManager::DisplayFormat format;
  // Always have none/none available for depth/stencil
  depth_stencil_formats.push_back(format);

  if (VulkanAPIUtils::GetImageFormatSupported(physical_device, VK_FORMAT_D24_UNORM_S8_UINT,
                                              VK_IMAGE_TILING_OPTIMAL,
                                              VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
    format.display_depth_format = DisplayManager::kDisplay_Depth_Format_D24S8_Packed;
    format.display_stencil_format = DisplayManager::kDisplay_Stencil_Format_D24S8_Packed;
    depth_stencil_formats.push_back(format);
  }

  if (VulkanAPIUtils::GetImageFormatSupported(physical_device, VK_FORMAT_S8_UINT,
                                              VK_IMAGE_TILING_OPTIMAL,
                                              VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
    format.display_depth_format = DisplayManager::kDisplay_Depth_Format_None;
    format.display_stencil_format = DisplayManager::kDisplay_Stencil_Format_S8;
    depth_stencil_formats.push_back(format);
  }

  if (VulkanAPIUtils::GetImageFormatSupported(physical_device, VK_FORMAT_D16_UNORM,
                                              VK_IMAGE_TILING_OPTIMAL,
                                              VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
    format.display_depth_format = DisplayManager::kDisplay_Depth_Format_16U;
    format.display_stencil_format = DisplayManager::kDisplay_Stencil_Format_None;
    depth_stencil_formats.push_back(format);
  }

  if (VulkanAPIUtils::GetImageFormatSupported(physical_device, VK_FORMAT_D16_UNORM_S8_UINT,
                                              VK_IMAGE_TILING_OPTIMAL,
                                              VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
    format.display_depth_format = DisplayManager::kDisplay_Depth_Format_16U;
    format.display_stencil_format = DisplayManager::kDisplay_Stencil_Format_S8;
    depth_stencil_formats.push_back(format);
  }

  if (VulkanAPIUtils::GetImageFormatSupported(physical_device, VK_FORMAT_D32_SFLOAT,
                                              VK_IMAGE_TILING_OPTIMAL,
                                              VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
    format.display_depth_format = DisplayManager::kDisplay_Depth_Format_32F;
    format.display_stencil_format = DisplayManager::kDisplay_Stencil_Format_None;
    depth_stencil_formats.push_back(format);
  }

  if (VulkanAPIUtils::GetImageFormatSupported(physical_device, VK_FORMAT_D32_SFLOAT_S8_UINT,
                                              VK_IMAGE_TILING_OPTIMAL,
                                              VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
    format.display_depth_format = DisplayManager::kDisplay_Depth_Format_32F;
    format.display_stencil_format = DisplayManager::kDisplay_Stencil_Format_S8;
    depth_stencil_formats.push_back(format);
  }
}

void VulkanAPIUtils::GetDisplayFormats(VkPhysicalDevice physical_device, VkSurfaceKHR surface,
                                       std::vector<DisplayManager::DisplayFormat> &display_formats) {
  std::vector<DisplayManager::DisplayFormat> depth_stencil_formats;
  VulkanAPIUtils::GetDepthStencilFormats(physical_device, depth_stencil_formats);

  uint32_t surface_format_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, nullptr);
  std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count,
                                       surface_formats.data());

  std::vector<DisplayManager::DisplayPixelFormat> pixel_formats;
  DisplayManager::DisplayFormat current_format;
  // Only supporting SRGB for display formats right now
  current_format.display_color_space = DisplayManager::kDisplay_Color_Space_SRGB;

  for (const VkSurfaceFormatKHR &format : surface_formats) {
    if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      switch (format.format) {
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:pixel_formats.push_back(
            DisplayManager::kDisplay_Pixel_Format_RGBA8);
          break;
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:pixel_formats.push_back(
            DisplayManager::kDisplay_Pixel_Format_BGRA8);
          break;
        case VK_FORMAT_R16G16B16A16_SFLOAT:pixel_formats.push_back(
            DisplayManager::kDisplay_Pixel_Format_RGBA_16F);
          break;
        case VK_FORMAT_R32G32B32A32_SFLOAT:pixel_formats.push_back(
            DisplayManager::kDisplay_Pixel_Format_RGBA_32F);
          break;
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:pixel_formats.push_back(
            DisplayManager::kDisplay_Pixel_Format_RGBA4);
          break;
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16:pixel_formats.push_back(
            DisplayManager::kDisplay_Pixel_Format_RGBA5551);
          break;
        case VK_FORMAT_R5G6B5_UNORM_PACK16:pixel_formats.push_back(
            DisplayManager::kDisplay_Pixel_Format_RGB565);
          break;
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:pixel_formats.push_back(
            DisplayManager::kDisplay_Pixel_Format_ABGR2_10_10_10);
          break;
        default:break;
      }
    }
  }

  // Build all the permutations of depth/stencil with each color format
  display_formats.reserve(pixel_formats.size() * depth_stencil_formats.size());
  for (const DisplayManager::DisplayPixelFormat pixel_format : pixel_formats) {
    current_format.display_pixel_format = pixel_format;
    for (const DisplayManager::DisplayFormat &depth_stencil_format : depth_stencil_formats) {
      current_format.display_depth_format = depth_stencil_format.display_depth_format;
      current_format.display_stencil_format = depth_stencil_format.display_stencil_format;
      display_formats.push_back(current_format);
    }
  }
}

const char *VulkanAPIUtils::GetMessageSeverityString(VkDebugUtilsMessageSeverityFlagBitsEXT s) {
  switch (s) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:return "VERBOSE";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:return "ERROR";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:return "WARNING";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:return "INFO";
    default:return "UNKNOWN";
  }
}

const char *VulkanAPIUtils::GetMessageTypeString(VkDebugUtilsMessageTypeFlagsEXT s) {
  if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
    return "General | Validation | Performance";
  if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
    return "Validation | Performance";
  if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
    return "General | Performance";
  if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
    return "Performance";
  if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT))
    return "General | Validation";
  if (s == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) return "Validation";
  if (s == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) return "General";
  return "Unknown";
}

bool VulkanAPIUtils::GetUsePhysicalDeviceProperties2() {
  const uint32_t api_version = PlatformUtilVulkan::GetVulkanApiVersion();
  // VK_KHR_get_physical_device_properties2 is core for everything but Vulkan 1.0,
  // in which case we have to check for the instance extension
  if (api_version == VK_API_VERSION_1_0) {
    bool found_extension = false;
    uint32_t instance_extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr);
    std::vector<VkExtensionProperties> instance_extensions(instance_extension_count);
    if (instance_extension_count > 0) {
      vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count,
                                             instance_extensions.data());

      for (const auto &extension_properties : instance_extensions) {
        if (strcmp(extension_properties.extensionName,
                   VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0) {
          found_extension = true;
          break;
        }
      }
    }
    return found_extension;
  }
  return true;
}

VkFormat VulkanAPIUtils::GetVkDepthFormat(const DisplayManager::DisplayDepthFormat depth_format) {
  if (depth_format >= 0) {
    return kDepthFormatTableVk[depth_format];
  }
  return VK_FORMAT_UNDEFINED;
}

VkFormat VulkanAPIUtils::GetVkPixelFormat(const DisplayManager::DisplayPixelFormat pixel_format) {
  if (pixel_format >= 0) {
    return kColorFormatTableVk[pixel_format];
  }
  return VK_FORMAT_UNDEFINED;
}

VkFormat VulkanAPIUtils::GetVkStencilFormat(
    const DisplayManager::DisplayStencilFormat stencil_format) {
  if (stencil_format >= 0) {
    return kStencilFormatTableVk[stencil_format];
  }
  return VK_FORMAT_UNDEFINED;
}

VkColorSpaceKHR VulkanAPIUtils::GetVkColorSpace(const DisplayManager::DisplayColorSpace color_space) {
  VkColorSpaceKHR vk_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  switch (color_space) {
    case DisplayManager::kDisplay_Color_Space_SRGB:
      // Don't actually have a real linear format here?
    case DisplayManager::kDisplay_Color_Space_Linear:break;
  }
  return vk_color_space;
}

bool VulkanAPIUtils::HasDeviceExtension(
    const std::vector<VkExtensionProperties> &available_extensions,
    const char *extension_name) {
  for (const auto &extension : available_extensions) {
    if (strcmp(extension.extensionName, extension_name) == 0) {
      return true;
    }
  }
  return false;
}

bool VulkanAPIUtils::IsRenderingDevice(VkPhysicalDevice device, VkSurfaceKHR surface,
                                       const std::vector<const char *> &required_device_extensions) {
  // Can device render and present to our surface?
  GraphicsAPIVulkan::QueueFamilyIndices indices = FindQueueFamilies(device, surface);
  if (!indices.isComplete()) {
    return false;
  }

  // Does device support our required device extensions?
  uint32_t extension_count;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count,
                                       nullptr);

  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count,
                                       available_extensions.data());

  std::set<std::string> required_extensions(required_device_extensions.begin(),
                                            required_device_extensions.end());

  for (const auto &extension : available_extensions) {
    required_extensions.erase(extension.extensionName);
  }

  if (!required_extensions.empty()) {
    return false;
  }

  // Does device support a functional swapchain with formats and present modes?
  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
  uint32_t present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
  if (format_count == 0 || present_mode_count == 0) {
    return false;
  }

  return true;
}
}