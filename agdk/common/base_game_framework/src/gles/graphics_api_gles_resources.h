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

#ifndef BASEGAMEFRAMEWORK_GRAPHICSAPI_GLES_RESOURCES_H_
#define BASEGAMEFRAMEWORK_GRAPHICSAPI_GLES_RESOURCES_H_

#include "platform_defines.h"
#include <cstdint>

namespace base_game_framework {

struct GraphicsAPIResourcesGLES {
  EGLContext egl_context = EGL_NO_CONTEXT;
};

struct SwapchainFrameResourcesGLES {
  EGLDisplay egl_display = EGL_NO_DISPLAY;
  EGLSurface egl_surface = EGL_NO_SURFACE;
};

} // namespace base_game_framework

#endif // BASEGAMEFRAMEWORK_GRAPHICSAPI_GLES_RESOURCES_H_
