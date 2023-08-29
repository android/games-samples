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

namespace base_game_framework {

void DisplayManager::HandlePlatformDisplayChange(const DisplayChangeMessage& change_message) {
  if (api_ != nullptr) {
    if (change_message == kDisplay_Change_Window_Terminate) {
      // We need to kill our surface and disassociate the native window
      if (active_api_ == kGraphicsAPI_GLES && api_->GetAPIStatus() == kGraphicsAPI_Active) {
        api_gles_->LostSurfaceGLES();
      }
      api_->SwapchainChanged(kSwapchain_Lost_Window);
    } else if (change_message == kDisplay_Change_Window_Init) {
      // If we had previously initialized a swapchain, we need to update with the
      // new window and a new surface
      if (active_api_ == kGraphicsAPI_GLES && api_->GetAPIStatus() == kGraphicsAPI_Active) {
        api_gles_->RestoreSurfaceGLES();
      }
    }
  }
}

} // namespace base_game_framework
