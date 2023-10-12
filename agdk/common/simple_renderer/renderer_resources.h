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

#ifndef SIMPLERENDERER_RESOURCES_H_
#define SIMPLERENDERER_RESOURCES_H_

#include "renderer_interface.h"
#include <queue>
#include <unordered_map>

namespace simple_renderer {

typedef uint64_t RendererKey;

class RendererResources {
 public:
  void ProcessDeleteQueue();

  std::shared_ptr<IndexBuffer> AddIndexBuffer(IndexBuffer* index_buffer);
  void QueueDeleteIndexBuffer(const std::shared_ptr<IndexBuffer>& index_buffer);

  std::shared_ptr<RenderPass> AddRenderPass(RenderPass* render_pass);
  void QueueDeleteRenderPass(const std::shared_ptr<RenderPass>& render_pass);

  std::shared_ptr<RenderState> AddRenderState(RenderState* render_state);
  void QueueDeleteRenderState(const std::shared_ptr<RenderState>& render_state);

  std::shared_ptr<ShaderProgram> AddShaderProgram(ShaderProgram* shader_program);
  void QueueDeleteShaderProgram(const std::shared_ptr<ShaderProgram>& shader_program);

  std::shared_ptr<Texture> AddTexture(Texture* texture);
  void QueueDeleteTexture(const std::shared_ptr<Texture>& texture);

  std::shared_ptr<UniformBuffer> AddUniformBuffer(UniformBuffer* uniform_buffer);
  void QueueDeleteUniformBuffer(const std::shared_ptr<UniformBuffer>& uniform_buffer);

  std::shared_ptr<VertexBuffer> AddVertexBuffer(VertexBuffer* vertex_buffer);
  void QueueDeleteVertexBuffer(const std::shared_ptr<VertexBuffer>& vertex_buffer);

  auto &GetRenderPasses() {
    return render_passes_;
  }

 private:
  std::unordered_map<RendererKey, std::shared_ptr<IndexBuffer> > index_buffers_;
  std::unordered_map<RendererKey, std::shared_ptr<RenderPass> > render_passes_;
  std::unordered_map<RendererKey, std::shared_ptr<RenderState> > render_states_;
  std::unordered_map<RendererKey, std::shared_ptr<ShaderProgram> > shader_programs_;
  std::unordered_map<RendererKey, std::shared_ptr<Texture> > textures_;
  std::unordered_map<RendererKey, std::shared_ptr<UniformBuffer> > uniform_buffers_;
  std::unordered_map<RendererKey, std::shared_ptr<VertexBuffer> > vertex_buffers_;

  std::queue< std::shared_ptr<IndexBuffer> > index_buffer_delete_queue_;
  std::queue< std::shared_ptr<RenderPass> > render_pass_delete_queue_;
  std::queue< std::shared_ptr<RenderState> > render_state_delete_queue_;
  std::queue< std::shared_ptr<ShaderProgram> > shader_program_delete_queue_;
  std::queue< std::shared_ptr<Texture> > texture_delete_queue_;
  std::queue< std::shared_ptr<UniformBuffer> > uniform_buffer_delete_queue_;
  std::queue< std::shared_ptr<VertexBuffer> > vertex_buffer_delete_queue_;
};

}

#endif // SIMPLERENDERER_RESOURCES_H_
