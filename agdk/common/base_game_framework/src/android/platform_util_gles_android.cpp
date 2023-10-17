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

#include "../gles/platform_util_gles.h"
#include <cstring>
#include <dlfcn.h>
#include "swappy/swappyGL.h"
#include "debug_manager.h"
#include "platform_util_android.h"

namespace base_game_framework {

typedef EGLDisplay (EGLAPIENTRY* PFNGETPLATFORMDISPLAY) (EGLenum platform, void *native_display,
                                                         const EGLAttrib *attrib_list);

static constexpr const char* kEGL_lib_path = "libEGL.so";
static void* lib_egl = nullptr;
static PFNGETPLATFORMDISPLAY eglGetPlatformDisplayFunc = nullptr;

bool PlatformUtilGLES::HasEGLExtension(const char* extension_name) {
  bool supported = false;

  // Passing EGL_NO_DISPLAY works on EGL 1.5 and later, implicit version check
  const char* eglExtensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
  if (eglExtensions != nullptr) {
    if (strstr(eglExtensions, extension_name) != nullptr) {
      supported = true;
    }
  } else {
    PlatformUtilGLES::CheckEGLError("calling eglQueryString - EGL_EXTENSIONS");
  }
  return supported;
}

bool PlatformUtilGLES::GetPlatformDisplaySupported() {
  bool supported = PlatformUtilGLES::HasEGLExtension("EGL_KHR_platform_android");
  // If the extension is supported, try to load the function pointer, it isn't included
  // in the NDK lib.
  if (supported) {
    if (lib_egl == nullptr) {
      lib_egl = dlopen(kEGL_lib_path, RTLD_LAZY);
      if (lib_egl == nullptr) {
        supported = false;
      } else {
        eglGetPlatformDisplayFunc = reinterpret_cast<PFNGETPLATFORMDISPLAY>(
            dlsym(lib_egl, "eglGetPlatformDisplay"));
        if (eglGetPlatformDisplayFunc == nullptr) {
          supported = false;
        }
      }
    }
  }
  return supported;
}

EGLDisplay PlatformUtilGLES::GetPlatformDisplay(EGLenum platform, void* native_display,
                                                       const EGLAttrib* attrib_list) {
  EGLDisplay display = EGL_NO_DISPLAY;
  if (eglGetPlatformDisplayFunc != nullptr) {
    display = eglGetPlatformDisplayFunc(platform, native_display, attrib_list);
    CheckEGLError("Calling eglGetPlatformDisplay");
  }
  return display;
}

EGLenum PlatformUtilGLES::GetPlatformEnum() {
  return EGL_PLATFORM_ANDROID_KHR;
}

void *PlatformUtilGLES::GetNativeDisplay() {
  return reinterpret_cast<void *>(EGL_DEFAULT_DISPLAY);
}

void PlatformUtilGLES::GetRefreshRates(std::vector<DisplayManager::DisplaySwapInterval>
    &swap_intervals) {
  SwappyGL_init(PlatformUtilAndroid::GetMainThreadJNIEnv(), PlatformUtilAndroid::GetActivityClassObject());
  const uint64_t swap_interval = SwappyGL_getSwapIntervalNS();

  // Find the closest match between our internal constants, and what swappy returned
  uint64_t closest_interval = 0;
  uint64_t closest_delta = UINT64_MAX;
  const uint64_t* current_interval = DisplayManager::GetSwapIntervalConstants();
  while (*current_interval > 0) {
    const uint64_t delta = ((*current_interval) > swap_interval) ?
                           ((*current_interval) - swap_interval) :
                           (swap_interval - (*current_interval));
    if (delta < closest_delta) {
      closest_delta = delta;
      closest_interval = *current_interval;
    }
    ++current_interval;
  }
  swap_intervals.push_back(static_cast<DisplayManager::DisplaySwapInterval>(closest_interval));
  // Generate 'step-down' refresh rates
  if (closest_interval == DisplayManager::kDisplay_Swap_Interval_120FPS) {
    swap_intervals.push_back(DisplayManager::kDisplay_Swap_Interval_60FPS);
    swap_intervals.push_back(DisplayManager::kDisplay_Swap_Interval_30FPS);
  } else if (closest_interval == DisplayManager::kDisplay_Swap_Interval_90FPS) {
    swap_intervals.push_back(DisplayManager::kDisplay_Swap_Interval_45FPS);
  } else if (closest_interval == DisplayManager::kDisplay_Swap_Interval_60FPS) {
    swap_intervals.push_back(DisplayManager::kDisplay_Swap_Interval_30FPS);
  }

  SwappyGL_destroy();
}

void PlatformUtilGLES::GetScreenResolutions(const EGLDisplay display, const EGLSurface surface,
    std::vector<DisplayManager::DisplayResolution> &display_resolutions) {
  EGLint display_width = 0;
  EGLint display_height = 0;
  eglQuerySurface(display, surface, EGL_WIDTH, &display_width);
  CheckEGLError("Calling eglQuerySurface width");
  eglQuerySurface(display, surface, EGL_HEIGHT, &display_height);
  CheckEGLError("Calling eglQuerySurface height");

  // TODO: fix with utils::getDeviceRotation when integrating rest of BaseGameFramework!
  // For now, hack assume aspect ratios 4:3 or greater are landscape mode
  const DisplayManager::DisplayOrientation orientation =
      ((display_width / display_height) >= (4.0f / 3.0f)) ?
      DisplayManager::kDisplay_Orientation_Landscape :
      DisplayManager::kDisplay_Orientation_Portrait;

  display_resolutions.push_back(DisplayManager::DisplayResolution(display_width,
    display_height, PlatformUtilAndroid::GetDisplayDPI(), orientation));
}

bool PlatformUtilGLES::CheckEGLError(const char *msg) {
  const EGLint eglError = eglGetError();
  if (eglError != EGL_SUCCESS) {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "EGL error: %d Msg: %s", eglError, msg);
    return false;
  }
  return true;
}

DisplayManager::GLESFeatureFlags PlatformUtilGLES::GetPlatformFeatureFlags() {
  uint32_t returnFlags = DisplayManager::kNo_GLES_Support;
  if (PlatformUtilAndroid::GetAndroidFeatureSupported(
      PlatformUtilAndroid::kAndroidFeatureAndroidExtensionPack)) {
    returnFlags |= DisplayManager::kGLES_AEP_Support;
  }

  return static_cast<DisplayManager::GLESFeatureFlags>(returnFlags);
}

bool PlatformUtilGLES::PlatformInitSwapchain(const uint64_t swap_interval) {
  bool success = SwappyGL_init(PlatformUtilAndroid::GetMainThreadJNIEnv(), PlatformUtilAndroid::GetActivityClassObject());
  if (success) {
    SwappyGL_setSwapIntervalNS(swap_interval);
    SwappyGL_setWindow(PlatformUtilAndroid::GetNativeWindow());
  }
  return success;
}

void PlatformUtilGLES::PlatformShutdownSwapchain() {
  SwappyGL_destroy();
}

void PlatformUtilGLES::PlatformPresentSwapchain(EGLDisplay display, EGLSurface surface) {
  bool swap_success = SwappyGL_swap(display, surface);
  if (!swap_success) {
    CheckEGLError("Calling eglSwapBuffers (via swappy)");
  }
}

bool PlatformUtilGLES::CheckScreenResolutionChange(const EGLDisplay display,
                                                   const EGLSurface surface,
                                                   DisplayManager::DisplayResolution& resolution) {
  EGLint display_width = 0;
  EGLint display_height = 0;
  eglQuerySurface(display, surface, EGL_WIDTH, &display_width);
  CheckEGLError("Calling eglQuerySurface width");
  eglQuerySurface(display, surface, EGL_HEIGHT, &display_height);
  CheckEGLError("Calling eglQuerySurface height");
  if (display_width != resolution.display_width || display_height != resolution.display_height) {
    resolution.display_width = display_width;
    resolution.display_height = display_height;
    resolution.display_dpi = PlatformUtilAndroid::GetDisplayDPI();
    return true;
  }
  return true;
}

void PlatformUtilGLES::LostSurface() {
  SwappyGL_setWindow(nullptr);
}

void PlatformUtilGLES::RestoreSurface() {
  SwappyGL_setWindow(PlatformUtilAndroid::GetNativeWindow());
}

EGLNativeWindowType PlatformUtilGLES::GetNativeWindow() {
  return PlatformUtilAndroid::GetNativeWindow();
}

} // namespace base_game_framework
