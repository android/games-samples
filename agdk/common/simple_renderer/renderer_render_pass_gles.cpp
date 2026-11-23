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
#include <GLES3/gl3.h>
#include "renderer_debug.h"

namespace simple_renderer {

RenderPassGLES::RenderPassGLES(const RenderPassCreationParams& params) : RenderPass() {
  pass_params_ = params;
}

RenderPassGLES::~RenderPassGLES() {
}

void RenderPassGLES::BeginRenderPass() {
  // Only clear the aspects the pass has (attachment_flags), as RenderPassVk does
  const uint32_t flags = pass_params_.attachment_flags;
  GLbitfield mask = 0;
  if ((flags & kRenderPassAttachment_Color) != 0 &&
      pass_params_.color_load == kRenderPassColorLoad_Clear) {
    mask = GL_COLOR_BUFFER_BIT;
    glClearColor(pass_params_.color_clear[0],
                 pass_params_.color_clear[1],
                 pass_params_.color_clear[2],
                 pass_params_.color_clear[3]);
  }
  if ((flags & kRenderPassAttachment_Depth) != 0 &&
      pass_params_.depth_load == kRenderPassDepthLoad_Clear) {
    mask |= GL_DEPTH_BUFFER_BIT;
    glClearDepthf(pass_params_.depth_clear);
  }
  if ((flags & kRenderPassAttachment_Stencil) != 0 &&
      pass_params_.stencil_load == kRenderPassStencilLoad_Clear) {
    mask |= GL_STENCIL_BUFFER_BIT;
    glClearStencil(pass_params_.stencil_clear);
  }
  if (mask) {
    // glClear honors the write masks and the scissor test, which the last render state
    // may have left set (e.g. depth_write = false, or a scissor rect): clear the whole
    // buffers, as a Vulkan render pass load does. Render states set these again when bound.
    if (mask & GL_COLOR_BUFFER_BIT) {
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
    if (mask & GL_DEPTH_BUFFER_BIT) {
      glDepthMask(GL_TRUE);
    }
    if (mask & GL_STENCIL_BUFFER_BIT) {
      glStencilMask(~0u);
    }
    glDisable(GL_SCISSOR_TEST);
    glClear(mask);
    RENDERER_CHECK_GLES("glClear");
  }
}

void RenderPassGLES::EndRenderPass() {

}

}