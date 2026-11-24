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
#include <algorithm>
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

// Frames in flight: the swapchain's, or if there is no swapchain yet, from the display buffer
// mode (as GraphicsAPIVulkan sizes its frame fences)
static uint32_t GetInFlightFrameCount() {
  DisplayManager& display_manager = DisplayManager::GetInstance();
  const DisplayManager::SwapchainFrameHandle frame_handle =
      display_manager.GetCurrentSwapchainFrame(Renderer::GetSwapchainHandle());
  SwapchainFrameResourcesVk frame_resources;
  if (frame_handle != DisplayManager::kInvalid_swapchain_handle &&
      display_manager.GetSwapchainFrameResourcesVk(frame_handle, frame_resources, false) &&
      frame_resources.swapchain_frame_count != 0) {
    return frame_resources.swapchain_frame_count;
  }
  return (display_manager.GetDisplayBufferMode() ==
          DisplayManager::kDisplay_Triple_Buffer) ? 3 : 2;
}

RendererVk::RendererVk() :
    // A released resource is deleted once every frame in flight was waited after its release
    resources_(GetInFlightFrameCount()),
    staging_command_buffer_(VK_NULL_HANDLE),
    render_command_buffer_(VK_NULL_HANDLE),
    frame_active_(false),
    frame_acquired_(false),
    active_extent_{0, 0},
    active_frame_pool_(VK_NULL_HANDLE),
    active_rotation_(DisplayManager::kSwapchain_Rotation_None),
    bound_descriptor_set_(VK_NULL_HANDLE),
    bound_image_view_(VK_NULL_HANDLE),
    dirty_descriptor_set_(false),
    descriptor_pools_(),
    descriptor_set_layouts_(),
    descriptor_set_vertex_table_(VertexBuffer::kVertexFormat_Count),
    texture_descriptor_frame_cache_() {
  // Reserve space up front; the cache starts empty each frame
  texture_descriptor_frame_cache_.reserve(kMaxSamplerDescriptors);
  DisplayManager& display_manager = DisplayManager::GetInstance();
  display_manager.GetGraphicsAPIResourcesVk(vk_);
  // Viewport limits, for clamping render state viewports
  VkPhysicalDeviceProperties device_properties{};
  vkGetPhysicalDeviceProperties(vk_.physical_device, &device_properties);
  max_viewport_dimensions_[0] = device_properties.limits.maxViewportDimensions[0];
  max_viewport_dimensions_[1] = device_properties.limits.maxViewportDimensions[1];
  viewport_bounds_range_[0] = device_properties.limits.viewportBoundsRange[0];
  viewport_bounds_range_[1] = device_properties.limits.viewportBoundsRange[1];
  // Use a per-vertex-format lookup table to retrieve the descriptor set
  // (at the moment, sampler/no sampler)
  for (size_t i = 0; i < VertexBuffer::kVertexFormat_Count; ++i) {
    descriptor_set_vertex_table_[i] = VK_NULL_HANDLE;
  }

  // Creating a pool per frame, currently our renderer only uses
  // a single sampler descriptor for draws that use a texture
  in_flight_frame_count_ = GetInFlightFrameCount();

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
  // The GPU may still be using the resources released below
  vkDeviceWaitIdle(vk_.device);
  render_pass_ = nullptr;
  render_state_ = nullptr;
  frame_active_ = false;
  frame_acquired_ = false;
  resources_.Shutdown("Renderer shutdown");

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
    case Renderer::kFeature_ASTC: {
      // Required by the Android Baseline Profile, but that isn't enforced at
      // initialization, so ask the device
      VkPhysicalDeviceFeatures device_features{};
      vkGetPhysicalDeviceFeatures(vk_.physical_device, &device_features);
      supported = (device_features.textureCompressionASTC_LDR == VK_TRUE);
      break;
    }
    default:
      break;
  }
  return supported;
}

void RendererVk::BeginFrame(
    const base_game_framework::DisplayManager::SwapchainHandle swapchain_handle) {
  frame_active_ = false;
  frame_acquired_ = false;
  // At the moment, we don't support render targets, so grab a swapchain image
  // as soon as we start a frame
  DisplayManager& display_manager = DisplayManager::GetInstance();
  DisplayManager::SwapchainFrameHandle frame_handle =
      display_manager.GetCurrentSwapchainFrame(swapchain_handle);
  bool acquired = (frame_handle != DisplayManager::kInvalid_swapchain_handle) &&
      display_manager.GetSwapchainFrameResourcesVk(frame_handle, swap_, true);
  if (!acquired) {
    // An out of date swapchain is recreated when acquiring fails, so try again with the new one
    frame_handle = display_manager.GetCurrentSwapchainFrame(swapchain_handle);
    acquired = (frame_handle != DisplayManager::kInvalid_swapchain_handle) &&
        display_manager.GetSwapchainFrameResourcesVk(frame_handle, swap_, true);
  }
  if (!acquired) {
    // No image to render to: skip the frame. The frame's resources (swap_) weren't updated
    // and may refer to a destroyed swapchain, and nothing signals the image available
    // semaphore, so nothing may be recorded or submitted.
    RENDERER_LOG("BeginFrame: no swapchain image acquired, skipping the frame");
    return;
  }
  // The image is acquired: the frame must be submitted (EndFrame), even if nothing can be
  // recorded, to signal its semaphores and fence for the present and the next frame
  frame_acquired_ = true;
  active_extent_ = swap_.swapchain_extent;
  active_rotation_ = display_manager.GetSwapchainRotationMode(swapchain_handle);

  // Per in-flight frame resources (sized from the swapchain's frame count)
  const uint32_t frame_index = swap_.swapchain_frame_index;
  // Acquiring the image waited for this frame slot's previous frame to finish: delete the
  // released resources that no frame in flight can still use. A resource waits for every
  // frame slot of the swapchain.
  const uint32_t frame_count = (swap_.swapchain_frame_count != 0) ?
      swap_.swapchain_frame_count : in_flight_frame_count_;
  resources_.FrameSlotWaited(frame_index, frame_count);
  resources_.ProcessDeleteQueue();

  RENDERER_ASSERT(frame_index < descriptor_pools_.size() && frame_index < command_buffers_.size());
  if (frame_index >= descriptor_pools_.size() || frame_index >= command_buffers_.size()) {
    return;
  }

  active_frame_pool_ = descriptor_pools_[frame_index];
  VkResult reset_result = vkResetDescriptorPool(vk_.device, active_frame_pool_, 0);
  RENDERER_CHECK_VK(reset_result, "vkResetDescriptorPool");

  render_command_buffer_ = command_buffers_[frame_index];

  const VkResult reset_command_result = vkResetCommandBuffer(render_command_buffer_, 0);
  RENDERER_CHECK_VK(reset_command_result, "vkResetCommandBuffer");

  VkCommandBufferBeginInfo command_buffer_begin_info = {};
  command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  const VkResult begin_command_result = vkBeginCommandBuffer(render_command_buffer_,
                                                             &command_buffer_begin_info);
  if (!RENDERER_CHECK_VK(begin_command_result, "vkBeginCommandBuffer")) {
    // Not recording: nothing is recorded, EndFrame submits without the command buffer
    return;
  }
  frame_active_ = true;

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
  const bool recorded = frame_active_;
  const bool acquired = frame_acquired_;
  frame_active_ = false;
  frame_acquired_ = false;
  if (recorded) {
    if (render_pass_.get() != nullptr) {
      render_pass_.get()->EndRenderPass();
    }
    vkEndCommandBuffer(render_command_buffer_);
  }
  render_pass_ = nullptr;
  render_state_ = nullptr;
  active_frame_pool_ = VK_NULL_HANDLE;
  texture_descriptor_frame_cache_.clear();
  ResetBoundDescriptorSet();
  if (!acquired) {
    // Skipped frame (see BeginFrame): no image, nothing to submit
    return;
  }

  // Submit the frame; without a recorded command buffer (recording couldn't start), still
  // wait on the image available semaphore and signal the render complete semaphore and frame
  // fence, which the present and the next use of this frame's resources wait on
  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore signal_semaphores[] = {swap_.render_complete};
  VkSemaphore wait_semaphores[] = {swap_.image_available};
  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = recorded ? 1 : 0;
  submit_info.pCommandBuffers = recorded ? &render_command_buffer_ : nullptr;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signal_semaphores;

  const VkResult queue_result = vkQueueSubmit(vk_.render_queue, 1, &submit_info, swap_.frame_fence);
  RENDERER_CHECK_VK(queue_result, "vkQueueSubmit");
}

void RendererVk::SwapchainRecreated() {
  // The swapchain images may still be in use, e.g. if a frame was in flight
  vkDeviceWaitIdle(vk_.device);
  // If this happens during a frame, the frame's swapchain resources (fence, semaphores, image
  // views) were destroyed with the old swapchain: abandon the frame (the command buffer is
  // reset by the next BeginFrame), and don't submit or record anything else in it
  frame_active_ = false;
  frame_acquired_ = false;
  // The GPU is idle, so released resources can all be deleted now
  resources_.DeleteAllQueued();
  // Our cached framebuffers were associated with image view from the old
  // swapchain, purge the cache to rebuild them using the new swapchain
  resources_.ForEachLiveRenderPass([](RenderPass* render_pass) {
    static_cast<RenderPassVk*>(render_pass)->PurgeFramebufferCache();
  });
}

void RendererVk::ResetBoundDescriptorSet() {
  // Nothing pending either: a bind of a null set is invalid
  bound_descriptor_set_ = VK_NULL_HANDLE;
  bound_image_view_ = VK_NULL_HANDLE;
  dirty_descriptor_set_ = false;
}

void RendererVk::BindDirtyDescriptorSet(const RenderStateVk& state) {
  if (dirty_descriptor_set_ && bound_descriptor_set_ != VK_NULL_HANDLE) {
    vkCmdBindDescriptorSets(render_command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            state.GetPipelineLayout(), 0, 1, &bound_descriptor_set_,
                            0, nullptr);
  }
  dirty_descriptor_set_ = false;
}

void RendererVk::Draw(const uint32_t vertex_count, const uint32_t first_vertex) {
  if (!frame_active_) {
    return;
  }
  RenderStateVk& state = *(static_cast<RenderStateVk*>(render_state_.get()));
  BindDirtyDescriptorSet(state);
  // Viewport or scissor changed while the state was bound
  if (state.GetViewportScissorDirty()) {
    ApplyViewportAndScissor(state);
  }

  // Update any uniform data that might have changed between draw calls
  state.UpdateUniformData(render_command_buffer_, true);

  vkCmdDraw(render_command_buffer_, vertex_count, 1, first_vertex, 0);
}

void RendererVk::DrawIndexed(const uint32_t index_count, const uint32_t first_index) {
  if (!frame_active_) {
    return;
  }
  RenderStateVk& state = *(static_cast<RenderStateVk*>(render_state_.get()));
  BindDirtyDescriptorSet(state);
  // Viewport or scissor changed while the state was bound
  if (state.GetViewportScissorDirty()) {
    ApplyViewportAndScissor(state);
  }

  // Update any uniform data that might have changed between draw calls
  state.UpdateUniformData(render_command_buffer_, true);

  vkCmdDrawIndexed(render_command_buffer_, index_count, 1, first_index, 0, 0);
}

void RendererVk::SetRenderPass(std::shared_ptr<RenderPass> render_pass) {
  if (!frame_active_) {
    // Skipped frame: no command buffer to record the pass into
    return;
  }
  // Setting a render pass always ends the current one and begins the new one, even if it is
  // the same pass (which loads/clears its attachments again), as RendererGLES does
  if (render_pass_ != nullptr) {
    render_pass_->EndRenderPass();
  }
  render_pass_ = render_pass;
  render_state_ = nullptr;
  // The next render state binds its own descriptor set
  ResetBoundDescriptorSet();
  if (render_pass_ != nullptr) {
    render_pass_->BeginRenderPass();
  }
}

void RendererVk::SetRenderState(std::shared_ptr<RenderState> render_state) {
  if (!frame_active_) {
    return;
  }
  if (render_state.get() != render_state_.get()) {
    render_state_ = render_state;
    // Textures are bound again for the new state (its descriptor set layout may differ)
    ResetBoundDescriptorSet();
    if (render_state_ == nullptr) {
      return;
    }
    RenderStateVk& state = *(static_cast<RenderStateVk*>(render_state_.get()));
    vkCmdBindPipeline(render_command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, state.GetPipeline());
    ApplyViewportAndScissor(state);
  }
}

RendererVk::Rect64 RendererVk::DisplayRectToFramebuffer(const int64_t x, const int64_t y,
                                                        const int64_t width,
                                                        const int64_t height) const {
  // The framebuffer has the swapchain's extent, which is the display size with width and
  // height swapped for 90/270 degree pre-rotation
  const bool swapped = (active_rotation_ == DisplayManager::kSwapchain_Rotation_90 ||
                        active_rotation_ == DisplayManager::kSwapchain_Rotation_270);
  const int64_t display_width = swapped ? active_extent_.height : active_extent_.width;
  const int64_t display_height = swapped ? active_extent_.width : active_extent_.height;
  // Where the content of display rect lands in the framebuffer. This follows from the Y
  // flipped viewport and the pre-rotation matrix the application applies for each rotation
  // (see agdktunnel's NativeEngine), which must match the presentation engine's rotation.
  // Computed in 64 bits so that extreme int32_t rects can't wrap.
  Rect64 rect;
  switch (active_rotation_) {
    case DisplayManager::kSwapchain_Rotation_90:
      rect = {y, x, height, width};
      break;
    case DisplayManager::kSwapchain_Rotation_180:
      rect = {display_width - x - width, y, width, height};
      break;
    case DisplayManager::kSwapchain_Rotation_270:
      rect = {display_height - y - height, display_width - x - width, height, width};
      break;
    default:
      rect = {x, display_height - y - height, width, height};
      break;
  }
  rect.width = std::max<int64_t>(rect.width, 0);
  rect.height = std::max<int64_t>(rect.height, 0);
  return rect;
}

void RendererVk::ApplyViewportAndScissor(RenderStateVk& state) {
  state.ClearViewportScissorDirty();
  // Render state viewports and scissor rects follow GLES: bottom left origin, display
  // orientation
  const RenderState::Viewport& state_viewport = state.GetViewport();
  const Rect64 viewport_rect = DisplayRectToFramebuffer(state_viewport.x, state_viewport.y,
                                                        state_viewport.width,
                                                        state_viewport.height);
  // Like glViewport, clamp the size to the maximum viewport size (it must also be at least
  // 1), and keep the viewport within the device's viewport bounds
  const double max_width = std::max<uint32_t>(max_viewport_dimensions_[0], 1);
  const double max_height = std::max<uint32_t>(max_viewport_dimensions_[1], 1);
  const double width = std::min(std::max<double>(viewport_rect.width, 1.0), max_width);
  const double height = std::min(std::max<double>(viewport_rect.height, 1.0), max_height);
  const double x = std::min(std::max<double>(viewport_rect.x, viewport_bounds_range_[0]),
                            viewport_bounds_range_[1] - width);
  const double top = std::min(std::max<double>(viewport_rect.y, viewport_bounds_range_[0]),
                              viewport_bounds_range_[1] - height);
  // Flip Y with a negative height (VK_KHR_maintenance1), as in BeginFrame
  VkViewport viewport{};
  viewport.x = static_cast<float>(x);
  viewport.width = static_cast<float>(width);
  viewport.y = static_cast<float>(top + height);
  viewport.height = -static_cast<float>(height);
  // GLES clamps the depth range to [0, 1]; Vulkan requires it without
  // VK_EXT_depth_range_unrestricted (a NaN becomes 0)
  const auto clamp_depth = [](const float depth) {
    return (depth >= 0.0f) ? std::min(depth, 1.0f) : 0.0f;
  };
  viewport.minDepth = clamp_depth(state_viewport.min_depth);
  viewport.maxDepth = clamp_depth(state_viewport.max_depth);
  vkCmdSetViewport(render_command_buffer_, 0, 1, &viewport);

  // GLES clips primitives at the viewport, but Vulkan implementations may rasterize
  // outside it (guard band clipping; the Pixel 10 Pro's PowerVR does), so the scissor is
  // always limited to the viewport rect, intersected with the scissor rect when the
  // scissor test is enabled, and clipped to the framebuffer (Vulkan scissor offsets can't
  // be negative).
  int64_t left = static_cast<int64_t>(x);
  int64_t top_edge = static_cast<int64_t>(top);
  int64_t right = left + static_cast<int64_t>(width);
  int64_t bottom = top_edge + static_cast<int64_t>(height);
  if (state.GetScissorTest()) {
    const RenderState::ScissorRect& state_scissor = state.GetScissorRect();
    const Rect64 rect = DisplayRectToFramebuffer(state_scissor.x, state_scissor.y,
                                                 state_scissor.width, state_scissor.height);
    left = std::max(left, rect.x);
    top_edge = std::max(top_edge, rect.y);
    right = std::min(right, rect.x + rect.width);
    bottom = std::min(bottom, rect.y + rect.height);
  }
  const int64_t fb_width = active_extent_.width;
  const int64_t fb_height = active_extent_.height;
  left = std::min(std::max<int64_t>(left, 0), fb_width);
  top_edge = std::min(std::max<int64_t>(top_edge, 0), fb_height);
  right = std::min(right, fb_width);
  bottom = std::min(bottom, fb_height);
  VkRect2D scissor{};
  scissor.offset.x = static_cast<int32_t>(left);
  scissor.offset.y = static_cast<int32_t>(top_edge);
  scissor.extent.width = static_cast<uint32_t>(right > left ? right - left : 0);
  scissor.extent.height = static_cast<uint32_t>(bottom > top_edge ? bottom - top_edge : 0);
  vkCmdSetScissor(render_command_buffer_, 0, 1, &scissor);
}

void RendererVk::BindIndexBuffer(std::shared_ptr<IndexBuffer> index_buffer) {
  if (!frame_active_) {
    return;
  }
  IndexBufferVk& index_buffer_vk = *(static_cast<IndexBufferVk*>(index_buffer.get()));
  vkCmdBindIndexBuffer(render_command_buffer_, index_buffer_vk.GetIndexBuffer(),
                       0, VK_INDEX_TYPE_UINT16);
}

void RendererVk::BindVertexBuffer(std::shared_ptr<VertexBuffer> vertex_buffer) {
  if (!frame_active_) {
    return;
  }
  VertexBufferVk& vertex_buffer_vk = *(static_cast<VertexBufferVk*>(vertex_buffer.get()));
  VkBuffer vertex_buffers[] = {vertex_buffer_vk.GetVertexBuffer()};
  VkDeviceSize vertex_offsets[] = {0};
  vkCmdBindVertexBuffers(render_command_buffer_, 0, 1, vertex_buffers, vertex_offsets);
}

void RendererVk::BindTexture(std::shared_ptr<Texture> texture) {
  if (!frame_active_) {
    // Skipped frame: no frame descriptor pool to allocate from
    return;
  }
  const TextureVk* texture_vk = static_cast<const TextureVk*>(texture.get());
  if (texture_vk == nullptr || texture_vk->GetImageView() == VK_NULL_HANDLE ||
      texture_vk->GetSampler() == VK_NULL_HANDLE) {
    // Nothing to bind (no texture, or one that failed to create). A null descriptor set
    // can't be bound, so drop any pending bind; the last bound set stays bound.
    ResetBoundDescriptorSet();
    return;
  }

  const VkImageView texture_image_view = texture_vk->GetImageView();
  if (texture_image_view == bound_image_view_) {
    return;
  }
  RENDERER_ASSERT(render_state_ != nullptr);
  if (render_state_ == nullptr) {
    // The descriptor set layout comes from the render state
    return;
  }

  // Only textured vertex formats (P3T2, P3T2C4) have a sampler binding in their descriptor
  // set layout; for the others there is nothing to bind the texture to
  RenderStateVk& state = *(static_cast<RenderStateVk*>(render_state_.get()));
  const VkDescriptorSetLayout layout = state.GetDescriptorSetLayout();
  const VkDescriptorSetLayout textured_layout =
      descriptor_set_vertex_table_[VertexBuffer::kVertexFormat_P3T2];
  if (layout == VK_NULL_HANDLE || layout != textured_layout) {
    return;
  }

  // Descriptor sets are cached per frame by texture and layout
  for (const TextureDescriptorFrameCache& cache : texture_descriptor_frame_cache_) {
    if (cache.texture_image_view == texture_image_view && cache.layout == layout) {
      bound_image_view_ = texture_image_view;
      bound_descriptor_set_ = cache.descriptor_set;
      dirty_descriptor_set_ = true;
      return;
    }
  }

  // Create a new descriptor/descriptor set for this texture for this frame and
  // write the descriptor data
  VkDescriptorSetLayout descriptor_set_layouts[] = {layout};
  VkDescriptorSetAllocateInfo descriptor_set_info = {};
  descriptor_set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptor_set_info.descriptorPool = active_frame_pool_;
  descriptor_set_info.descriptorSetCount = 1;
  descriptor_set_info.pSetLayouts = descriptor_set_layouts;
  VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
  const VkResult allocate_result = vkAllocateDescriptorSets(vk_.device, &descriptor_set_info,
                                                            &descriptor_set);
  if (!RENDERER_CHECK_VK(allocate_result, "vkAllocateDescriptorSets")) {
    return;
  }

  VkDescriptorImageInfo descriptor_image_info = {};
  descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  descriptor_image_info.imageView = texture_image_view;
  descriptor_image_info.sampler = texture_vk->GetSampler();

  VkWriteDescriptorSet write_descriptor_set = {};
  write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_set.dstSet = descriptor_set;
  write_descriptor_set.dstBinding = 1;
  write_descriptor_set.dstArrayElement = 0;
  write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write_descriptor_set.descriptorCount = 1;
  write_descriptor_set.pImageInfo = &descriptor_image_info;

  vkUpdateDescriptorSets(vk_.device, 1, &write_descriptor_set, 0, nullptr);
  bound_image_view_ = texture_image_view;
  bound_descriptor_set_ = descriptor_set;
  dirty_descriptor_set_ = true;

  texture_descriptor_frame_cache_.push_back({texture_image_view, layout, descriptor_set});
}

std::shared_ptr<IndexBuffer> RendererVk::CreateIndexBuffer(
    const IndexBuffer::IndexBufferCreationParams& params) {
  return resources_.AddIndexBuffer(new IndexBufferVk(params));
}

std::shared_ptr<RenderPass> RendererVk::CreateRenderPass(
    const RenderPass::RenderPassCreationParams& params) {
  return resources_.AddRenderPass(new RenderPassVk(params));
}

std::shared_ptr<RenderState> RendererVk::CreateRenderState(
    const RenderState::RenderStateCreationParams& params) {
  return resources_.AddRenderState(new RenderStateVk(params));
}

std::shared_ptr<ShaderProgram> RendererVk::CreateShaderProgram(
    const ShaderProgram::ShaderProgramCreationParams& params) {
  return resources_.AddShaderProgram(new ShaderProgramVk(params));
}

std::shared_ptr<Texture> RendererVk::CreateTexture(const Texture::TextureCreationParams& params) {
  return resources_.AddTexture(new TextureVk(params));
}

std::shared_ptr<UniformBuffer> RendererVk::CreateUniformBuffer(
    const UniformBuffer::UniformBufferCreationParams& params) {
  return resources_.AddUniformBuffer(new UniformBufferVk(params));
}

std::shared_ptr<VertexBuffer> RendererVk::CreateVertexBuffer(
    const VertexBuffer::VertexBufferCreationParams& params) {
  return resources_.AddVertexBuffer(new VertexBufferVk(params));
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
  RENDERER_ASSERT(staging_command_buffer_ != nullptr);

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
  RENDERER_ASSERT(staging_command_buffer_ != nullptr);
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
