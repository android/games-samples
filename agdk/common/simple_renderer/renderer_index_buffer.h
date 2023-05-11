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

#include <cstdint>
#include <string>

namespace simple_renderer
{
/**
 * @brief The base class definition for the `IndexBuffer` class of SimpleRenderer.
 * Use the `Renderer` class interface to create and destroy `IndexBuffer` objects.
 * Currently all index elements are fixed as 16-bit values.
 * `IndexBuffer` does not currently support dynamically updating index buffer data after
 * initial creation.
 */
class IndexBuffer {
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

  /**
   * @brief Get the size of the `IndexBuffer` data.
   * @return The size of the `IndexBuffer` data in bytes
   */
  size_t GetBufferSize() const { return buffer_size_bytes_; }
  /**
   * @brief Get the size of an `IndexBuffer` index data element.
   * @return The size of an `IndexBuffer` data element in bytes (currently will always be 2).
   */
  size_t GetIndexElementSize() const { return sizeof(uint16_t); }
  /**
   * @brief Get the number of indices in the `IndexBuffer`.
   * @return The number of indices in the `IndexBuffer`
   */
  size_t GetIndexCount() const { return GetBufferSize() / GetIndexElementSize(); }

  /**
   * @brief Retrieve the debug name string associated with the `IndexBuffer`
   * @result A string containing the debug name.
   */
  const std::string& GetIndexBufferDebugName() const { return index_buffer_debug_name_; }
  /**
   * @brief Set a debug name string to associate with the `IndexBuffer`
   * @param name A string containing the debug name.
   */
  void SetIndexBufferDebugName(const std::string& name) { index_buffer_debug_name_ = name; }

 protected:
  IndexBuffer(const IndexBufferCreationParams& params) :
      buffer_size_bytes_(params.data_byte_size),
      index_buffer_debug_name_("noname") {
  }

 private:
  IndexBuffer() : buffer_size_bytes_(0) {}
  size_t buffer_size_bytes_;
  std::string index_buffer_debug_name_;
};
} // namespace simple_renderer

#endif // SIMPLERENDERER_INDEX_BUFFER_H_
