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

#ifndef SIMPLERENDERER_SHADER_PROGRAM_VK_H_
#define SIMPLERENDERER_SHADER_PROGRAM_VK_H_

#include <cstdint>
#include "renderer_vk_includes.h"
#include "renderer_shader_program.h"

namespace simple_renderer
{
class ShaderProgramVk : public ShaderProgram {
 public:
  ShaderProgramVk(const ShaderProgram::ShaderProgramCreationParams& params);
  virtual ~ShaderProgramVk();

  VkShaderModule GetVertexModule() const { return vertex_module_; }
  VkShaderModule GetFragmentModule() const { return fragment_module_; }

 private:
  VkShaderModule vertex_module_;
  VkShaderModule fragment_module_;
  bool valid_program_;
};
} // namespace simple_renderer

#endif // SIMPLERENDERER_SHADER_PROGRAM_VK_H_
