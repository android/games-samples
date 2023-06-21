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

#include "renderer_index_buffer_gles.h"
#include "renderer_debug.h"

namespace simple_renderer {

IndexBufferGLES::IndexBufferGLES(const IndexBuffer::IndexBufferCreationParams& params)
    : IndexBuffer(params) {
  index_buffer_object_ = 0;
  glGenBuffers(1, &index_buffer_object_);
  RENDERER_CHECK_GLES("glGenBuffers");
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_object_);
  RENDERER_CHECK_GLES("glBindBuffer GL_ELEMENT_ARRAY_BUFFER");
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, params.data_byte_size, params.index_data,
               GL_STATIC_DRAW);
  RENDERER_CHECK_GLES("glBufferData GL_ELEMENT_ARRAY_BUFFER");
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  RENDERER_CHECK_GLES("glBindBuffer GL_ELEMENT_ARRAY_BUFFER");
}

IndexBufferGLES::~IndexBufferGLES() {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  RENDERER_CHECK_GLES("glBindBuffer GL_ELEMENT_ARRAY_BUFFER");
  glDeleteBuffers(1, &index_buffer_object_);
  RENDERER_CHECK_GLES("glDeleteBuffers");
  index_buffer_object_ = 0;
}
}