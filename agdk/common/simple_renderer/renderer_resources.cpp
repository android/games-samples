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

#include "renderer_resources.h"
#include "renderer_debug.h"

namespace simple_renderer {

static constexpr long kExpectedUseCount = 1;

void RendererResources::ProcessDeleteQueue() {
  while (!index_buffer_delete_queue_.empty()) {
    RENDERER_ASSERT(index_buffer_delete_queue_.front().use_count() == kExpectedUseCount)
    index_buffer_delete_queue_.pop();
  }

  while (!render_state_delete_queue_.empty()) {
    RENDERER_ASSERT(render_state_delete_queue_.front().use_count() == kExpectedUseCount)
    render_state_delete_queue_.pop();
  }

  while (!render_pass_delete_queue_.empty()) {
    RENDERER_ASSERT(render_pass_delete_queue_.front().use_count() == kExpectedUseCount)
    render_pass_delete_queue_.pop();
  }

  while (!shader_program_delete_queue_.empty()) {
    RENDERER_ASSERT(shader_program_delete_queue_.front().use_count() == kExpectedUseCount)
    shader_program_delete_queue_.pop();
  }

  while (!texture_delete_queue_.empty()) {
    RENDERER_ASSERT(texture_delete_queue_.front().use_count() == kExpectedUseCount)
    texture_delete_queue_.pop();
  }

  while (!uniform_buffer_delete_queue_.empty()) {
    RENDERER_ASSERT(uniform_buffer_delete_queue_.front().use_count() == kExpectedUseCount)
    uniform_buffer_delete_queue_.pop();
  }

  while (!vertex_buffer_delete_queue_.empty()) {
    RENDERER_ASSERT(vertex_buffer_delete_queue_.front().use_count() == kExpectedUseCount)
    vertex_buffer_delete_queue_.pop();
  }
}

std::shared_ptr<IndexBuffer> RendererResources::AddIndexBuffer(IndexBuffer* index_buffer) {
  std::shared_ptr<IndexBuffer> share = std::shared_ptr<IndexBuffer>(index_buffer);
  index_buffers_.insert({reinterpret_cast<RendererKey>(index_buffer), share});
  return share;
}

void RendererResources::QueueDeleteIndexBuffer(const std::shared_ptr<IndexBuffer>& index_buffer) {
  const RendererKey buffer_key = reinterpret_cast<RendererKey>(index_buffer.get());
  auto iter = index_buffers_.find(buffer_key);
  if (iter != index_buffers_.end()) {
    index_buffer_delete_queue_.push(iter->second);
    index_buffers_.erase(iter);
  }

}

std::shared_ptr<RenderPass> RendererResources::AddRenderPass(RenderPass* render_pass) {
  std::shared_ptr<RenderPass> share = std::shared_ptr<RenderPass>(render_pass);
  render_passes_.insert({reinterpret_cast<RendererKey>(render_pass), share});
  return share;

}

void RendererResources::QueueDeleteRenderPass(const std::shared_ptr<RenderPass>& render_pass) {
  const RendererKey pass_key = reinterpret_cast<RendererKey>(render_pass.get());
  auto iter = render_passes_.find(pass_key);
  if (iter != render_passes_.end()) {
    render_pass_delete_queue_.push(iter->second);
    render_passes_.erase(iter);
  }
}

std::shared_ptr<RenderState> RendererResources::AddRenderState(RenderState* render_state) {
  std::shared_ptr<RenderState> share = std::shared_ptr<RenderState>(render_state);
  render_states_.insert({reinterpret_cast<RendererKey>(render_state),share});
  return share;

}

void RendererResources::QueueDeleteRenderState(const std::shared_ptr<RenderState>& render_state) {
  const RendererKey state_key = reinterpret_cast<RendererKey>(render_state.get());
  auto iter = render_states_.find(state_key);
  if (iter != render_states_.end()) {
    render_state_delete_queue_.push(iter->second);
    render_states_.erase(iter);
  }
}

std::shared_ptr<ShaderProgram> RendererResources::AddShaderProgram(ShaderProgram* shader_program) {
  std::shared_ptr<ShaderProgram> share = std::shared_ptr<ShaderProgram>(shader_program);
  shader_programs_.insert({reinterpret_cast<RendererKey>(shader_program),share});
  return share;
}

void RendererResources::QueueDeleteShaderProgram(const std::shared_ptr<ShaderProgram>&
    shader_program) {
  const RendererKey shader_key = reinterpret_cast<RendererKey>(shader_program.get());
  auto iter = shader_programs_.find(shader_key);
  if (iter != shader_programs_.end()) {
    shader_program_delete_queue_.push(iter->second);
    shader_programs_.erase(iter);
  }
}

std::shared_ptr<Texture> RendererResources::AddTexture(Texture* texture) {
  std::shared_ptr<Texture> share = std::shared_ptr<Texture>(texture);
  textures_.insert({reinterpret_cast<RendererKey>(texture), share});
  return share;
}

void RendererResources::QueueDeleteTexture(const std::shared_ptr<Texture>& texture) {
  const RendererKey texture_key = reinterpret_cast<RendererKey>(texture.get());
  auto iter = textures_.find(texture_key);
  if (iter != textures_.end()) {
    texture_delete_queue_.push(iter->second);
    textures_.erase(iter);
  }
}

std::shared_ptr<UniformBuffer> RendererResources::AddUniformBuffer(UniformBuffer* uniform_buffer) {
  std::shared_ptr<UniformBuffer> share = std::shared_ptr<UniformBuffer>(uniform_buffer);
  uniform_buffers_.insert({reinterpret_cast<RendererKey>(uniform_buffer), share});
  return share;
}

void RendererResources::QueueDeleteUniformBuffer(const std::shared_ptr<UniformBuffer>&
    uniform_buffer) {
  const RendererKey uniform_key = reinterpret_cast<RendererKey>(uniform_buffer.get());
  auto iter = uniform_buffers_.find(uniform_key);
  if (iter != uniform_buffers_.end()) {
    uniform_buffer_delete_queue_.push(iter->second);
    uniform_buffers_.erase(iter);
  }
}

std::shared_ptr<VertexBuffer> RendererResources::AddVertexBuffer(VertexBuffer* vertex_buffer) {
  std::shared_ptr<VertexBuffer> share = std::shared_ptr<VertexBuffer>(vertex_buffer);
  vertex_buffers_.insert({reinterpret_cast<RendererKey>(vertex_buffer), share});
  return share;
}

void RendererResources::QueueDeleteVertexBuffer(const std::shared_ptr<VertexBuffer>&
    vertex_buffer) {
  const RendererKey buffer_key = reinterpret_cast<RendererKey>(vertex_buffer.get());
  auto iter = vertex_buffers_.find(buffer_key);
  if (iter != vertex_buffers_.end()) {
    vertex_buffer_delete_queue_.push(iter->second);
    vertex_buffers_.erase(iter);
  }
}

}