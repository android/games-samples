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

#include "graphics_api_gles.h"
#include "debug_manager.h"
#include "platform_util_gles.h"

namespace base_game_framework {

static constexpr DisplayManager::SwapchainFrameHandle kDefault_swapchain_handle = 1;
static constexpr uint32_t kMax_element_count = 20;
// Just 'double buffer' for GLES
static constexpr uint32_t kSwapchain_frame_count_gles = 2;
// Just 'vblank' present mode for GLES
static constexpr DisplayManager::SwapchainPresentMode present_mode_gles =
    DisplayManager::kSwapchain_Present_Fifo;

GraphicsAPIGLES::GraphicsAPIGLES()
  : GraphicsAPIBase()
  , api_status_(kGraphicsAPI_ObtainingAvailability)
  , api_features_()
  , display_changed_callback_(nullptr)
  , display_changed_user_data_(nullptr)
  , feature_flags_(DisplayManager::kNo_GLES_Support)
  , swapchain_format_()
  , swapchain_resolution_(0, 0, 0, DisplayManager::kDisplay_Orientation_Landscape)
  , swapchain_interval_(DisplayManager::kDisplay_Swap_Interval_60FPS)
  , swapchain_frame_count_(0)
  , swapchain_present_mode_(DisplayManager::kSwapchain_Present_Fifo)
  , egl_config_(nullptr)
  , egl_display_(EGL_NO_DISPLAY)
  , egl_surface_(EGL_NO_SURFACE)
  , egl_context_(EGL_NO_CONTEXT)
  , srgb_framebuffer_support_(false) {
}

GraphicsAPIGLES::~GraphicsAPIGLES() {

}

void GraphicsAPIGLES::QueryAvailability() {
  // early out
  EGLNativeWindowType native_window = PlatformUtilGLES::GetNativeWindow();
  if (native_window != nullptr) {
    QueryCapabilities();
  }
}

void GraphicsAPIGLES::QueryCapabilities() {
  egl_display_ = InitializeEGLDisplay();
  if (egl_display_ != EGL_NO_DISPLAY) {
    srgb_framebuffer_support_ = PlatformUtilGLES::HasEGLExtension("EGL_KHR_gl_colorspace");
    eglInitialize(egl_display_, nullptr, nullptr);

    if (PlatformUtilGLES::CheckEGLError("Calling eglInitialize")) {
      // Build a list of supported configurations and use that to generate our
      // list of display formats valid for 'swapchain' creation
      ParseEGLConfigs();

      if (display_formats_.size() > 0) {
        egl_surface_ = InitializeEGLSurface(display_formats_[0]);
        if (egl_surface_ != EGL_NO_SURFACE) {
          // Grab our screen resolutions
          PlatformUtilGLES::GetScreenResolutions(egl_display_, egl_surface_, display_resolutions_);
          // Use Swappy to generate a list of refresh rates
          PlatformUtilGLES::GetRefreshRates(swap_intervals_);

          egl_context_ = InitializeEGLContext();
          if (egl_context_ != EGL_NO_CONTEXT) {
            // Bind the context
            if (eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_)
                  == EGL_TRUE) {
              // Setup the version and feature flags
              const char *version_string = reinterpret_cast<const char *>(glGetString(GL_VERSION));
              uint32_t version_flags = DisplayManager::kGLES_3_0_Support;
              if (version_string != nullptr) {
                if (strstr(version_string, "3.2")) {
                  version_flags |= DisplayManager::kGLES_3_2_Support;
                  version_flags |= DisplayManager::kGLES_3_1_Support;
                } else if (strstr(version_string, "3.1")) {
                  version_flags |= DisplayManager::kGLES_3_1_Support;
                }
              }

              const DisplayManager::GLESFeatureFlags platform_flags =
                  PlatformUtilGLES::GetPlatformFeatureFlags();
              feature_flags_ = static_cast<DisplayManager::GLESFeatureFlags>(
                  (version_flags | static_cast<uint32_t>(platform_flags)));
              api_features_.SetGraphicsFeature(GraphicsAPIFeatures::kGraphicsFeature_Wide_Lines);

              DebugManager::Log(DebugManager::kLog_Channel_Default,
                                DebugManager::kLog_Level_Info,
                                BGM_CLASS_TAG,
                                "GL_VERSION: %s Feature flags: 0x%x", version_string,
                                static_cast<unsigned int>(feature_flags_));

              // Done with the queries, tear everything down
              eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            } else {
              PlatformUtilGLES::CheckEGLError("Calling eglMakeCurrent");
            }
            eglDestroyContext(egl_display_, egl_context_);
            egl_context_ = EGL_NO_CONTEXT;
          }
          eglDestroySurface(egl_display_, egl_surface_);
          egl_surface_ = EGL_NO_SURFACE;
        }
      }
    }
    eglTerminate(egl_display_);
    egl_config_ = nullptr;
    egl_display_ = EGL_NO_DISPLAY;
  }

  api_status_ = kGraphicsAPI_AvailabilityReady;
}

bool GraphicsAPIGLES::DisplayFormatExists(const DisplayManager::DisplayFormat& display_format) {
  // Config list is small, just do a linear search
  for (auto iter = display_formats_.begin(); iter != display_formats_.end(); ++iter) {
    if (*iter == display_format) {
      return true;
    }
  }
  return false;
}

const EGLint *GraphicsAPIGLES::GenerateEGLConfigAttribList(
    const DisplayManager::DisplayFormat& display_format) {
  EGLint* attrib = new EGLint[kMax_element_count];
  // Set up defaults
  EGLint red_size = 8;
  EGLint green_size = 8;
  EGLint blue_size = 8;
  EGLint alpha_size = 0;
  EGLint depth_size = 0;
  EGLint stencil_size = 0;
  EGLint *current_attrib = attrib;

  switch (display_format.display_pixel_format) {
    case DisplayManager::kDisplay_Pixel_Format_RGBA8:
      alpha_size = 8;
      break;
    case DisplayManager::kDisplay_Pixel_Format_RGBA4:
      red_size = 4;
      green_size = 4;
      blue_size = 4;
      alpha_size = 4;
      break;
    case DisplayManager::kDisplay_Pixel_Format_RGBA5551:
      red_size = 5;
      green_size = 5;
      blue_size = 5;
      alpha_size = 1;
    case DisplayManager::kDisplay_Pixel_Format_RGB565:
      red_size = 5;
      green_size = 6;
      blue_size = 5;
    default: break;
  }

  switch (display_format.display_depth_format) {
    case DisplayManager::kDisplay_Depth_Format_16F:
    case DisplayManager::kDisplay_Depth_Format_16U:
      depth_size = 16;
      break;
    case DisplayManager::kDisplay_Depth_Format_D24S8_Packed:
      depth_size = 24;
      break;
    default: break;
  }

  switch (display_format.display_stencil_format) {
    case DisplayManager::kDisplay_Stencil_Format_S8:
    case DisplayManager::kDisplay_Stencil_Format_D24S8_Packed:
      stencil_size = 8;
      break;
    default: break;
  }

  *current_attrib++ = EGL_RENDERABLE_TYPE;
  *current_attrib++ = EGL_OPENGL_ES3_BIT;
  *current_attrib++ = EGL_SURFACE_TYPE;
  *current_attrib++ = EGL_WINDOW_BIT;
  *current_attrib++ = EGL_RED_SIZE;
  *current_attrib++ = red_size;
  *current_attrib++ = EGL_GREEN_SIZE;
  *current_attrib++ = green_size;
  *current_attrib++ = EGL_BLUE_SIZE;
  *current_attrib++ = blue_size;
  if (alpha_size > 0) {
    *current_attrib++ = EGL_ALPHA_SIZE;
    *current_attrib++ = alpha_size;
  }
  if (depth_size > 0) {
    *current_attrib++ = EGL_DEPTH_SIZE;
    *current_attrib++ = depth_size;
  }
  if (stencil_size > 0) {
    *current_attrib++ = EGL_STENCIL_SIZE;
    *current_attrib++ = stencil_size;
  }
  // terminate the attribute list
  *current_attrib = EGL_NONE;
  return attrib;
}

/*
 * Get a list of display configurations for this display, iterate through the
 * GLES 3 capable configurations and check their color, depth and stencil formats.
 * Build a list of configs that have format configurations that match our
 * internal enums to present as options for display/swapchain initialization
 */
void GraphicsAPIGLES::ParseEGLConfigs() {
  EGLint config_count;
  eglGetConfigs(egl_display_, nullptr, 0, &config_count);
  if (config_count > 0) {
    EGLConfig* configs = new EGLConfig[config_count];
    eglGetConfigs(egl_display_, configs, config_count, &config_count);

    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Info,
                      BGM_CLASS_TAG,
                      "EGL config count: %d Feature flags: 0x%x", config_count);

    for (EGLint i = 0; i < config_count; ++i) {
      EGLConfig config = configs[i];
      if (config != nullptr) {
        EGLint renderable_type = 0;
        EGLint surface_type = 0;
        EGLBoolean renderable_result = eglGetConfigAttrib(egl_display_, config,
                                                          EGL_RENDERABLE_TYPE, &renderable_type);
        EGLBoolean surface_result = eglGetConfigAttrib(egl_display_, config,
                                                       EGL_SURFACE_TYPE, &surface_type);
        if (renderable_result == EGL_TRUE && ((renderable_type & EGL_OPENGL_ES3_BIT) != 0) &&
            surface_result == EGL_TRUE && ((surface_type & EGL_WINDOW_BIT) != 0)) {
          bool validFormat = true;
          DisplayManager::DisplayFormat config_display_format;
          config_display_format.display_depth_format = DisplayManager::kDisplay_Depth_Format_None;
          config_display_format.display_stencil_format =
              DisplayManager::kDisplay_Stencil_Format_None;
          config_display_format.display_pixel_format = DisplayManager::kDisplay_Pixel_Format_RGBA8;
          config_display_format.display_color_space = DisplayManager::kDisplay_Color_Space_Linear;

          EGLint red_size = 0;
          EGLint green_size = 0;
          EGLint blue_size = 0;
          EGLint alpha_size = 0;

          // Require RGB channels to exist for this to be a useful config
          EGLBoolean result = eglGetConfigAttrib(egl_display_, config, EGL_RED_SIZE, &red_size);
          if (result == EGL_FALSE || red_size == 0) {
            validFormat = false;
          }
          result = eglGetConfigAttrib(egl_display_, config, EGL_GREEN_SIZE, &green_size);
          if (result == EGL_FALSE || green_size == 0) {
            validFormat = false;
          }
          result = eglGetConfigAttrib(egl_display_, config, EGL_BLUE_SIZE, &blue_size);
          if (result == EGL_FALSE || blue_size == 0) {
            validFormat = false;
          }
          // Alpha is optional
          eglGetConfigAttrib(egl_display_, config, EGL_ALPHA_SIZE, &alpha_size);

          // Check results against the RGBA configurations we care about
          if (red_size == 4 && green_size == 4 && blue_size == 4 && alpha_size == 4) {
            config_display_format.display_pixel_format =
                DisplayManager::kDisplay_Pixel_Format_RGBA4;
          } else if (red_size == 5 && green_size == 6 && blue_size == 5 && alpha_size == 0) {
            config_display_format.display_pixel_format =
                DisplayManager::kDisplay_Pixel_Format_RGB565;
          } else if (red_size == 5 && green_size == 5 && blue_size == 5 && alpha_size == 1) {
            config_display_format.display_pixel_format =
                DisplayManager::kDisplay_Pixel_Format_RGBA5551;
          } else if (red_size == 8 && green_size == 8 && blue_size == 8 && alpha_size == 8) {
            config_display_format.display_pixel_format =
                DisplayManager::kDisplay_Pixel_Format_RGBA8;
          } else {
            validFormat = false;
          }

          // Depth and stencil are optional, but part of the config
          EGLint depth_size = 0;
          EGLint stencil_size = 0;
          eglGetConfigAttrib(egl_display_, config, EGL_DEPTH_SIZE, &depth_size);
          eglGetConfigAttrib(egl_display_, config, EGL_STENCIL_SIZE, &stencil_size);
          if (depth_size == 24 && stencil_size == 8) {
            config_display_format.display_depth_format =
                DisplayManager::kDisplay_Depth_Format_D24S8_Packed;
            config_display_format.display_stencil_format =
                DisplayManager::kDisplay_Stencil_Format_D24S8_Packed;
          } else {
            if (depth_size == 16) {
              config_display_format.display_depth_format =
                  DisplayManager::kDisplay_Depth_Format_16U;
            }

            if (stencil_size == 8) {
              config_display_format.display_stencil_format =
                  DisplayManager::kDisplay_Stencil_Format_S8;
            }
          }

          DebugManager::Log(DebugManager::kLog_Channel_Default,
                            DebugManager::kLog_Level_Info,
                            BGM_CLASS_TAG,
                            "EGL format rgba,d,s: %d %d %d %d - %d %d", red_size, green_size,
                            blue_size, alpha_size, depth_size, stencil_size);

          if (validFormat) {
            // Add config to our array of available display formats, if it hasn't
            // already been added
            if (!GraphicsAPIGLES::DisplayFormatExists(config_display_format)) {
              display_formats_.push_back(config_display_format);
              // Add sRGB variant for RGB8/RGBA8 if sRGB available
              if (srgb_framebuffer_support_) {
                if (config_display_format.display_pixel_format ==
                        DisplayManager::kDisplay_Pixel_Format_RGBA8) {
                  config_display_format.display_color_space =
                      DisplayManager::kDisplay_Color_Space_SRGB;
                  display_formats_.push_back(config_display_format);
                }
              }
            }
          }
        }
      }
    }
    delete[] configs;
  }
}

EGLDisplay GraphicsAPIGLES::InitializeEGLDisplay() {
  EGLDisplay returnDisplay = nullptr;
  if (PlatformUtilGLES::GetPlatformDisplaySupported()) {
    returnDisplay = PlatformUtilGLES::GetPlatformDisplay(PlatformUtilGLES::GetPlatformEnum(),
                                          PlatformUtilGLES::GetNativeDisplay(), nullptr);
    // fallback to eglGetDisplay
    if (returnDisplay == EGL_NO_DISPLAY) {
      returnDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
  } else {
    returnDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  }
  if (returnDisplay == EGL_NO_DISPLAY) {
    PlatformUtilGLES::CheckEGLError("eglGetDisplay failed");
  }
  return returnDisplay;
}

EGLSurface GraphicsAPIGLES::InitializeEGLSurface(
    const DisplayManager::DisplayFormat &display_format) {
  EGLSurface surface = EGL_NO_SURFACE;
  EGLNativeWindowType native_window = PlatformUtilGLES::GetNativeWindow();
  if (egl_display_ != EGL_NO_DISPLAY && native_window != nullptr) {
    const EGLint* attribs = GenerateEGLConfigAttribList(display_format);
    EGLint num_configs;
    if (eglChooseConfig(egl_display_, attribs, &egl_config_, 1, &num_configs) == EGL_TRUE) {
      // create EGL surface
      surface = eglCreateWindowSurface(egl_display_, egl_config_, native_window, nullptr);
      if (surface == EGL_NO_SURFACE) {
        PlatformUtilGLES::CheckEGLError("eglCreateWindowSurface failed");
      }
    }
    delete[] attribs;
  }
  return surface;
}

EGLContext GraphicsAPIGLES::InitializeEGLContext() {
  EGLContext context = EGL_NO_CONTEXT;
  if (egl_display_ != EGL_NO_DISPLAY && egl_config_ != nullptr) {
    const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    context = eglCreateContext(egl_display_, egl_config_, nullptr, context_attribs);
    PlatformUtilGLES::CheckEGLError("Calling eglCreateContext");
  }
  return context;
}

bool GraphicsAPIGLES::InitializeGraphicsAPI() {
  egl_display_ = InitializeEGLDisplay();
  if (egl_display_ != EGL_NO_DISPLAY) {
    eglInitialize(egl_display_, nullptr, nullptr);
    if (PlatformUtilGLES::CheckEGLError("Calling eglInitialize")) {
      api_status_ = kGraphicsAPI_Active;
      return true;
    }
  }
  return false;
}

void GraphicsAPIGLES::ShutdownGraphicsAPI() {
  if (egl_display_ != EGL_NO_DISPLAY) {
    eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (egl_context_ != EGL_NO_CONTEXT) {
      eglDestroyContext(egl_display_, egl_context_);
      egl_context_ = EGL_NO_CONTEXT;
      if (egl_surface_ != EGL_NO_SURFACE) {
        eglDestroySurface(egl_display_, egl_surface_);
        egl_surface_ = EGL_NO_SURFACE;
      }
    }
    eglTerminate(egl_display_);
    egl_config_ = nullptr;
    egl_display_ = EGL_NO_DISPLAY;
  }
  api_status_ = kGraphicsAPI_AvailabilityReady;
}

DisplayManager::SwapchainConfigurations *GraphicsAPIGLES::GenerateSwapchainConfigurations() {
  DisplayManager::SwapchainConfigurations* swap_configs =
      new DisplayManager::SwapchainConfigurations(display_formats_, display_resolutions_,
                                                  swap_intervals_, kSwapchain_frame_count_gles,
                                                  kSwapchain_frame_count_gles, present_mode_gles,
                                                  DisplayManager::kDefault_Display);
  return swap_configs;
}

void GraphicsAPIGLES::LostSurfaceGLES() {
  eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  if (egl_surface_ != EGL_NO_SURFACE) {
    eglDestroySurface(egl_display_, egl_surface_);
    egl_surface_ = EGL_NO_SURFACE;
  }
  PlatformUtilGLES::LostSurface();
}

void GraphicsAPIGLES::RestoreSurfaceGLES() {
  egl_surface_ = InitializeEGLSurface(swapchain_format_);
  if (egl_surface_ != nullptr) {
    PlatformUtilGLES::RestoreSurface();
  } else {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "RestoreSurfaceGLES failed to create new EGLSurface");
  }
}

DisplayManager::InitSwapchainResult GraphicsAPIGLES::InitSwapchain(
    const DisplayManager::DisplayFormat& display_format,
    const DisplayManager::DisplayResolution& display_resolution,
    const DisplayManager::DisplaySwapInterval display_swap_interval,
    const uint32_t swapchain_frame_count,
    const DisplayManager::SwapchainPresentMode present_mode) {
  // Use frame_count as an 'initialized' flag, must be non-zero when swapchain is active
  if (swapchain_frame_count_ == 0 && swapchain_frame_count == kSwapchain_frame_count_gles) {
    swapchain_format_ = display_format;
    swapchain_resolution_ = display_resolution;
    swapchain_interval_ = display_swap_interval;
    swapchain_frame_count_ = swapchain_frame_count;
    swapchain_present_mode_ = present_mode;

    egl_surface_ = InitializeEGLSurface(swapchain_format_);
    if (egl_surface_ != EGL_NO_SURFACE) {
      egl_context_ = InitializeEGLContext();
      if (egl_context_ != EGL_NO_CONTEXT) {
        if (eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_) == EGL_TRUE) {
          if (PlatformUtilGLES::PlatformInitSwapchain(swapchain_interval_)) {
            return DisplayManager::kInit_Swapchain_Success;
          }
        }
        eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(egl_display_, egl_context_);
        egl_context_ = EGL_NO_CONTEXT;
        eglDestroySurface(egl_display_, egl_surface_);
        egl_surface_ = EGL_NO_SURFACE;
      } else {
        eglDestroySurface(egl_display_, egl_surface_);
        egl_surface_ = EGL_NO_SURFACE;
      }
    }
  }
  return DisplayManager::kInit_Swapchain_Failure;
}

void GraphicsAPIGLES::ShutdownSwapchain() {
  PlatformUtilGLES::PlatformShutdownSwapchain();
  eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  if (egl_context_ != EGL_NO_CONTEXT) {
    eglDestroyContext(egl_display_, egl_context_);
    egl_context_ = EGL_NO_CONTEXT;
    if (egl_surface_ != EGL_NO_SURFACE) {
      eglDestroySurface(egl_display_, egl_surface_);
      egl_surface_ = EGL_NO_SURFACE;
    }
  }
  swapchain_frame_count_ = 0;
}

bool GraphicsAPIGLES::GetSwapchainValid() {
  return (egl_context_ != EGL_NO_CONTEXT &&
          egl_surface_ != EGL_NO_SURFACE &&
          egl_display_ != EGL_NO_DISPLAY);
}

DisplayManager::SwapchainFrameHandle GraphicsAPIGLES::GetCurrentSwapchainFrame() {
  return kDefault_swapchain_handle;
}

DisplayManager::SwapchainFrameHandle GraphicsAPIGLES::PresentCurrentSwapchainFrame() {
  PlatformUtilGLES::PlatformPresentSwapchain(egl_display_, egl_surface_);
  // Check for a screen resolution change at present time and fire the callback if registered
  if (PlatformUtilGLES::CheckScreenResolutionChange(egl_display_, egl_surface_,
                                                    swapchain_resolution_)) {
    if (display_changed_callback_) {
      static bool _called = false;
      if (!_called) {
        _called = true;
        DisplayManager::DisplayChangeInfo change_info(swapchain_resolution_,
    DisplayManager::kDisplay_Change_Window_Resized);
        display_changed_callback_(change_info, display_changed_user_data_);
      }
    }
  }
  return kDefault_swapchain_handle;
}

bool GraphicsAPIGLES::GetGraphicsAPIResourcesGLES(GraphicsAPIResourcesGLES& api_resources_gles) {
  api_resources_gles.egl_context = egl_context_;
  return (egl_context_ != EGL_NO_CONTEXT);
}

bool GraphicsAPIGLES::GetSwapchainFrameResourcesGLES(
                                    const DisplayManager::SwapchainFrameHandle frame_handle,
                                    SwapchainFrameResourcesGLES& frame_resources) {
  bool valid_resources = false;
  if (frame_handle == kDefault_swapchain_handle) {
    frame_resources.egl_display = egl_display_;
    frame_resources.egl_surface = egl_surface_;
    valid_resources =  true;
  } else {
    frame_resources.egl_display = EGL_NO_DISPLAY;
    frame_resources.egl_surface = EGL_NO_SURFACE;
  }
  return valid_resources;
}

bool GraphicsAPIGLES::SetDisplayChangedCallback(DisplayManager::DisplayChangedCallback callback,
                                                void* user_data) {
  display_changed_callback_ = callback;
  display_changed_user_data_ = user_data;
  return true;
}

bool GraphicsAPIGLES::SetSwapchainChangedCallback(DisplayManager::SwapchainChangedCallback callback,
                                                  void* user_data) {
  swapchain_changed_callback_ = callback;
  swapchain_changed_user_data_ = user_data;
  return true;
}

void GraphicsAPIGLES::SwapchainChanged(const DisplayManager::SwapchainChangeMessage message) {
  if (swapchain_changed_callback_ != nullptr) {
    swapchain_changed_callback_(message, swapchain_changed_user_data_);
  }
}

} // namespace base_game_framework
