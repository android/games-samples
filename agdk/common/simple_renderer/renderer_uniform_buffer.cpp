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

#include "renderer_uniform_buffer.h"
#include <cstring>
#include "renderer_debug.h"

namespace simple_renderer {

uint32_t UniformBuffer::GetElementFloatCount(const UniformBufferElementType element_type) {
  switch (element_type) {
    case kBufferElement_Float4:
      return 4;
    case kBufferElement_Matrix44:
      return 16;
  }
  return 0;
}

UniformBuffer::ElementLayout UniformBuffer::GetUsableElementLayout(
    const UniformBufferCreationParams& params) {
  ElementLayout layout = {0, 0};
  if (params.element_array == nullptr) {
    RENDERER_ASSERT(params.element_count == 0);
  } else {
    while (layout.element_count < params.element_count) {
      const uint32_t element_floats =
          GetElementFloatCount(params.element_array[layout.element_count].element_type);
      if (layout.element_count >= kMaxUniforms || element_floats == 0 ||
          layout.float_count + element_floats > kMaxUniformBufferFloatSize) {
        // Element offsets and GLES uniform locations hold kMaxUniforms entries, and the data
        // must fit in the buffer
        RENDERER_ERROR("UniformBuffer: only the first %u of %u elements fit (max %u elements, "
                       "%zu bytes, known types)", layout.element_count, params.element_count,
                       kMaxUniforms, kMaxUniformBufferByteSize);
        RENDERER_ASSERT(false);
        break;
      }
      layout.float_count += element_floats;
      ++layout.element_count;
    }
  }
  // The buffer size is that of the (usable) elements
  const size_t byte_size = layout.float_count * sizeof(float);
  if (params.data_byte_count != byte_size) {
    RENDERER_ERROR("UniformBuffer: data_byte_count %zu doesn't match the %zu bytes of the "
                   "elements, using %zu", params.data_byte_count, byte_size, byte_size);
    RENDERER_ASSERT(false);
  }
  return layout;
}

UniformBuffer::UniformBuffer(const UniformBufferCreationParams& params) :
    UniformBuffer(params, GetUsableElementLayout(params)) {
}

UniformBuffer::UniformBuffer(const UniformBufferCreationParams& params,
                             const ElementLayout& layout) :
    RendererBuffer(layout.element_count, layout.float_count * sizeof(float),
                   layout.float_count * sizeof(float)),
    element_data_float_count_(layout.float_count) {
  // Copy the usable elements (the caller's array doesn't need to persist), with their
  // offsets (32-bit float indexed, not byte indexed) in the data buffer. Unused entries are
  // empty Float4 elements at offset 0.
  uint32_t offset = 0;
  for (uint32_t i = 0; i < kMaxUniforms; ++i) {
    if (i < layout.element_count) {
      elements_[i] = params.element_array[i];
      if (elements_[i].element_name != nullptr) {
        element_names_[i] = elements_[i].element_name;
      }
      element_offsets_[i] = offset;
      offset += GetElementFloatCount(elements_[i].element_type);
    } else {
      elements_[i] = {kBufferElement_Float4, 0, 0, 0, nullptr};
      element_offsets_[i] = 0;
    }
    elements_[i].element_name = element_names_[i].c_str();
  }
}

const UniformBuffer::UniformBufferElement& UniformBuffer::GetElement(uint32_t index) const {
  RENDERER_ASSERT(index < GetBufferElementCount());
  if (index >= GetBufferElementCount()) {
    index = 0;
  }
  return elements_[index];
}

uint32_t UniformBuffer::GetElementOffset(uint32_t index) const {
  RENDERER_ASSERT(index < GetBufferElementCount());
  if (index >= GetBufferElementCount()) {
    index = 0;
  }
  return element_offsets_[index];
}

bool UniformBuffer::CopyElementData(const uint32_t index, const float* data, const size_t size,
                                    float* buffer) const {
  RENDERER_ASSERT(index < GetBufferElementCount());
  if (index >= GetBufferElementCount()) {
    return false;
  }
  // The data must be whole floats, and fit in the element (a larger write would overwrite
  // the next element, or run past the end of the buffer). Usable elements end within the
  // buffer.
  const size_t element_size = GetElementFloatCount(elements_[index].element_type) *
                              sizeof(float);
  const bool valid = data != nullptr && size > 0 && (size % sizeof(float)) == 0 &&
                     size <= element_size;
  RENDERER_ASSERT(valid);
  if (!valid) {
    RENDERER_ERROR("UniformBuffer: invalid write of %zu bytes for element %u of %zu bytes",
                   size, index, element_size);
    return false;
  }
  memcpy(&buffer[element_offsets_[index]], data, size);
  return true;
}

}
