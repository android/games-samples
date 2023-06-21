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

#ifndef SIMPLERENDERER_SHADER_PROGRAM_H_
#define SIMPLERENDERER_SHADER_PROGRAM_H_

#include <cstdint>
#include <string>

namespace simple_renderer
{
/**
 * @brief The base class definition for the `ShaderProgram` class of SimpleRenderer.
 * Use the `Renderer` class interface to create and destroy `ShaderProgram` objects.
 * Currently all shader programs consist of a paired vertex shader and fragment shader.
 */
class ShaderProgram {
 public:
  /**
   * @brief A structure holding required parameters to create a new `ShaderProgram`.
   * Passed to the Renderer::CreateShaderProgram function.
   */
  struct ShaderProgramCreationParams {
    /** @brief A pointer to an array of fragment shader data */
    void* fragment_shader_data;
    /** @brief A pointer to an array of vertex shader data */
    void* vertex_shader_data;
    /** @brief Size of the fragment shader data array in bytes */
    size_t fragment_data_byte_count;
    /** @brief Size of the vertex shader data array in bytes */
    size_t vertex_data_byte_count;
  };

  /**
   * @brief Retrieve the debug name string associated with the fragment shader
   * @result A string containing the debug name.
   */
  const std::string &GetFragmentDebugName() const { return fragment_debug_name_; }
  /**
   * @brief Retrieve the debug name string associated with the vertex shader
   * @result A string containing the debug name.
   */
  const std::string &GetVertexDebugName() const { return vertex_debug_name_; }
  /**
   * @brief Set a debug name string to associate with the fragment shader
   * @param name A string containing the debug name.
   */
  void SetFragmentDebugName(const std::string& name) { fragment_debug_name_ = name; }
  /**
   * @brief Set a debug name string to associate with the vertex shader
   * @param name A string containing the debug name.
   */
  void SetVertexDebugName(const std::string& name) { vertex_debug_name_ = name; }

  /**
   * @brief Base class destructor, do not call directly.
   */
  virtual ~ShaderProgram() {}

 protected:
  ShaderProgram() {
    fragment_debug_name_ = "noname";
    vertex_debug_name_ = "noname";
  }

 private:
  std::string fragment_debug_name_;
  std::string vertex_debug_name_;
};
}

#endif // SIMPLERENDERER_SHADER_PROGRAM_H_
