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

#ifndef SIMPLERENDERER_VK_H_
#define SIMPLERENDERER_VK_H_

#include "renderer_vk_includes.h"
#include "renderer_interface.h"
#include "renderer_resources.h"
#include "vulkan/graphics_api_vulkan_resources.h"
#include <unordered_map>
#include <utility>
#include <vector>

namespace simple_renderer {

class RenderStateVk;

/*
struct RendererVkResources {
  VkDevice device = VK_NULL_HANDLE;
  VkFence frame_fence = VK_NULL_HANDLE;
  VkInstance instance = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkQueue render_queue;
  VkSemaphore image_available = VK_NULL_HANDLE;
  VkSemaphore render_complete = VK_NULL_HANDLE;
  VkFormat swapchain_color = VK_FORMAT_UNDEFINED;
  VkFormat swapchain_depth = VK_FORMAT_UNDEFINED;
  VkFormat swapchain_stencil = VK_FORMAT_UNDEFINED;
  uint32_t swapchain_image_count = 0;
  uint32_t inflight_frame_count = 0;
};

struct RendererSwapchainResources {
  VkImageView swapchain_color_image_view = VK_NULL_HANDLE;
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  uint32_t swapchain_frame_index = 0;
  uint32_t swapchain_image_index = 0;
};
*/

/**
 * @brief A subclass implementation of the base Renderer class for the
 * Vulkan API. This class should not be used directly.
 */
class RendererVk : public Renderer {
 public:
  static RendererVk& GetInstanceVk();

  RendererVk();
  virtual ~RendererVk();

  virtual bool GetFeatureAvailable(const RendererFeature feature);

  virtual void BeginFrame(
      const base_game_framework::DisplayManager::SwapchainHandle swapchain_handle);
  virtual void EndFrame();

  virtual void SwapchainRecreated();

  virtual void Draw(const uint32_t vertex_count, const uint32_t first_vertex);
  virtual void DrawIndexed(const uint32_t index_count, const uint32_t first_index);

  virtual void SetRenderPass(std::shared_ptr<RenderPass> render_pass);
  virtual void SetRenderState(std::shared_ptr<RenderState> render_state);

  // Resource binds
  virtual void BindIndexBuffer(std::shared_ptr<IndexBuffer> index_buffer);
  virtual void BindVertexBuffer(std::shared_ptr<VertexBuffer> vertex_buffer);
  virtual void BindTexture(std::shared_ptr<Texture> texture);

  // Resource creation (resources are destroyed when released, see RendererResources)
  virtual std::shared_ptr<IndexBuffer> CreateIndexBuffer(
      const IndexBuffer::IndexBufferCreationParams& params);

  virtual std::shared_ptr<RenderPass> CreateRenderPass(
      const RenderPass::RenderPassCreationParams& params);

  virtual std::shared_ptr<RenderState> CreateRenderState(
      const RenderState::RenderStateCreationParams& params);

  virtual std::shared_ptr<ShaderProgram> CreateShaderProgram(
      const ShaderProgram::ShaderProgramCreationParams& params);

  virtual std::shared_ptr<Texture> CreateTexture(
      const Texture::TextureCreationParams& params);

  virtual std::shared_ptr<UniformBuffer> CreateUniformBuffer(
      const UniformBuffer::UniformBufferCreationParams& params);

  virtual std::shared_ptr<VertexBuffer> CreateVertexBuffer(
      const VertexBuffer::VertexBufferCreationParams& params);

  // Called by other vulkan renderer implementation classes, not exposed via the base interface
  VkDevice GetDevice() const { return RendererVk::vk_.device; }
  VkPhysicalDevice GetPhysicalDevice() const { return RendererVk::vk_.physical_device; }
  VkInstance GetInstance() const { return RendererVk::vk_.instance; }
  VmaAllocator GetAllocator() const { return RendererVk::vk_.allocator; }
  const base_game_framework::SwapchainFrameResourcesVk& GetSwapchainResources() const {
    return RendererVk::swap_;
  }

  VkDescriptorSetLayout GetDescriptorSetLayout(const VertexBuffer::VertexFormat vertex_format);

  // Uploads through staging buffers. Copies from all the uploads are recorded in one staging
  // command buffer, which is submitted ahead of the next frame's commands (in the same queue
  // submission), so creating many resources doesn't stall on each upload; a large batch is
  // submitted (and waited for) early. BeginStagingCommandBuffer returns the command buffer
  // to record an upload's copies (and barriers) into, or VK_NULL_HANDLE if it can't record.
  // EndStagingCommandBuffer takes ownership of the upload's staging buffer, destroyed once
  // the copies have executed. The resource being created is kept alive until then (see
  // KeepAliveForUpload).
  VkCommandBuffer BeginStagingCommandBuffer();
  void EndStagingCommandBuffer(VkBuffer staging_buffer, VmaAllocation staging_alloc,
                               VkDeviceSize staging_size);

  VkCommandBuffer GetRenderCommandBuffer() const { return render_command_buffer_; };
  VkExtent2D GetActiveExtent() const { return active_extent_; }

  VkFormat GetSwapchainColorFormat() const { return RendererVk::swap_.swapchain_color_format; }
  VkFormat GetSwapchainDepthStencilFormat() const {
    return RendererVk::swap_.swapchain_depth_stencil_format; }

 protected:
  virtual void PrepareShutdown();

 private:

  // Descriptor information for the texture is going to be the same each frame,
  // cache rather than creating a new descriptor per draw for the same texture
  // (samplers are baked into our texture class)
  struct TextureDescriptorFrameCache {
    VkImageView texture_image_view;
    VkDescriptorSetLayout layout;
    VkDescriptorSet descriptor_set;
  };

  static constexpr uint32_t kMaxSamplerDescriptors = 128;
  // Staging data recorded in one batch before it is submitted and waited for right away
  // (bounds the staging memory held, e.g. while loading many textures in one frame)
  static constexpr VkDeviceSize kMaxStagingBatchSize = 32 * 1024 * 1024;

  // Staging uploads recorded into one command buffer, see BeginStagingCommandBuffer
  struct StagingBatch {
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    // Staging buffers to destroy once the batch has executed
    std::vector<std::pair<VkBuffer, VmaAllocation>> buffers;
    // The resources the batch copies to, kept alive (not queued for deletion) until it has
    // executed or is discarded
    std::vector<std::shared_ptr<void>> destinations;
    VkDeviceSize size = 0;
    // Submitted with this frame slot's frame (done when its fence is next waited)
    uint32_t frame_slot = 0;
  };

  // A created resource whose upload was recorded in the open batch is kept alive by it, so
  // it can't be deleted (when released) before the copy executes. uploads_before: the upload
  // serial before the resource was constructed.
  template <typename T>
  std::shared_ptr<T> KeepAliveForUpload(std::shared_ptr<T> resource, uint64_t uploads_before);

  // End the recording batch's command buffer (with a barrier making its copies visible to
  // the vertex input and shaders of later commands). False if there is no batch or it failed.
  bool EndStagingBatch();
  // Submit the recording batch (if any) on its own and wait for it
  void FlushStagingBatchAndWait();
  // Drop the recording batch (if any) without executing it (at shutdown)
  void DiscardStagingBatch();
  // Destroy the staging buffers of a batch that has executed (or will never execute), release
  // its destinations, and keep it for reuse
  void RecycleStagingBatch(StagingBatch&& batch);
  // Recycle the submitted batches of a frame slot whose fence was just waited (and ones of
  // slots that no longer exist, frame_slot >= frame_count), or all of them (the GPU is idle)
  void RecycleSubmittedStagingBatches(uint32_t frame_slot, uint32_t frame_count, bool all);

  void CreateCommandBuffers();

  void DestroyCommandBuffers();

  void CreateDescriptorPools();

  // Forget the bound texture descriptor set, including a pending (not yet drawn) bind
  void ResetBoundDescriptorSet();

  // Bind the pending texture descriptor set, if there is one, before a draw
  void BindDirtyDescriptorSet(const RenderStateVk& state);

  // A rect with 64 bit coordinates (doesn't wrap for any int32_t input rect)
  struct Rect64 {
    int64_t x;
    int64_t y;
    int64_t width;
    int64_t height;
  };

  // Set the dynamic viewport and scissor from a render state
  void ApplyViewportAndScissor(RenderStateVk& state);

  // Map a GLES style rect (bottom left origin, in display orientation) to a framebuffer rect
  // (top left origin, in the swapchain's pre-rotated orientation)
  Rect64 DisplayRectToFramebuffer(int64_t x, int64_t y, int64_t width, int64_t height) const;

  RendererResources resources_;

  std::shared_ptr<RenderPass> render_pass_;
  std::shared_ptr<RenderState> render_state_;

  // The batch recording uploads (if staging_recording_), the batches submitted with frames
  // and not known to be done, and recycled batches (with their command buffers)
  StagingBatch staging_batch_;
  bool staging_recording_;
  // Counts the uploads recorded (see KeepAliveForUpload)
  uint64_t staging_upload_serial_;
  std::vector<StagingBatch> staging_submitted_;
  std::vector<StagingBatch> staging_free_;
  // For batches submitted on their own (FlushStagingBatchAndWait)
  VkFence staging_fence_;

  uint32_t in_flight_frame_count_;

  // Active frame resources
  VkCommandBuffer render_command_buffer_;
  // A swapchain image was acquired and the render command buffer is recording. If BeginFrame
  // couldn't acquire an image (e.g. the swapchain is out of date, or there is no window),
  // nothing is recorded or submitted for the frame.
  bool frame_active_;
  // A swapchain image was acquired this frame, so EndFrame must submit (to signal the frame's
  // semaphores and fence), even if recording couldn't start (frame_active_ false)
  bool frame_acquired_;
  VkExtent2D active_extent_;
  VkDescriptorPool active_frame_pool_;
  // Pre-rotation of the swapchain this frame (see DisplayManager::GetSwapchainRotationMode)
  base_game_framework::DisplayManager::SwapchainRotationMode active_rotation_;
  // Device viewport limits (VkPhysicalDeviceLimits)
  uint32_t max_viewport_dimensions_[2] = {0, 0};
  float viewport_bounds_range_[2] = {0.0f, 0.0f};

  // Active draw resources
  VkDescriptorSet bound_descriptor_set_;
  VkImageView bound_image_view_;
  bool dirty_descriptor_set_;

  VkCommandPool command_pool_;
  std::vector<VkCommandBuffer> command_buffers_;
  std::vector<VkDescriptorPool> descriptor_pools_;
  std::vector<VkDescriptorSetLayout> descriptor_set_layouts_;
  // Build a mapping table per-vertex format for easier lookup from render state
  std::vector<VkDescriptorSetLayout> descriptor_set_vertex_table_;
  // Plain old vector since we only have a handful of textures, this
  // would need a more efficient storage/lookup method otherwise
  std::vector<TextureDescriptorFrameCache> texture_descriptor_frame_cache_;

  base_game_framework::GraphicsAPIResourcesVk vk_;
  base_game_framework::SwapchainFrameResourcesVk swap_;
};

} // namespace simple_renderer

#endif // SIMPLERENDERER_VK_H_
