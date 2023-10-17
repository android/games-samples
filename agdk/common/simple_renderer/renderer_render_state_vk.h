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

#ifndef SIMPLERENDERER_RENDER_STATE_VK_H_
#define SIMPLERENDERER_RENDER_STATE_VK_H_

#include "renderer_render_state.h"
#include "renderer_vk_includes.h"

namespace simple_renderer {

class RenderStateVk : public RenderState {
 public:
  RenderStateVk(const RenderStateCreationParams& params);
  virtual ~RenderStateVk();

  virtual void SetViewport(const RenderState::Viewport& viewport) {
    viewport_ = viewport;
  }

  virtual void SetScissorRect(const RenderState::ScissorRect& scissor_rect) {
    scissor_rect_ = scissor_rect;
  }

  VkDescriptorSetLayout GetDescriptorSetLayout() const { return descriptor_set_layout_; }

  VkPipeline GetPipeline() const { return pipeline_; }

  VkPipelineLayout GetPipelineLayout() const { return pipeline_layout_; }

  void UpdateUniformData(VkCommandBuffer command_buffer, bool force_update);

 private:
  void CreatePipelineLayout(const RenderStateCreationParams& params);

  RenderState::ScissorRect scissor_rect_;
  RenderState::Viewport viewport_;
  std::shared_ptr<RenderPass> render_pass_;
  std::shared_ptr<ShaderProgram> state_program_;
  std::shared_ptr<UniformBuffer> state_uniform_;
  VkDescriptorSetLayout descriptor_set_layout_;
  VkPipeline pipeline_;
  VkPipelineLayout pipeline_layout_;
  VertexBuffer::VertexFormat state_vertex_layout_;
  VkPrimitiveTopology primitive_type_;
};

}

#endif // SIMPLERENDERER_RENDER_STATE_VK_H_