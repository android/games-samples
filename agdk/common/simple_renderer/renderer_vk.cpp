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

#include "renderer_vk.h"
#include "renderer_debug.h"
#include "renderer_index_buffer_vk.h"
#include "renderer_render_pass_vk.h"
#include "renderer_render_state_vk.h"
#include "renderer_shader_program_vk.h"
#include "renderer_texture_vk.h"
#include "renderer_uniform_buffer_vk.h"
#include "renderer_vertex_buffer_vk.h"
#include "display_manager.h"

using namespace base_game_framework;

namespace simple_renderer {

RendererVk& RendererVk::GetInstanceVk() {
  return *(static_cast<RendererVk*>(Renderer::GetInstancePtr()));
}

RendererVk::RendererVk() :
    staging_command_buffer_(VK_NULL_HANDLE),
    render_command_buffer_(VK_NULL_HANDLE),
    active_extent_{0, 0},
    active_frame_pool_(VK_NULL_HANDLE),
    bound_descriptor_set_(VK_NULL_HANDLE),
    bound_image_view_(VK_NULL_HANDLE),
    dirty_descriptor_set_(false),
    descriptor_pools_(),
    descriptor_set_layouts_(),
    descriptor_set_vertex_table_(VertexBuffer::kVertexFormat_Count),
    texture_descriptor_frame_cache_(kMaxSamplerDescriptors) {
  DisplayManager& display_manager = DisplayManager::GetInstance();
  const GraphicsAPIFeatures& api_features = display_manager.GetGraphicsAPIFeatures();
  display_manager.GetGraphicsAPIResourcesVk(vk_);
  // Use a per-vertex-format lookup table to retrieve the descriptor set
  // (at the moment, sampler/no sampler)
  for (size_t i = 0; i < VertexBuffer::kVertexFormat_Count; ++i) {
    descriptor_set_vertex_table_[i] = VK_NULL_HANDLE;
  }

  // Creating a pool per frame, currently our renderer only uses
  // a single sampler descriptor for draws that use a texture
  in_flight_frame_count_ = 2;
  switch (DisplayManager::GetInstance().GetDisplayBufferMode()) {
    case DisplayManager::kDisplay_Double_Buffer:
      in_flight_frame_count_ = 2;
      break;
    case DisplayManager::kDisplay_Triple_Buffer:
      in_flight_frame_count_ = 3;
      break;
  }

  CreateDescriptorPools();
  CreateCommandBuffers();

  // Grab swapchain information, but don't request a frame yet (should only happen in BeginFrame)
  const DisplayManager::SwapchainFrameHandle frame_handle =
      display_manager.GetCurrentSwapchainFrame(Renderer::GetSwapchainHandle());
  if (frame_handle != DisplayManager::kInvalid_swapchain_handle) {
    display_manager.GetSwapchainFrameResourcesVk(frame_handle, swap_, false);
  }
}

RendererVk::~RendererVk() {
}

void RendererVk::PrepareShutdown() {
  render_pass_ = nullptr;
  render_state_ = nullptr;
  resources_.ProcessDeleteQueue();

  DestroyCommandBuffers();

  for (const VkDescriptorSetLayout layout : descriptor_set_layouts_) {
    vkDestroyDescriptorSetLayout(vk_.device, layout, nullptr);
  }
  descriptor_set_layouts_.clear();

  for (const VkDescriptorPool pool : descriptor_pools_) {
    vkDestroyDescriptorPool(vk_.device, pool, nullptr);
  }
  descriptor_pools_.clear();
}

bool RendererVk::GetFeatureAvailable(const RendererFeature feature) {
  bool supported = false;
  switch (feature) {
    case Renderer::kFeature_ASTC:
      // Android Baseline Profile requires ASTC support, we wouldn't have
      // initialized RendererVk without it
      supported = true;
      break;
    default:
      break;
  }
  return supported;
}

void RendererVk::BeginFrame(
    const base_game_framework::DisplayManager::SwapchainHandle swapchain_handle) {
  resources_.ProcessDeleteQueue();
  // At the moment, we don't support render targets, so grab a swapchain image
  // as soon as we start a frame
  DisplayManager& display_manager = DisplayManager::GetInstance();
  const DisplayManager::SwapchainFrameHandle frame_handle =
      display_manager.GetCurrentSwapchainFrame(swapchain_handle);
  if (frame_handle != DisplayManager::kInvalid_swapchain_handle) {
    display_manager.GetSwapchainFrameResourcesVk(frame_handle, swap_, true);
    active_extent_ = swap_.swapchain_extent;

    RENDERER_ASSERT(swap_.swapchain_frame_index < descriptor_pools_.size())
    active_frame_pool_ = descriptor_pools_[swap_.swapchain_frame_index];
    VkResult reset_result = vkResetDescriptorPool(vk_.device, active_frame_pool_, 0);
    RENDERER_CHECK_VK(reset_result, "vkResetDescriptorPool");
  }

  render_command_buffer_ = command_buffers_[swap_.swapchain_frame_index];

  const VkResult reset_command_result = vkResetCommandBuffer(render_command_buffer_, 0);
  RENDERER_CHECK_VK(reset_command_result, "vkResetCommandBuffer");

  VkCommandBufferBeginInfo command_buffer_begin_info = {};
  command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  const VkResult begin_command_result = vkBeginCommandBuffer(render_command_buffer_,
                                                             &command_buffer_begin_info);
  RENDERER_CHECK_VK(begin_command_result, "vkBeginCommandBuffer");

  // We enabled dynamic viewport and width in the pipeline object,
  // so set them at the beginning of our render command buffer

  VkViewport viewport{};
  viewport.width = static_cast<float>(active_extent_.width);
  // SimpleRenderer assumes GL style Y axis points up. Vulkan is Y axis points
  // down. The VK_KHR_MAINTENANCE1 extension lets us negate the viewport
  // height to flip the Y axis, we also have to set Y = height instead of 0
  viewport.y = static_cast<float>(active_extent_.height);
  viewport.height = -(static_cast<float>(active_extent_.height));
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(render_command_buffer_, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.extent = active_extent_;
  vkCmdSetScissor(render_command_buffer_, 0, 1, &scissor);
}

void RendererVk::EndFrame() {
  if (render_pass_.get() != nullptr) {
    render_pass_.get()->EndRenderPass();
    render_pass_ = nullptr;
  }
  render_state_ = nullptr;
  vkEndCommandBuffer(render_command_buffer_);

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore signal_semaphores[] = {swap_.render_complete};
  VkSemaphore wait_semaphores[] = {swap_.image_available};
  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &render_command_buffer_;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signal_semaphores;

  const VkResult queue_result = vkQueueSubmit(vk_.render_queue, 1, &submit_info, swap_.frame_fence);
  RENDERER_CHECK_VK(queue_result, "vkQueueSubmit");

  active_frame_pool_ = VK_NULL_HANDLE;
  texture_descriptor_frame_cache_.clear();
  bound_descriptor_set_ = VK_NULL_HANDLE;
  bound_image_view_ = VK_NULL_HANDLE;
}

void RendererVk::SwapchainRecreated() {
  // Our cached framebuffers were associated with image view from the old
  // swapchain, purge the cache to rebuild them using the new swapchain
  for (auto &render_pass : resources_.GetRenderPasses()) {
    RenderPassVk *render_pass_vk = reinterpret_cast<RenderPassVk*>(render_pass.second.get());
    render_pass_vk->PurgeFramebufferCache();
  }
}

void RendererVk::Draw(const uint32_t vertex_count, const uint32_t first_vertex) {
  RenderStateVk& state = *(static_cast<RenderStateVk*>(render_state_.get()));
  if (dirty_descriptor_set_) {
    vkCmdBindDescriptorSets(render_command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            state.GetPipelineLayout(), 0, 1, &bound_descriptor_set_,
                            0, nullptr);
    dirty_descriptor_set_ = false;
  }

  // Update any uniform data that might have changed between draw calls
  state.UpdateUniformData(render_command_buffer_, true);

  vkCmdDraw(render_command_buffer_, vertex_count, 1, first_vertex, 0);
}

void RendererVk::DrawIndexed(const uint32_t index_count, const uint32_t first_index) {
  RenderStateVk& state = *(static_cast<RenderStateVk*>(render_state_.get()));
  if (dirty_descriptor_set_) {
    vkCmdBindDescriptorSets(render_command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            state.GetPipelineLayout(), 0, 1, &bound_descriptor_set_,
                            0, nullptr);
    dirty_descriptor_set_ = false;
  }

  // Update any uniform data that might have changed between draw calls
  state.UpdateUniformData(render_command_buffer_, true);

  vkCmdDrawIndexed(render_command_buffer_, index_count, 1, first_index, 0, 0);
}

void RendererVk::SetRenderPass(std::shared_ptr<RenderPass> render_pass) {
  RenderPass* new_render_pass = render_pass.get();
  RenderPass* old_render_pass = render_pass_.get();
  if (new_render_pass != old_render_pass) {
    if (old_render_pass != nullptr) {
      old_render_pass->EndRenderPass();
    }
    render_pass_ = render_pass;
    render_state_ = nullptr;
    new_render_pass->BeginRenderPass();
  }
}

void RendererVk::SetRenderState(std::shared_ptr<RenderState> render_state) {
  if (render_state.get() != render_state_.get()) {
    render_state_ = render_state;
    RenderStateVk& state = *(static_cast<RenderStateVk*>(render_state_.get()));
    vkCmdBindPipeline(render_command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, state.GetPipeline());
    bound_descriptor_set_ = VK_NULL_HANDLE;
    bound_image_view_ = VK_NULL_HANDLE;
  }
}

void RendererVk::BindIndexBuffer(std::shared_ptr<IndexBuffer> index_buffer) {
  IndexBufferVk& index_buffer_vk = *(static_cast<IndexBufferVk*>(index_buffer.get()));
  vkCmdBindIndexBuffer(render_command_buffer_, index_buffer_vk.GetIndexBuffer(),
                       0, VK_INDEX_TYPE_UINT16);
}

void RendererVk::BindVertexBuffer(std::shared_ptr<VertexBuffer> vertex_buffer) {
  VertexBufferVk& vertex_buffer_vk = *(static_cast<VertexBufferVk*>(vertex_buffer.get()));
  VkBuffer vertex_buffers[] = {vertex_buffer_vk.GetVertexBuffer()};
  VkDeviceSize vertex_offsets[] = {0};
  vkCmdBindVertexBuffers(render_command_buffer_, 0, 1, vertex_buffers, vertex_offsets);
}

void RendererVk::BindTexture(std::shared_ptr<Texture> texture) {
  if (texture.get() == nullptr) {
    bound_descriptor_set_ = VK_NULL_HANDLE;
    bound_image_view_ = VK_NULL_HANDLE;
    dirty_descriptor_set_ = true;
  }

  TextureVk& texture_vk = *(static_cast<TextureVk*>(texture.get()));
  const VkImageView texture_image_view = texture_vk.GetImageView();
  if (texture_image_view == bound_image_view_) {
    return;
  }
  bound_image_view_ = texture_image_view;
  for (const TextureDescriptorFrameCache& cache : texture_descriptor_frame_cache_) {
    if (cache.texture_image_view == texture_image_view) {
      bound_descriptor_set_ = cache.descriptor_set;
      dirty_descriptor_set_ = true;
      return;
    }
  }

  // Create a new descriptor/descriptor set for this texture for this frame and
  // write the descriptor data
  RenderStateVk& state = *(static_cast<RenderStateVk*>(render_state_.get()));

  VkDescriptorSetLayout descriptor_set_layouts[] = {state.GetDescriptorSetLayout() };
  VkDescriptorSetAllocateInfo descriptor_set_info = {};
  descriptor_set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptor_set_info.descriptorPool = active_frame_pool_;
  descriptor_set_info.descriptorSetCount = 1;
  descriptor_set_info.pSetLayouts = descriptor_set_layouts;
  const VkResult allocate_result = vkAllocateDescriptorSets(vk_.device, &descriptor_set_info,
                                                            &bound_descriptor_set_);
  RENDERER_CHECK_VK(allocate_result, "vkAllocateDescriptorSets");

  VkDescriptorImageInfo descriptor_image_info = {};
  descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  descriptor_image_info.imageView = texture_vk.GetImageView();
  descriptor_image_info.sampler = texture_vk.GetSampler();

  VkWriteDescriptorSet write_descriptor_set = {};
  write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_set.dstSet = bound_descriptor_set_;
  write_descriptor_set.dstBinding = 1;
  write_descriptor_set.dstArrayElement = 0;
  write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write_descriptor_set.descriptorCount = 1;
  write_descriptor_set.pImageInfo = &descriptor_image_info;

  vkUpdateDescriptorSets(vk_.device, 1, &write_descriptor_set, 0, nullptr);
  dirty_descriptor_set_ = true;

  texture_descriptor_frame_cache_.push_back(
      {texture_vk.GetImageView(), bound_descriptor_set_});
}

std::shared_ptr<IndexBuffer> RendererVk::CreateIndexBuffer(
    const IndexBuffer::IndexBufferCreationParams& params) {
  return resources_.AddIndexBuffer(new IndexBufferVk(params));
}

void RendererVk::DestroyIndexBuffer(std::shared_ptr<IndexBuffer> index_buffer) {
  resources_.QueueDeleteIndexBuffer(index_buffer);
}

std::shared_ptr<RenderPass> RendererVk::CreateRenderPass(
    const RenderPass::RenderPassCreationParams& params) {
  return resources_.AddRenderPass(new RenderPassVk(params));
}

void RendererVk::DestroyRenderPass(std::shared_ptr<RenderPass> render_pass) {
  resources_.QueueDeleteRenderPass(render_pass);
}

std::shared_ptr<RenderState> RendererVk::CreateRenderState(
    const RenderState::RenderStateCreationParams& params) {
  return resources_.AddRenderState(new RenderStateVk(params));
}

void RendererVk::DestroyRenderState(std::shared_ptr<RenderState> render_state) {
  resources_.QueueDeleteRenderState(render_state);
}

std::shared_ptr<ShaderProgram> RendererVk::CreateShaderProgram(
    const ShaderProgram::ShaderProgramCreationParams& params) {
  return resources_.AddShaderProgram(new ShaderProgramVk(params));
}

void RendererVk::DestroyShaderProgram(std::shared_ptr<ShaderProgram> shader_program) {
  resources_.QueueDeleteShaderProgram(shader_program);
}

std::shared_ptr<Texture> RendererVk::CreateTexture(const Texture::TextureCreationParams& params) {
  return resources_.AddTexture(new TextureVk(params));
}

void RendererVk::DestroyTexture(std::shared_ptr<Texture> texture) {
  resources_.QueueDeleteTexture(texture);
}

std::shared_ptr<UniformBuffer> RendererVk::CreateUniformBuffer(
    const UniformBuffer::UniformBufferCreationParams& params) {
  return resources_.AddUniformBuffer(new UniformBufferVk(params));
}

void RendererVk::DestroyUniformBuffer(std::shared_ptr<UniformBuffer> uniform_buffer) {
  resources_.QueueDeleteUniformBuffer(uniform_buffer);
}

std::shared_ptr<VertexBuffer> RendererVk::CreateVertexBuffer(
    const VertexBuffer::VertexBufferCreationParams& params) {
  return resources_.AddVertexBuffer(new VertexBufferVk(params));
}

void RendererVk::DestroyVertexBuffer(std::shared_ptr<VertexBuffer> vertex_buffer) {
  resources_.QueueDeleteVertexBuffer(vertex_buffer);
}

VkDescriptorSetLayout RendererVk::GetDescriptorSetLayout(
    const VertexBuffer::VertexFormat vertex_format) {
  if (vertex_format == VertexBuffer::kVertexFormat_Count) {
    return VK_NULL_HANDLE;
  }
  if (descriptor_set_vertex_table_[vertex_format] == VK_NULL_HANDLE) {
    // Doesn't exist yet for this vertex format, allocate a new one
    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
    bool has_texture = false;

    VkDescriptorSetLayoutBinding sampler_layout_binding = {};
    VkDescriptorSetLayoutCreateInfo
        descriptor_set_layout_info = {};
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    if (vertex_format == VertexBuffer::kVertexFormat_P3T2 ||
        vertex_format == VertexBuffer::kVertexFormat_P3T2C4) {
      // If we have a texture vertex format, assume we need to hook up a sampler,
      // and we need a descriptor set and descriptor to specify it.
      // Since we are using push constants, we don't need to configure a uniform or
      // storage buffer descriptor.

      sampler_layout_binding.binding = 1;
      sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      sampler_layout_binding.descriptorCount = 1;
      sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

      descriptor_set_layout_info.bindingCount = 1;
      descriptor_set_layout_info.pBindings = &sampler_layout_binding;
      has_texture = true;
    }
    const VkResult layout_result = vkCreateDescriptorSetLayout(vk_.device,
                                                               &descriptor_set_layout_info,
                                                               nullptr, &descriptor_set_layout);
    RENDERER_CHECK_VK(layout_result, "vkCreateDescriptorSetLayout");

    // We technically only need two layouts, one for a texture (sampler) and one without
    // so duplicate across the matching formats
    if (has_texture) {
      descriptor_set_vertex_table_[VertexBuffer::kVertexFormat_P3T2] = descriptor_set_layout;
      descriptor_set_vertex_table_[VertexBuffer::kVertexFormat_P3T2C4] = descriptor_set_layout;
    } else {
      descriptor_set_vertex_table_[VertexBuffer::kVertexFormat_P3] = descriptor_set_layout;
      descriptor_set_vertex_table_[VertexBuffer::kVertexFormat_P3C4] = descriptor_set_layout;
    }
    // Keep track of uniques for disposal at shutdown
    if (descriptor_set_layout != VK_NULL_HANDLE) {
      descriptor_set_layouts_.push_back(descriptor_set_layout);
    }
  }
  return descriptor_set_vertex_table_[vertex_format];
}

VkCommandBuffer RendererVk::BeginStagingCommandBuffer() {
  RENDERER_ASSERT(staging_command_buffer_ != nullptr)

  VkCommandBufferBeginInfo begin_info = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      nullptr,
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      nullptr};
  vkResetCommandBuffer(staging_command_buffer_, 0);
  const VkResult begin_result = vkBeginCommandBuffer(staging_command_buffer_, &begin_info);
  RENDERER_CHECK_VK(begin_result, "vkBeginCommandBuffer (BeginStagingCommandBuffer)");
  return staging_command_buffer_;
}

void RendererVk::EndStagingCommandBuffer() {
  RENDERER_ASSERT(staging_command_buffer_ != nullptr)
  const VkResult end_result = vkEndCommandBuffer(staging_command_buffer_);
  RENDERER_CHECK_VK(end_result, "vkEndCommandBuffer (EndStagingCommandBuffer");

  VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &staging_command_buffer_;
  const VkResult submit_result = vkQueueSubmit(vk_.render_queue, 1, &submit_info, VK_NULL_HANDLE);
  RENDERER_CHECK_VK(submit_result, "vkQueueSubmit (EndStagingCommandBuffer)");

  const VkResult wait_result = vkQueueWaitIdle(vk_.render_queue);
  RENDERER_CHECK_VK(wait_result, "vkQueueWaitIdle (EndStagingCommandBuffer)");
}

void RendererVk::CreateCommandBuffers() {
  command_buffers_.resize(in_flight_frame_count_);

  VkCommandPoolCreateInfo command_pool_info = {};
  command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  command_pool_info.queueFamilyIndex = vk_.graphics_queue_index;
  command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  VkResult result = vkCreateCommandPool(vk_.device, &command_pool_info, nullptr, &command_pool_);
  RENDERER_CHECK_VK(result, "vkCreateCommandPool");

  VkCommandBufferAllocateInfo command_buffer_info = {};
  command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_info.commandPool = command_pool_;
  command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command_buffer_info.commandBufferCount = in_flight_frame_count_;
  result = vkAllocateCommandBuffers(vk_.device, &command_buffer_info, command_buffers_.data());
  RENDERER_CHECK_VK(result, "vkAllocateCommandBuffers");

  command_buffer_info.commandBufferCount = 1;
  result = vkAllocateCommandBuffers(vk_.device, &command_buffer_info, &staging_command_buffer_);
  RENDERER_CHECK_VK(result, "vkAllocateCommandBuffers");
}

void RendererVk::DestroyCommandBuffers() {
  vkFreeCommandBuffers(vk_.device, command_pool_, command_buffers_.size(), command_buffers_.data());
  command_buffers_.clear();
  vkFreeCommandBuffers(vk_.device, command_pool_, 1, &staging_command_buffer_);
  staging_command_buffer_ = VK_NULL_HANDLE;
  vkDestroyCommandPool(vk_.device, command_pool_, nullptr);
  command_pool_ = VK_NULL_HANDLE;
}

void RendererVk::CreateDescriptorPools() {
  descriptor_pools_.resize(in_flight_frame_count_);

  VkDescriptorPoolSize pool_size{};
  pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_size.descriptorCount = kMaxSamplerDescriptors;

  VkDescriptorPoolCreateInfo pool_create_info{};
  pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_create_info.poolSizeCount = 1;
  pool_create_info.pPoolSizes = &pool_size;
  pool_create_info.maxSets = kMaxSamplerDescriptors;

  for (uint32_t i = 0; i < in_flight_frame_count_; ++i) {
    VkDescriptorPool pool;
    const VkResult pool_result = vkCreateDescriptorPool(vk_.device, &pool_create_info,
                                                        nullptr, &pool);
    RENDERER_CHECK_VK(pool_result, "vkCreateDescriptorPool");
    if (pool_result == VK_SUCCESS) {
      descriptor_pools_[i] = pool;
    }
  }
}

}
