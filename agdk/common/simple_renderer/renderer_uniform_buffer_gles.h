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

#ifndef SIMPLERENDERER_UNIFORM_BUFFER_GLES_H_
#define SIMPLERENDERER_UNIFORM_BUFFER_GLES_H_

#include <cstdint>
#include <GLES3/gl3.h>
#include "renderer_uniform_buffer.h"

namespace simple_renderer
{
// The GLES uniform buffer implementation currently keeps the original rendering code
// GLES2 element update style instead of actually using a buffer object.
class UniformBufferGLES : public UniformBuffer {
 public:
  UniformBufferGLES(const UniformBuffer::UniformBufferCreationParams& params);
  virtual ~UniformBufferGLES();

  virtual uint32_t GetElementOffset(uint32_t index) const;

  virtual void SetBufferElementData(const uint32_t index, const float* data, const size_t size);

  const float* GetBufferData() const { return buffer_data_; }

  bool GetBufferDirty() const { return buffer_dirty_; }
  void SetBufferDirty(bool dirty) { buffer_dirty_ = dirty; }

 private:
  float buffer_data_[kMaxUniformBufferFloatSize];
  uint32_t element_offsets_[kMaxUniforms];
  bool buffer_dirty_;
};
} // namespace simple_renderer

#endif // SIMPLERENDERER_UNIFORM_BUFFER_GLES_H_
