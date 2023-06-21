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

#include <EGL/egl.h>
#include <GLES3/gl3.h>

namespace simple_renderer {

bool RendererCheckGLES(const char* message) {
  bool no_error = true;
  GLenum error = glGetError();
  while (error != GL_NO_ERROR) {
    no_error = false;
    const char* error_string = "Unknown GL error";
    switch (error) {
      case GL_INVALID_ENUM:
        error_string = "GL_INVALID_ENUM";
        break;
      case GL_INVALID_VALUE:
        error_string = "GL_INVALID_VALUE";
        break;
      case GL_INVALID_OPERATION:
        error_string = "GL_INVALID_OPERATION";
        break;
      case GL_OUT_OF_MEMORY:
        error_string = "GL_OUT_OF_MEMORY";
        break;
      default:
        break;
    }
    RENDERER_ERROR("GL error: (%u) %s from %s", error, error_string, message)
    error = glGetError();
  }
  RENDERER_ASSERT(no_error)

  return no_error;
}

}