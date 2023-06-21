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

#ifndef SIMPLERENDERER_INDEX_BUFFER_H_
#define SIMPLERENDERER_INDEX_BUFFER_H_

#include "renderer_buffer.h"

namespace simple_renderer
{
/**
 * @brief The base class definition for the `IndexBuffer` class of SimpleRenderer.
 * Use the `Renderer` class interface to create and destroy `IndexBuffer` objects.
 * Currently all index elements are fixed as 16-bit values.
 * `IndexBuffer` does not currently support dynamically updating index buffer data after
 * initial creation.
 */
class IndexBuffer : public RendererBuffer {
 public:
  /**
   * @brief A structure holding required parameters to create a new `IndexBuffer`.
   * Passed to the Renderer::CreateIndexBuffer function.
   */
  struct IndexBufferCreationParams {
    /** @brief A pointer to an array of 16-bit index values. This pointer does not
     * need to persist after the call to ::CreateIndexBuffer completes.
     */
    void* index_data;
    /** @brief The size of the index buffer array in bytes */
    size_t data_byte_size;
  };

  virtual ~IndexBuffer() {}

 protected:
  IndexBuffer(const IndexBufferCreationParams& params) :
      RendererBuffer(params.data_byte_size / kIndexElementSize,
                     params.data_byte_size, kIndexElementSize) {
  }

 private:
  // At the moment we only support 16-bit index values
  static constexpr size_t kIndexElementSize = sizeof(uint16_t);

  IndexBuffer() : RendererBuffer(0, 0, 0) {}
};
} // namespace simple_renderer

#endif // SIMPLERENDERER_INDEX_BUFFER_H_
