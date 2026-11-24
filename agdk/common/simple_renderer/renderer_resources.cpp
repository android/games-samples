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
#include <map>
#include <string>
#include <type_traits>

namespace simple_renderer {

static uint32_t AllFrameSlots(const uint32_t frame_slot_count) {
  return (frame_slot_count >= 32) ? 0xFFFFFFFFu : ((1u << frame_slot_count) - 1u);
}

RendererResources::RendererResources(const uint32_t frame_slot_count)
    : state_(std::make_shared<State>()) {
  // Frame slots are tracked in a 32 bit mask
  RENDERER_ASSERT(frame_slot_count <= 32);
  state_->frame_slot_count = (frame_slot_count <= 32) ? frame_slot_count : 32;
  state_->all_frame_slots = AllFrameSlots(state_->frame_slot_count);
}

RendererResources::~RendererResources() {
  // Queued resources can't be deleted here (the renderer is being destroyed; its shutdown
  // deletes them). Resources released from now on are leaked by their deleters (a deleter
  // that already holds the state takes the mutex after this, and sees shut_down).
  std::lock_guard<std::mutex> lock(state_->mutex);
  state_->shut_down = true;
  if (!state_->queue.empty()) {
    RENDERER_ERROR("RendererResources: %zu resources queued for deletion were leaked",
                   state_->queue.size());
  }
}

template <typename T>
std::shared_ptr<T> RendererResources::Add(T* resource, const char* type_name) {
  if (resource == nullptr) {
    return nullptr;
  }
  {
    std::lock_guard<std::mutex> lock(state_->mutex);
    state_->live.emplace(resource, type_name);
    if constexpr (std::is_same_v<T, RenderPass>) {
      state_->live_render_passes.insert(resource);
    }
  }
  // The deleter only queues the resource; it holds the state weakly, as the pointer may
  // outlive the renderer
  std::weak_ptr<State> weak_state = state_;
  return std::shared_ptr<T>(resource, [weak_state, type_name](T* released) {
    std::shared_ptr<State> state = weak_state.lock();
    bool leaked = (state == nullptr);
    if (!leaked) {
      std::lock_guard<std::mutex> lock(state->mutex);
      state->live.erase(released);
      if constexpr (std::is_same_v<T, RenderPass>) {
        state->live_render_passes.erase(released);
      }
      if (state->shut_down) {
        leaked = true;
      } else {
        state->queue.push_back({[released]() { delete released; }, 0});
      }
    }
    if (leaked) {
      // Its device or context is gone (or going): it can't be deleted
      RENDERER_ERROR("%s %p was released after its renderer was shut down, leaking it",
                     type_name, static_cast<const void*>(released));
    }
  });
}

std::shared_ptr<IndexBuffer> RendererResources::AddIndexBuffer(IndexBuffer* index_buffer) {
  return Add(index_buffer, "IndexBuffer");
}

std::shared_ptr<RenderPass> RendererResources::AddRenderPass(RenderPass* render_pass) {
  return Add(render_pass, "RenderPass");
}

std::shared_ptr<RenderState> RendererResources::AddRenderState(RenderState* render_state) {
  return Add(render_state, "RenderState");
}

std::shared_ptr<ShaderProgram> RendererResources::AddShaderProgram(
    ShaderProgram* shader_program) {
  return Add(shader_program, "ShaderProgram");
}

std::shared_ptr<Texture> RendererResources::AddTexture(Texture* texture) {
  return Add(texture, "Texture");
}

std::shared_ptr<UniformBuffer> RendererResources::AddUniformBuffer(
    UniformBuffer* uniform_buffer) {
  return Add(uniform_buffer, "UniformBuffer");
}

std::shared_ptr<VertexBuffer> RendererResources::AddVertexBuffer(VertexBuffer* vertex_buffer) {
  return Add(vertex_buffer, "VertexBuffer");
}

void RendererResources::FrameSlotWaited(const uint32_t frame_slot,
                                        const uint32_t frame_slot_count) {
  RENDERER_ASSERT(frame_slot < frame_slot_count && frame_slot_count <= 32);
  if (frame_slot >= frame_slot_count || frame_slot_count > 32) {
    return;
  }
  std::lock_guard<std::mutex> lock(state_->mutex);
  // The swapchain's frame count decides which slots a resource must wait for (e.g. a new
  // swapchain with another buffer mode)
  state_->frame_slot_count = frame_slot_count;
  state_->all_frame_slots = AllFrameSlots(frame_slot_count);
  for (QueuedDeletion& deletion : state_->queue) {
    deletion.frame_slots_waited |= (1u << frame_slot);
  }
}

void RendererResources::ProcessDeleteQueue() {
  std::vector<std::function<void()>> ready;
  {
    std::lock_guard<std::mutex> lock(state_->mutex);
    std::vector<QueuedDeletion> waiting;
    for (QueuedDeletion& deletion : state_->queue) {
      if ((deletion.frame_slots_waited & state_->all_frame_slots) == state_->all_frame_slots) {
        ready.push_back(std::move(deletion.destroy));
      } else {
        waiting.push_back(std::move(deletion));
      }
    }
    state_->queue.swap(waiting);
  }
  // Without the lock: deleting a resource can release others (e.g. a render state's shader
  // program), which queues them (for a later frame)
  for (std::function<void()>& destroy : ready) {
    destroy();
  }
}

void RendererResources::DeleteAllQueued() {
  while (true) {
    std::vector<QueuedDeletion> queue;
    {
      std::lock_guard<std::mutex> lock(state_->mutex);
      queue.swap(state_->queue);
    }
    if (queue.empty()) {
      break;
    }
    for (QueuedDeletion& deletion : queue) {
      deletion.destroy();
    }
  }
}

void RendererResources::Shutdown(const char* reason) {
  std::map<std::string, size_t> counts;
  while (true) {
    std::vector<QueuedDeletion> queue;
    {
      std::lock_guard<std::mutex> lock(state_->mutex);
      if (state_->queue.empty()) {
        // Nothing is left to delete: from now on (with no gap) a released resource is leaked
        state_->shut_down = true;
        for (const auto& live : state_->live) {
          ++counts[live.second];
        }
        break;
      }
      queue.swap(state_->queue);
    }
    // Without the lock (deleting can release, and so queue, other resources)
    for (QueuedDeletion& deletion : queue) {
      deletion.destroy();
    }
  }
  for (const auto& count : counts) {
    RENDERER_ERROR("%s: %zu %s still referenced", reason, count.second, count.first.c_str());
  }
}

void RendererResources::ForEachLiveRenderPass(
    const std::function<void(RenderPass*)>& function) {
  std::vector<RenderPass*> render_passes;
  {
    std::lock_guard<std::mutex> lock(state_->mutex);
    render_passes.assign(state_->live_render_passes.begin(), state_->live_render_passes.end());
  }
  for (RenderPass* render_pass : render_passes) {
    function(render_pass);
  }
}

}
