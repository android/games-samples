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
#include "renderer_debug.h"

namespace simple_renderer {

const UniformBuffer::UniformBufferElement& UniformBuffer::GetElement(uint32_t index) const {
  if (index > GetBufferElementCount()) {
    index = 0;
  }
  return element_array_[index];
}

}