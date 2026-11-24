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

#ifndef agdktunnel_simplegeom_hpp
#define agdktunnel_simplegeom_hpp

#include <memory>
#include <utility>
#include "simple_renderer/renderer_interface.h"
#include "simple_renderer/renderer_index_buffer.h"
#include "simple_renderer/renderer_vertex_buffer.h"

// Convenience class that represents a geometry in terms of a
// vertex buffer + index buffer. It owns the buffers: they are destroyed with it. It can be
// moved but not copied (a copy would destroy the buffers while the original still uses them).
class SimpleGeom {
 public:
  std::shared_ptr<simple_renderer::IndexBuffer> index_buffer_;
  std::shared_ptr<simple_renderer::VertexBuffer> vertex_buffer_;

  SimpleGeom() = default;

  SimpleGeom(std::shared_ptr<simple_renderer::IndexBuffer> index_buffer,
             std::shared_ptr<simple_renderer::VertexBuffer> vertex_buffer)
      : index_buffer_(std::move(index_buffer)), vertex_buffer_(std::move(vertex_buffer)) {}

  explicit SimpleGeom(std::shared_ptr<simple_renderer::VertexBuffer> vertex_buffer)
      : index_buffer_(nullptr), vertex_buffer_(std::move(vertex_buffer)) {}

  SimpleGeom(const SimpleGeom &) = delete;
  SimpleGeom &operator=(const SimpleGeom &) = delete;

  // The moved-from geometry is left empty
  SimpleGeom(SimpleGeom &&other) noexcept
      : index_buffer_(std::move(other.index_buffer_)),
        vertex_buffer_(std::move(other.vertex_buffer_)) {
    other.index_buffer_ = nullptr;
    other.vertex_buffer_ = nullptr;
  }

  SimpleGeom &operator=(SimpleGeom &&other) noexcept {
    if (this != &other) {
      DestroyBuffers();
      index_buffer_ = std::move(other.index_buffer_);
      vertex_buffer_ = std::move(other.vertex_buffer_);
      other.index_buffer_ = nullptr;
      other.vertex_buffer_ = nullptr;
    }
    return *this;
  }

  ~SimpleGeom() {
    DestroyBuffers();
  }

 private:
  void DestroyBuffers() {
    if (index_buffer_ == nullptr && vertex_buffer_ == nullptr) {
      return;
    }
    // After the renderer is shut down its buffers are gone; don't create a new renderer
    if (simple_renderer::Renderer::HasInstance()) {
      simple_renderer::Renderer& renderer = simple_renderer::Renderer::GetInstance();
      if (index_buffer_ != nullptr) {
        renderer.DestroyIndexBuffer(index_buffer_);
      }
      if (vertex_buffer_ != nullptr) {
        renderer.DestroyVertexBuffer(vertex_buffer_);
      }
    }
    index_buffer_ = nullptr;
    vertex_buffer_ = nullptr;
  }
};

#endif
