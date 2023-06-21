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

#include "renderer_render_pass_gles.h"
#include "renderer_debug.h"

namespace simple_renderer {

RenderPassGLES::RenderPassGLES(const RenderPassCreationParams& params) : RenderPass() {
  pass_params_ = params;
}

RenderPassGLES::~RenderPassGLES() {
}

void RenderPassGLES::BeginRenderPass() {
  GLbitfield mask = 0;
  if (pass_params_.color_load == kRenderPassColorLoad_Clear) {
    mask = GL_COLOR_BUFFER_BIT;
    glClearColor(pass_params_.color_clear[0],
                 pass_params_.color_clear[1],
                 pass_params_.color_clear[2],
                 pass_params_.color_clear[3]);
  }
  if (pass_params_.depth_load == kRenderPassDepthLoad_Clear) {
    mask |= GL_DEPTH_BUFFER_BIT;
    glClearDepthf(pass_params_.depth_clear);
  }
  if (pass_params_.stencil_load == kRenderPassStencilLoad_Clear) {
    mask |= GL_STENCIL_BUFFER_BIT;
    glClearStencil(pass_params_.stencil_clear);
  }
  if (mask) {
    glClear(mask);
    RENDERER_CHECK_GLES("glClear");
  }
}

void RenderPassGLES::EndRenderPass() {

}

}