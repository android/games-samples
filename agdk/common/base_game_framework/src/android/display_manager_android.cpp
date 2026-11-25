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
#include "gles/graphics_api_gles.h"
#include "platform_util_android.h"
#if defined BGF_DISPLAY_MANAGER_VULKAN
#include "vulkan/graphics_api_vulkan.h"
#endif

namespace base_game_framework {

DisplayManager::DisplayRotation DisplayManager::GetDisplayRotation() {
  switch (PlatformUtilAndroid::GetDisplayRotation()) {
    case 1:
      return kDisplay_Rotation_90;
    case 2:
      return kDisplay_Rotation_180;
    case 3:
      return kDisplay_Rotation_270;
    default:
      return kDisplay_Rotation_0;
  }
}

void DisplayManager::HandlePlatformDisplayChange(const DisplayChangeMessage& change_message) {
  // Only for an active graphics API
  if (api_ != nullptr && active_api_ != kGraphicsAPI_None &&
      api_->GetAPIStatus() == kGraphicsAPI_Active) {
    if (change_message == kDisplay_Change_Window_Terminate) {
      // We need to kill our surface and disassociate the native window
      if (active_api_ == kGraphicsAPI_GLES) {
        api_gles_->LostSurfaceGLES();
      }
      api_->SwapchainChanged(kSwapchain_Lost_Window);
    } else if (change_message == kDisplay_Change_Window_Init) {
      // If we had previously initialized a swapchain, we need to update with the
      // new window and a new surface
      if (active_api_ == kGraphicsAPI_GLES) {
        api_gles_->RestoreSurfaceGLES();
      }
      api_->SwapchainChanged(kSwapchain_Gained_Window);
    } else if (change_message == kDisplay_Change_Window_Resized) {
#if defined BGF_DISPLAY_MANAGER_VULKAN
      // A Vulkan swapchain keeps its size (the presentation engine scales it to the window),
      // so recreate it at the new size. GLES surfaces follow the window, and resizes are
      // detected when presenting.
      if (active_api_ == kGraphicsAPI_Vulkan) {
        api_vulkan_->HandleWindowResized();
      }
#endif
    }
  }
}

} // namespace base_game_framework
