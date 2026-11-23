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
#include <algorithm>
#include <cstring>
#include "renderer_debug.h"

namespace simple_renderer {

// Returns true if a stage range is usable as a push constant range: 4 byte aligned, nonzero,
// and within the element data (which is within kMaxUniformBufferByteSize, the push
// constant size Vulkan guarantees)
static bool IsValidStageRange(const uint32_t offset, const uint32_t size,
                              const uint32_t data_size, const char* stage) {
  const bool valid = (offset % 4) == 0 && (size % 4) == 0 && size > 0 &&
                     offset < data_size && size <= data_size - offset;
  if (!valid) {
    RENDERER_ERROR("UniformBufferVk: invalid %s stage range (offset %u size %u, data %u "
                   "bytes), stage not used", stage, offset, size, data_size);
    RENDERER_ASSERT(false);
  }
  return valid;
}

UniformBufferVk::UniformBufferVk(const UniformBuffer::UniformBufferCreationParams& params) :
    UniformBuffer(params) {
  buffer_flags_ = params.buffer_flags;
  // The base class lays out the elements
  for (uint32_t i = 0; i < GetBufferElementCount(); ++i) {
    const UniformBufferElement& element = GetElement(i);
    RENDERER_LOG("Element %u : name: %s type: %u set: %u index: %u",
                 i, element.element_name, element.element_type,
                 element.element_bind_set, element.element_bind_index);
  }
  buffer_size_ = GetElementDataByteSize();
  memset(buffer_data_, 0, sizeof(buffer_data_));

  // A stage is unused when both its offset and size are kUnusedStageRange. Invalid stage
  // ranges (including only one of the two being kUnusedStageRange) are reported and marked
  // unused.
  const auto is_unused = [](const uint32_t offset, const uint32_t size) {
    return offset == kUnusedStageRange && size == kUnusedStageRange;
  };
  stage_ranges_ = params.stage_ranges;
  if (!is_unused(stage_ranges_.vertex_stage_offset, stage_ranges_.vertex_stage_size) &&
      !IsValidStageRange(stage_ranges_.vertex_stage_offset, stage_ranges_.vertex_stage_size,
                         buffer_size_, "vertex")) {
    stage_ranges_.vertex_stage_offset = kUnusedStageRange;
    stage_ranges_.vertex_stage_size = kUnusedStageRange;
  }
  if (!is_unused(stage_ranges_.fragment_stage_offset, stage_ranges_.fragment_stage_size) &&
      !IsValidStageRange(stage_ranges_.fragment_stage_offset, stage_ranges_.fragment_stage_size,
                         buffer_size_, "fragment")) {
    stage_ranges_.fragment_stage_offset = kUnusedStageRange;
    stage_ranges_.fragment_stage_size = kUnusedStageRange;
  }
  stage_flags_ = 0;
  if (stage_ranges_.vertex_stage_offset != kUnusedStageRange) {
    stage_flags_ |= VK_SHADER_STAGE_VERTEX_BIT;
  }
  if (stage_ranges_.fragment_stage_offset != kUnusedStageRange) {
    stage_flags_ |= VK_SHADER_STAGE_FRAGMENT_BIT;
  }
  SetBufferDirty(true);
}

UniformBufferVk::~UniformBufferVk() {
}

void UniformBufferVk::SetBufferElementData(const uint32_t index, const float* data,
                                           const size_t size) {
  if (CopyElementData(index, data, size, buffer_data_)) {
    SetBufferDirty(true);
  }
}

uint32_t UniformBufferVk::GetPushConstantRanges(VkPushConstantRange* ranges) const {
  uint32_t count = 0;
  if (stage_ranges_.vertex_stage_offset != kUnusedStageRange) {
    if (ranges != nullptr) {
      ranges[count].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
      ranges[count].offset = stage_ranges_.vertex_stage_offset;
      ranges[count].size = stage_ranges_.vertex_stage_size;
    }
    ++count;
  }
  if (stage_ranges_.fragment_stage_offset != kUnusedStageRange) {
    if (ranges != nullptr) {
      ranges[count].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
      ranges[count].offset = stage_ranges_.fragment_stage_offset;
      ranges[count].size = stage_ranges_.fragment_stage_size;
    }
    ++count;
  }
  return count;
}

void UniformBufferVk::PushConstants(VkCommandBuffer command_buffer,
                                    VkPipelineLayout pipeline_layout) const {
  RENDERER_ASSERT(command_buffer != VK_NULL_HANDLE && pipeline_layout != VK_NULL_HANDLE);
  if (command_buffer == VK_NULL_HANDLE || pipeline_layout == VK_NULL_HANDLE) {
    return;
  }
  VkPushConstantRange ranges[kMaxPushConstantRanges];
  const uint32_t range_count = GetPushConstantRanges(ranges);
  // Split the ranges at every range start and end, so each piece is covered by the same
  // set of ranges throughout, and push each piece with the stages of those ranges
  // (VUID-vkCmdPushConstants-offset-01795 and -01796)
  uint32_t edges[kMaxPushConstantRanges * 2];
  uint32_t edge_count = 0;
  for (uint32_t i = 0; i < range_count; ++i) {
    edges[edge_count++] = ranges[i].offset;
    edges[edge_count++] = ranges[i].offset + ranges[i].size;
  }
  std::sort(edges, edges + edge_count);
  const uint8_t* data = reinterpret_cast<const uint8_t*>(buffer_data_);
  for (uint32_t e = 0; e + 1 < edge_count; ++e) {
    const uint32_t start = edges[e];
    const uint32_t end = edges[e + 1];
    if (start == end) {
      continue;
    }
    VkShaderStageFlags stages = 0;
    for (uint32_t i = 0; i < range_count; ++i) {
      if (ranges[i].offset <= start && end <= ranges[i].offset + ranges[i].size) {
        stages |= ranges[i].stageFlags;
      }
    }
    if (stages != 0) {
      vkCmdPushConstants(command_buffer, pipeline_layout, stages, start, end - start,
                         data + start);
    }
  }
}
}
