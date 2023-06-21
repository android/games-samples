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
#include "simple_renderer/renderer_interface.h"
#include "simple_renderer/renderer_index_buffer.h"
#include "simple_renderer/renderer_vertex_buffer.h"

// Convenience class that represents a geometry in terms of a
// vertex buffer + index buffer.
class SimpleGeom {
 public:
  std::shared_ptr<simple_renderer::IndexBuffer> index_buffer_;
  std::shared_ptr<simple_renderer::VertexBuffer> vertex_buffer_;

  SimpleGeom() {
    index_buffer_ = nullptr;
    vertex_buffer_ = nullptr;
  }

  SimpleGeom(std::shared_ptr<simple_renderer::IndexBuffer> index_buffer,
              std::shared_ptr<simple_renderer::VertexBuffer> vertex_buffer) {
    index_buffer_ = index_buffer;
    vertex_buffer_ = vertex_buffer;
  }

  SimpleGeom(std::shared_ptr<simple_renderer::VertexBuffer> vertex_buffer) {
    index_buffer_ = nullptr;
    vertex_buffer_ = vertex_buffer;
  }

  ~SimpleGeom() {
    simple_renderer::Renderer& renderer = simple_renderer::Renderer::GetInstance();
    if (index_buffer_.get() != nullptr) {
      renderer.DestroyIndexBuffer(index_buffer_);
      index_buffer_ = nullptr;
    }
    if (vertex_buffer_.get() != nullptr) {
      renderer.DestroyVertexBuffer(vertex_buffer_);
      vertex_buffer_ = nullptr;
    }
  }
};

#endif
