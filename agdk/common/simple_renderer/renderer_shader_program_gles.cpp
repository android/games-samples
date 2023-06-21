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

#include "renderer_shader_program_gles.h"
#include "renderer_debug.h"

namespace simple_renderer {

static bool CheckProgramStatus(GLuint program_handle) {
  GLint status = 0;
  bool valid = true;
  glGetProgramiv(program_handle, GL_LINK_STATUS, &status);
  if (status == 0) {
    char error_buffer[2048];
    valid = false;
    RENDERER_ERROR("Shader program link failed %u", program_handle)
    glGetProgramInfoLog(program_handle, sizeof(error_buffer) - 1, nullptr, error_buffer);
    RENDERER_ERROR("*** Info log:\n%s", error_buffer)
    RENDERER_ASSERT(false)
  }
  return valid;
}

static bool CheckShaderStatus(GLuint shader_handle) {
  GLint status = 0;
  bool valid = true;
  glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &status);
  if (status == 0) {
    char error_buffer[2048];
    valid = false;
    RENDERER_ERROR("Shader compile failed %u", shader_handle)
    glGetShaderInfoLog(shader_handle, sizeof(error_buffer) - 1, nullptr, error_buffer);
    RENDERER_ERROR("*** Info log:\n%s", error_buffer)
    RENDERER_ASSERT(false)
  }
  return valid;
}

ShaderProgramGLES::ShaderProgramGLES(const ShaderProgram::ShaderProgramCreationParams& params) {
  valid_program_ = false;
  fragment_handle_ = glCreateShader(GL_FRAGMENT_SHADER);
  vertex_handle_ = glCreateShader(GL_VERTEX_SHADER);
  program_handle_ = glCreateProgram();

  if (fragment_handle_ && vertex_handle_ && program_handle_) {
    // Compile vertex shader
    glShaderSource(vertex_handle_, 1,
                   reinterpret_cast<const GLchar * const *>(&params.vertex_shader_data),
                   nullptr);
    glCompileShader(vertex_handle_);
    valid_program_ = CheckShaderStatus(vertex_handle_);
    if (valid_program_) {
      // Compile fragment shader
      glShaderSource(fragment_handle_, 1,
                     reinterpret_cast<const GLchar * const *>(&params.fragment_shader_data),
                     nullptr);
      glCompileShader(fragment_handle_);
      valid_program_ = CheckShaderStatus(fragment_handle_);
      if (valid_program_) {
        // Link shaders
        glAttachShader(program_handle_, vertex_handle_);
        glAttachShader(program_handle_, fragment_handle_);
        glLinkProgram(program_handle_);
        valid_program_ = CheckProgramStatus(program_handle_);
      }
    }
  } else {
    RENDERER_ERROR("Failed to create shader program handles")
    RENDERER_ASSERT(false)
  }
}

ShaderProgramGLES::~ShaderProgramGLES() {
  if (valid_program_) {
    valid_program_ = false;
    glDetachShader(program_handle_, fragment_handle_);
    glDetachShader(program_handle_, vertex_handle_);
    glDeleteProgram(program_handle_);
    glDeleteShader(fragment_handle_);
    glDeleteShader(vertex_handle_);
    program_handle_ = 0;
    fragment_handle_ = 0;
    vertex_handle_ = 0;
  }
}

}