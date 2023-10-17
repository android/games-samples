/*
 * Copyright 2023 The Android Open Source Project
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
#include "renderer_debug.h"
#include "renderer_vk_includes.h"

namespace simple_renderer {
bool RendererCheckVk(int result, const char* message) {
  const char* error_string = "unknown VkResult";
  if (result < 0) {
    switch (result) {
      case VK_ERROR_OUT_OF_HOST_MEMORY:
        error_string = "VK_ERROR_OUT_OF_HOST_MEMORY"; break;
      case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        error_string = "VK_ERROR_OUT_OF_DEVICE_MEMORY"; break;
      case VK_ERROR_INITIALIZATION_FAILED:
        error_string = "VK_ERROR_INITIALIZATION_FAILED"; break;
      case VK_ERROR_DEVICE_LOST:
        error_string = "VK_ERROR_DEVICE_LOST"; break;
      case VK_ERROR_MEMORY_MAP_FAILED:
        error_string = "VK_ERROR_MEMORY_MAP_FAILED"; break;
      case VK_ERROR_LAYER_NOT_PRESENT:
        error_string = "VK_ERROR_LAYER_NOT_PRESENT"; break;
      case VK_ERROR_EXTENSION_NOT_PRESENT:
        error_string = "VK_ERROR_EXTENSION_NOT_PRESENT"; break;
      case VK_ERROR_FEATURE_NOT_PRESENT:
        error_string = "VK_ERROR_FEATURE_NOT_PRESENT"; break;
      case VK_ERROR_INCOMPATIBLE_DRIVER:
        error_string = "VK_ERROR_INCOMPATIBLE_DRIVER"; break;
      case VK_ERROR_TOO_MANY_OBJECTS:
        error_string = "VK_ERROR_TOO_MANY_OBJECTS"; break;
      case VK_ERROR_FORMAT_NOT_SUPPORTED:
        error_string = "VK_ERROR_FORMAT_NOT_SUPPORTED"; break;
      case VK_ERROR_FRAGMENTED_POOL:
        error_string = "VK_ERROR_FRAGMENTED_POOL"; break;
      case VK_ERROR_UNKNOWN:
        error_string = "VK_ERROR_UNKNOWN"; break;
      case VK_ERROR_OUT_OF_POOL_MEMORY:
        error_string = "VK_ERROR_OUT_OF_POOL_MEMORY"; break;
      case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        error_string = "VK_ERROR_INVALID_EXTERNAL_HANDLE"; break;
      case VK_ERROR_FRAGMENTATION:
        error_string = "VK_ERROR_FRAGMENTATION"; break;
      case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        error_string = "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS"; break;
      case VK_PIPELINE_COMPILE_REQUIRED:
        error_string = "VK_PIPELINE_COMPILE_REQUIRED"; break;
      case VK_ERROR_SURFACE_LOST_KHR:
        error_string = "VK_ERROR_SURFACE_LOST_KHR"; break;
      case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        error_string = "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR"; break;
      case VK_SUBOPTIMAL_KHR:
        error_string = "VK_SUBOPTIMAL_KHR"; break;
      case VK_ERROR_OUT_OF_DATE_KHR:
        error_string = "VK_ERROR_OUT_OF_DATE_KHR"; break;
      case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        error_string = "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR"; break;
      case VK_ERROR_VALIDATION_FAILED_EXT:
        error_string = "VK_ERROR_VALIDATION_FAILED_EXT"; break;
      case VK_ERROR_INVALID_SHADER_NV:
        error_string = "VK_ERROR_INVALID_SHADER_NV"; break;
      case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
        error_string = "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT"; break;
      case VK_ERROR_NOT_PERMITTED_KHR:
        error_string = "VK_ERROR_NOT_PERMITTED_KHR"; break;
      case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
        error_string = "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT"; break;
      default:
        break;
    }
    RENDERER_ERROR("Vk error: (%d) %s from %s", result, error_string, message)
    RENDERER_ASSERT(false)
  }
  return (result >= 0);
}
}