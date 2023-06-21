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

#ifndef SIMPLERENDERER_VERTEX_BUFFER_H_
#define SIMPLERENDERER_VERTEX_BUFFER_H_

#include "renderer_buffer.h"

namespace simple_renderer
{
/**
 * @brief The base class definition for the `VertexBuffer` class of SimpleRenderer.
 * Use the `Renderer` class interface to create and destroy `VertexBuffer` objects.
 * Currently all vertex formats use 32-bit floats for all element data.
 * `VertexBuffer` does not currently support dynamically updating vertex buffer data after
 * initial creation.
 */
class VertexBuffer : public RendererBuffer {
 public:
  /**
   * @brief The vertex formats supported by `VertexBuffer`
   */
  enum VertexFormat : uint32_t {
    /** @brief A vertex with three position (XYZ) elements */
    kVertexFormat_P3 = 0,
    /** @brief A vertex with three position (XYZ) elements
     * and two texture coordinate (UV) elements
     */
    kVertexFormat_P3T2,
    /** @brief A vertex with three position (XYZ) elements
     * and four color (RGBA) elements
     */
    kVertexFormat_P3C4,
    /** @brief A vertex with three position (XYZ) elements,
     * two texture coordinate (UV) elements,
     * and four color (RGBA) elements
     */
    kVertexFormat_P3T2C4,
    /** @brief Count of vertex formats */
    kVertexFormat_Count
  };

  /**
   * @brief A structure holding required parameters to create a new `VertexBuffer`.
   * Passed to the Renderer::CreateVertexBuffer function.
   */
  struct VertexBufferCreationParams {
    /** @brief A pointer to an array of vertex data */
    void* vertex_data;
    /** @brief The format of the vertex data */
    VertexBuffer::VertexFormat vertex_format;
    /** @brief The size of the index buffer array in bytes */
    size_t data_byte_size;
  };

  /**
   * @brief Get the vertex format of the `VertexBuffer` data.
   * @return A `VertexFormat` enum of the vertex format of the `VertexBuffer`.
   */
  VertexFormat GetVertexFormat() const { return vertex_format_; }

  /**
   * @brief Get the per-vertex stride, in bytes, of a specific `VertexFormat`
   * @param format A `VertexFormat` enum of the vertex format being queried for stride.
   * @return The stride of the `VertexFormat` in bytes
   */
  static size_t GetVertexFormatStride(const VertexFormat format) {
    if (format < kVertexFormat_Count) {
      return kVertexStrides[format];
    }
    return 0;
  }

  /**
   * @brief Get the per-vertex stride, in bytes, of the `VertexFormat` used by the 'VertexBuffer`
   * @return The stride of the `VertexBuffer`'s `VertexFormat` in bytes
   */
  size_t GetVertexStride() const {
    if (vertex_format_ < kVertexFormat_Count) {
      return kVertexStrides[vertex_format_];
    }
    return 0;
  }

  /**
   * @brief Base class destructor, do not call directly.
   */
  virtual ~VertexBuffer() {}

 protected:
  VertexBuffer(const VertexBufferCreationParams& params) :
      RendererBuffer(params.data_byte_size / kVertexStrides[params.vertex_format],
                     params.data_byte_size, kVertexStrides[params.vertex_format]),
      vertex_format_(params.vertex_format) {
  }

 private:
  VertexBuffer() :
      RendererBuffer(0, 0, 0),
      vertex_format_(kVertexFormat_Count) {

  }

  static constexpr size_t kVertexStrides[kVertexFormat_Count] = {
      12, 20, 28, 36 };
  VertexFormat vertex_format_;
};
} // namespace simple_renderer

#endif // SIMPLERENDERER_VERTEX_BUFFER_H_
