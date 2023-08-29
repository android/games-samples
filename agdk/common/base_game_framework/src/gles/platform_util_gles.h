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

#ifndef BASEGAMEFRAMEWORK_PLATFORM_UTIL_GLES_H_
#define BASEGAMEFRAMEWORK_PLATFORM_UTIL_GLES_H_

#include <cstdint>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <vector>

#include "display_manager.h"

namespace base_game_framework {

class PlatformUtilGLES {
 public:
  static bool HasEGLExtension(const char* extension_name);
  static bool GetPlatformDisplaySupported();
  static EGLDisplay GetPlatformDisplay (EGLenum platform, void* native_display,
                                        const EGLAttrib *attrib_list);
  static EGLenum GetPlatformEnum();
  static void GetRefreshRates(std::vector<DisplayManager::DisplaySwapInterval>& swap_intervals);
  static void GetScreenResolutions(const EGLDisplay display, const EGLSurface surface,
      std::vector<DisplayManager::DisplayResolution>& display_resolutions);
  static void *GetNativeDisplay();
  static bool PlatformInitSwapchain(const uint64_t swap_interval);
  static void PlatformShutdownSwapchain();
  static void PlatformPresentSwapchain(EGLDisplay display, EGLSurface surface);
  static bool CheckEGLError(const char* msg);
  static bool CheckScreenResolutionChange(const EGLDisplay display, const EGLSurface surface,
                                          DisplayManager::DisplayResolution& resolution);
  static DisplayManager::GLESFeatureFlags GetPlatformFeatureFlags();
  static EGLNativeWindowType GetNativeWindow();
  static void LostSurface();
  static void RestoreSurface();
 private:
  static constexpr const char* BGM_CLASS_TAG = "BGF::PlatformUtilGLES";
};

} // namespace base_game_framework

#endif //BASEGAMEFRAMEWORK_PLATFORM_UTIL_GLES_H_
