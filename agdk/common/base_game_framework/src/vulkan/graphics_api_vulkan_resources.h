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

#ifndef BASEGAMEFRAMEWORK_GRAPHICSAPI_VULKAN_RESOURCES_H_
#define BASEGAMEFRAMEWORK_GRAPHICSAPI_VULKAN_RESOURCES_H_

#include "platform_defines.h"
#include <cstdint>
#include "volk.h"
#include "vk_mem_alloc.h"

namespace base_game_framework {

struct GraphicsAPIResourcesVk {
  VkDevice device = VK_NULL_HANDLE;
  VkInstance instance = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkQueue render_queue = VK_NULL_HANDLE;
  VmaAllocator allocator = VK_NULL_HANDLE;
  uint32_t graphics_queue_index = 0;
  VkSurfaceTransformFlagBitsKHR pretransform_flags = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
};

struct SwapchainFrameResourcesVk {
  VkFence frame_fence = VK_NULL_HANDLE;
  VkSemaphore image_available = VK_NULL_HANDLE;
  VkSemaphore render_complete = VK_NULL_HANDLE;
  VkImageView swapchain_color_image_view = VK_NULL_HANDLE;
  VkImageView swapchain_depth_stencil_image_view = VK_NULL_HANDLE;
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  VkExtent2D swapchain_extent = {0, 0};
  VkFormat swapchain_color_format = VK_FORMAT_UNDEFINED;
  VkFormat swapchain_depth_stencil_format = VK_FORMAT_UNDEFINED;
  uint32_t swapchain_frame_index = 0;
  uint32_t swapchain_image_index = 0;
};

} // namespace base_game_framework

#endif // BASEGAMEFRAMEWORK_GRAPHICSAPI_VULKAN_RESOURCES_H_
