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

#include "renderer_uniform_buffer_gles.h"
#include <cstring>
#include "renderer_debug.h"

namespace simple_renderer {

UniformBufferGLES::UniformBufferGLES(const UniformBuffer::UniformBufferCreationParams& params) :
  UniformBuffer(params) {
  // The base class lays out the elements
  for (uint32_t i = 0; i < GetBufferElementCount(); ++i) {
    const UniformBufferElement& element = GetElement(i);
    RENDERER_LOG("Element %u : name: %s type: %u set: %u index: %u",
                 i, element.element_name, element.element_type,
                 element.element_bind_set, element.element_bind_index);
  }
  memset(buffer_data_, 0, sizeof(buffer_data_));
  SetBufferDirty(true);
}

UniformBufferGLES::~UniformBufferGLES() {
}

void UniformBufferGLES::SetBufferElementData(const uint32_t index, const float* data,
                                         const size_t size) {
  if (CopyElementData(index, data, size, buffer_data_)) {
    SetBufferDirty(true);
  }
}
}
