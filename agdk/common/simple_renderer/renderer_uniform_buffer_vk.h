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

#ifndef SIMPLERENDERER_UNIFORM_BUFFER_VK_H_
#define SIMPLERENDERER_UNIFORM_BUFFER_VK_H_

#include <cstdint>
#include "renderer_vk_includes.h"
#include "renderer_uniform_buffer.h"

namespace simple_renderer
{
class UniformBufferVk : public UniformBuffer {
 public:
  UniformBufferVk(const UniformBuffer::UniformBufferCreationParams& params);
  virtual ~UniformBufferVk();

  virtual uint32_t GetElementOffset(uint32_t index) const;

  virtual void SetBufferElementData(const uint32_t index, const float* data, const size_t size);

  bool GetBufferDirty() const { return buffer_dirty_; }
  void SetBufferDirty(bool dirty) { buffer_dirty_ = dirty; }

  const float* GetBufferData() const { return buffer_data_; }

  const uint32_t GetBufferSize() const { return buffer_size_; }

  const VkShaderStageFlags GetStageFlags() const { return stage_flags_; }

  const UniformBufferStageRanges& GetStageRanges() const { return stage_ranges_; }

 private:
  UniformBufferStageRanges stage_ranges_;
  float buffer_data_[kMaxUniformBufferFloatSize];
  uint32_t element_offsets_[kMaxUniforms];
  VkShaderStageFlags stage_flags_;
  uint32_t buffer_flags_;
  uint32_t buffer_size_;
  bool buffer_dirty_;
};
} // namespace simple_renderer

#endif // SIMPLERENDERER_UNIFORM_BUFFER_VK_H_
