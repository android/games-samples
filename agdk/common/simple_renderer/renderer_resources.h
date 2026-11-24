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
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace simple_renderer {

/*
 * Owns the lifetime of a renderer's resources.
 *
 * Resources are handed out as shared pointers whose deleter doesn't delete the resource but
 * queues it for deletion: the GPU may still be using it (Vulkan frames in flight), and GLES
 * objects must be deleted with the context current. Queued resources are deleted by
 * ProcessDeleteQueue once that is safe:
 * - With frame slots (Vulkan): once every frame slot was waited (FrameSlotWaited) after the
 *   resource was queued, so no submitted frame that could use it is still executing.
 * - Without (GLES): at the next ProcessDeleteQueue.
 * DeleteAllQueued deletes everything queued (when the GPU is idle).
 *
 * A resource released after the renderer is shut down (Shutdown) or gone can't be deleted
 * (its device or context is gone too): it is leaked, with an error.
 */
class RendererResources {
 public:
  // frame_slot_count: frames that can be in flight, updated by FrameSlotWaited (0: no
  // tracking, see above)
  explicit RendererResources(uint32_t frame_slot_count);
  ~RendererResources();

  RendererResources(const RendererResources&) = delete;
  RendererResources& operator=(const RendererResources&) = delete;

  std::shared_ptr<IndexBuffer> AddIndexBuffer(IndexBuffer* index_buffer);
  std::shared_ptr<RenderPass> AddRenderPass(RenderPass* render_pass);
  std::shared_ptr<RenderState> AddRenderState(RenderState* render_state);
  std::shared_ptr<ShaderProgram> AddShaderProgram(ShaderProgram* shader_program);
  std::shared_ptr<Texture> AddTexture(Texture* texture);
  std::shared_ptr<UniformBuffer> AddUniformBuffer(UniformBuffer* uniform_buffer);
  std::shared_ptr<VertexBuffer> AddVertexBuffer(VertexBuffer* vertex_buffer);

  // The fence of a frame slot was waited (its last submitted frame finished). frame_slot_count
  // is the number of frame slots in use (the swapchain's frames in flight), which a queued
  // resource must all have waited before it is deleted.
  void FrameSlotWaited(uint32_t frame_slot, uint32_t frame_slot_count);

  // Delete the queued resources that are safe to delete
  void ProcessDeleteQueue();

  // Delete all queued resources (including ones queued while deleting). The GPU must be idle.
  void DeleteAllQueued();

  // At renderer shutdown (the GPU must be idle): delete all queued resources, then log the
  // ones still referenced. A resource released from then on is leaked, with an error.
  void Shutdown(const char* reason);

  // Calls function for each render pass that is still referenced (not queued for deletion)
  void ForEachLiveRenderPass(const std::function<void(RenderPass*)>& function);

 private:
  struct QueuedDeletion {
    std::function<void()> destroy;
    uint32_t frame_slots_waited;  // bit mask
  };

  // Shared with the deleters of the handed out pointers, which may outlive this object
  struct State {
    std::mutex mutex;
    // Set (with the mutex held) once the renderer is shut down: released resources are leaked
    bool shut_down = false;
    uint32_t frame_slot_count = 0;
    uint32_t all_frame_slots = 0;  // bit mask
    std::vector<QueuedDeletion> queue;
    // Referenced resources, with their type name
    std::unordered_map<const void*, const char*> live;
    std::unordered_set<RenderPass*> live_render_passes;
  };

  template <typename T>
  std::shared_ptr<T> Add(T* resource, const char* type_name);

  std::shared_ptr<State> state_;
};

}

#endif // SIMPLERENDERER_RESOURCES_H_
