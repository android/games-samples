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

#ifndef SIMPLERENDERER_BUFFER_H_
#define SIMPLERENDERER_BUFFER_H_

#include <cstdint>
#include <string>

namespace simple_renderer
{
/**
 * @brief The base class definition for the `RendererBuffer` class of SimpleRenderer.
 * RenderBuffer is used as a base class for `IndexBuffer`, `VertexBuffer` and
 * `UniformBuffer` objects. This class is not instantiated directly.
 * Use the `Renderer` class interface to create and destroy the different buffer objects.
 */
 class RendererBuffer {
 public:
   /**
    * @brief Class destructor, do not call directly, use appropriate Destroy
    * call via Renderer interface.
    */
  virtual ~RendererBuffer() {}

   /**
    * @brief Get the size of the data stored in the `RendererBuffer` derived object.
    * @return The size of the buffer data in bytes.
    */
  size_t GetBufferSizeInBytes() const { return buffer_size_bytes_; }

   /**
    * @brief Get the stride in bytes of an element stored in the `RendererBuffer`
    * derived object. This is the vertex stride for a vertex buffer, the index
    * size for an index buffer, and the uniform buffer size for a uniform buffer.
    * @return The stride in bytes of this buffer's element.
    */
  size_t GetBufferStride() const { return buffer_stride_; }

   /**
    * @brief Get the number of elements stored in the `RendererBuffer` derived object.
    * This is the vertex count for a vertex buffer, the indices count for an index buffer,
    * and the number of uniform elements in a uniform buffer.
    * @return The number of elements in the buffer.
    */
  size_t GetBufferElementCount() const { return buffer_element_count_; }

   /**
    * @brief Retrieve the debug name string associated with the `IndexBuffer`
    * @result A string containing the debug name.
    */
  const std::string& GetBufferDebugName() const { return buffer_debug_name_; }

   /**
    * @brief Set a debug name string to associate with the `RendererBuffer` derived object.
    * @param name A string containing the debug name.
    */
  void SetBufferDebugName(const std::string& name) { buffer_debug_name_ = name; }

 protected:
  RendererBuffer(size_t buffer_element_count, size_t buffer_size_bytes, size_t buffer_stride) :
       buffer_element_count_(buffer_element_count),
       buffer_size_bytes_(buffer_size_bytes),
       buffer_stride_(buffer_stride),
       buffer_debug_name_("noname") {
  }

 private:
  size_t buffer_element_count_;
  size_t buffer_size_bytes_;
  size_t buffer_stride_;

  std::string buffer_debug_name_;
};
}

#endif // SIMPLERENDERER_BUFFER_H_