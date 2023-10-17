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

#include "renderer_shader_program_vk.h"
#include "renderer_debug.h"
#include "renderer_vk.h"

namespace simple_renderer {

ShaderProgramVk::ShaderProgramVk(const ShaderProgram::ShaderProgramCreationParams& params) {
  RendererVk& renderer = RendererVk::GetInstanceVk();
  valid_program_ = false;
  // Input SPIR-V code must be 32 bit aligned
  RENDERER_ASSERT((((uint64_t)params.vertex_shader_data) & 0x3) == 0)
  RENDERER_ASSERT((((uint64_t)params.fragment_shader_data) & 0x3) == 0)

  VkShaderModuleCreateInfo vertex_create_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
  vertex_create_info.codeSize = params.vertex_data_byte_count;
  vertex_create_info.pCode = reinterpret_cast<const uint32_t*>(params.vertex_shader_data);
  VkResult vertex_create_result = vkCreateShaderModule(renderer.GetDevice(), &vertex_create_info,
                                                       nullptr, &vertex_module_);
  RENDERER_CHECK_VK(vertex_create_result, "vkCreateShaderModule (vertex)");

  VkShaderModuleCreateInfo fragment_create_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
  fragment_create_info.codeSize = params.fragment_data_byte_count;
  fragment_create_info.pCode = reinterpret_cast<const uint32_t*>(params.fragment_shader_data);
  VkResult fragment_create_result = vkCreateShaderModule(renderer.GetDevice(), &fragment_create_info,
                                                       nullptr, &fragment_module_);
  RENDERER_CHECK_VK(fragment_create_result, "vkCreateShaderModule (fragment)");
  valid_program_ = (vertex_create_result == VK_SUCCESS && fragment_create_result == VK_SUCCESS);
}

ShaderProgramVk::~ShaderProgramVk() {
  RendererVk& renderer = RendererVk::GetInstanceVk();
  if (valid_program_) {
    vkDestroyShaderModule(renderer.GetDevice(), vertex_module_, nullptr);
    vkDestroyShaderModule(renderer.GetDevice(), fragment_module_, nullptr);
    vertex_module_ = VK_NULL_HANDLE;
    fragment_module_ = VK_NULL_HANDLE;
    valid_program_ = false;
  }
}

}