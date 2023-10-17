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

#include "renderer_uniform_buffer_vk.h"
#include "renderer_debug.h"

namespace simple_renderer {

UniformBufferVk::UniformBufferVk(const UniformBuffer::UniformBufferCreationParams& params) :
    UniformBuffer(params) {
  buffer_flags_ = params.buffer_flags;
  stage_ranges_ = params.stage_ranges;
  const uint32_t count = GetBufferElementCount();
  uint32_t i = 0;
  uint32_t offset = 0;

  // Construct a list of offsets (32-bit float indexed, not byte indexed)
  // into our internal data buffer for each uniform buffer element
  while (i < count) {
    element_offsets_[i] = offset;
    const UniformBufferElement& element = GetElement(i);

    switch (element.element_type) {
      case UniformBuffer::kBufferElement_Float4:
        offset += 4;
        break;
      case UniformBuffer::kBufferElement_Matrix44:
        offset += 16;
        break;
    }
    RENDERER_LOG("Element %u : name: %s type: %u set: %u index: %u",
                 i, element.element_name, element.element_type,
                 element.element_bind_set, element.element_bind_index)
    ++i;
  }
  while (i < kMaxUniforms) {
    element_offsets_[i] = 0;
    ++i;
  }
  buffer_size_ = offset * sizeof(float);
  stage_flags_ = 0;
  if (params.stage_ranges.vertex_stage_offset != UniformBufferVk::kUnusedStageRange) {
    stage_flags_ |= VK_SHADER_STAGE_VERTEX_BIT;
  }
  if (params.stage_ranges.fragment_stage_offset != UniformBufferVk::kUnusedStageRange) {
    stage_flags_ |= VK_SHADER_STAGE_FRAGMENT_BIT;
  }
  SetBufferDirty(true);
}

UniformBufferVk::~UniformBufferVk() {

}

uint32_t UniformBufferVk::GetElementOffset(uint32_t index) const {
  RENDERER_ASSERT(index < GetBufferElementCount())

  if (index > GetBufferElementCount()) {
    index = 0;
  }
  return element_offsets_[index];
}

void UniformBufferVk::SetBufferElementData(const uint32_t index, const float* data,
                                           const size_t size) {
  RENDERER_ASSERT(index < GetBufferElementCount())

  if (index < GetBufferElementCount()) {
    const uint32_t offset = element_offsets_[index];
    const uint32_t end_offset = offset + (size / sizeof(float));
    RENDERER_ASSERT(offset < kMaxUniformBufferFloatSize)
    RENDERER_ASSERT(end_offset < kMaxUniformBufferFloatSize)
    if (offset < kMaxUniformBufferFloatSize && end_offset < kMaxUniformBufferFloatSize) {
      memcpy(&buffer_data_[offset], data, size);
    }
  }
  SetBufferDirty(true);
}

}