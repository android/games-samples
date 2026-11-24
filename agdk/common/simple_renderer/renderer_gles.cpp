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

#include "renderer_gles.h"
#include "renderer_debug.h"
#include "renderer_index_buffer_gles.h"
#include "renderer_render_pass_gles.h"
#include "renderer_render_state_gles.h"
#include "renderer_shader_program_gles.h"
#include "renderer_texture_gles.h"
#include "renderer_uniform_buffer_gles.h"
#include "renderer_vertex_buffer_gles.h"
#include "display_manager.h"
#include "gles/graphics_api_gles_resources.h"

using namespace base_game_framework;

namespace simple_renderer {

static const char *kAstcExtensionString = "GL_OES_texture_compression_astc";

RendererGLES::RendererGLES() :
    // GL keeps objects used by queued commands alive until they are done, so released
    // resources are deleted at the next frame (with the context current) without frame tracking
    resources_(0),
    swapchain_handle_(Renderer::GetSwapchainHandle()),
    frame_active_(false),
    egl_context_(EGL_NO_CONTEXT),
    egl_display_(EGL_NO_DISPLAY),
    egl_surface_(EGL_NO_SURFACE) {
  // Call BeginFrame to make sure the context is set in case the user starts creating resources
  // immediately after initialization
  BeginFrame(Renderer::GetSwapchainHandle());
}

RendererGLES::~RendererGLES() {
}

void RendererGLES::RefreshEGLResources() {
  // The surface is destroyed and recreated when the window is lost and gained (and the
  // context with the swapchain), so don't keep using handles from an earlier frame
  DisplayManager& display_manager = DisplayManager::GetInstance();
  GraphicsAPIResourcesGLES graphics_api_resources_gles;
  if (display_manager.GetGraphicsAPIResourcesGLES(graphics_api_resources_gles)) {
    egl_context_ = graphics_api_resources_gles.egl_context;
    egl_display_ = graphics_api_resources_gles.egl_display;
  } else {
    egl_context_ = EGL_NO_CONTEXT;
    egl_display_ = EGL_NO_DISPLAY;
  }
  SwapchainFrameResourcesGLES swapchain_frame_resources_gles;
  const base_game_framework::DisplayManager::SwapchainFrameHandle frame_handle =
      display_manager.GetCurrentSwapchainFrame(swapchain_handle_);
  if (display_manager.GetSwapchainFrameResourcesGLES(frame_handle,
                                                     swapchain_frame_resources_gles)) {
    egl_surface_ = swapchain_frame_resources_gles.egl_surface;
  } else {
    // No valid swapchain (e.g. no window)
    egl_surface_ = EGL_NO_SURFACE;
  }
}

void RendererGLES::PrepareShutdown() {
  render_pass_ = nullptr;
  render_state_ = nullptr;
  // GL objects can only be deleted with their context current, and the window (with the
  // surface) may already be gone, e.g. when the Activity finishes (APP_CMD_TERM_WINDOW comes
  // before APP_CMD_DESTROY). Make the context current, without a surface then
  // (EGL_KHR_surfaceless_context). Resources whose context isn't current skip their GL
  // deletes (see IsGLESContextCurrent): they are deleted with their context.
  const EGLDisplay previous_display = eglGetCurrentDisplay();
  const EGLSurface previous_draw = eglGetCurrentSurface(EGL_DRAW);
  const EGLSurface previous_read = eglGetCurrentSurface(EGL_READ);
  const EGLContext previous_context = eglGetCurrentContext();
  RefreshEGLResources();
  bool made_current = false;
  if (egl_context_ != EGL_NO_CONTEXT && egl_display_ != EGL_NO_DISPLAY) {
    made_current = (eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_) ==
                    EGL_TRUE);
    if (!made_current) {
      RENDERER_ERROR("PrepareShutdown: eglMakeCurrent failed: 0x%x, GL objects are left to "
                     "the context", eglGetError());
    }
  }
  resources_.Shutdown("Renderer shutdown");
  // Leave the previously current context (or none) current
  if (made_current && (previous_context != egl_context_ || previous_draw != egl_surface_ ||
                       previous_read != egl_surface_)) {
    if (previous_context == EGL_NO_CONTEXT) {
      eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    } else {
      eglMakeCurrent(previous_display, previous_draw, previous_read, previous_context);
    }
  }
}

bool RendererGLES::GetFeatureAvailable(const RendererFeature feature) {
  bool supported = false;
  switch (feature) {
    case Renderer::kFeature_ASTC:
    {
      GLint extensionCount = 0;

      glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
      for (GLint i = 0; i < extensionCount; i++) {
        const GLubyte *extensionString = glGetStringi(GL_EXTENSIONS, i);
        if (strcmp(reinterpret_cast<const char *>(extensionString), kAstcExtensionString) == 0) {
          supported = true;
          break;
        }
      }
    }
      break;
    default:
      break;
  }
  return supported;
}

void RendererGLES::BeginFrame(
    const base_game_framework::DisplayManager::SwapchainHandle swapchain_handle) {
  swapchain_handle_ = swapchain_handle;
  RefreshEGLResources();
  frame_active_ = false;
  if (egl_surface_ != EGL_NO_SURFACE && egl_context_ != EGL_NO_CONTEXT) {
    if (eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_) == EGL_TRUE) {
      frame_active_ = true;
    } else {
      RENDERER_ERROR("eglMakeCurrent failed: %d", eglGetError());
    }
  }
  if (!frame_active_) {
    // No surface (e.g. no window) or context: skip the frame. Queued deletions wait for a
    // current context (GL deletes without one are ignored, leaking the objects).
    return;
  }
  resources_.ProcessDeleteQueue();

  // Make sure errors are cleared at top of frame (bounded, in case of a lost context)
  for (int i = 0; i < 32 && glGetError() != GL_NO_ERROR; ++i) {
  }
}

void RendererGLES::EndFrame() {
  if (frame_active_) {
    EndRenderPass();
  }

  // Clear current render pass and state
  render_pass_ = nullptr;
  render_state_ = nullptr;
}

void RendererGLES::SwapchainRecreated() {
  RefreshEGLResources();
}

void RendererGLES::EndRenderPass() {
  // Unbind any current render state
  if (render_state_ != nullptr) {
    RenderStateGLES& previous_state = *(static_cast<RenderStateGLES*>(render_state_.get()));
    previous_state.UnbindRenderState();
  }
  // Clear current render state
  render_state_ = nullptr;

  // Tell the current render pass to end
  if (render_pass_ != nullptr) {
    render_pass_->EndRenderPass();
  }
}

void RendererGLES::Draw(const uint32_t vertex_count, const uint32_t first_vertex) {
  if (!frame_active_) {
    // Skipped frame (see BeginFrame)
    return;
  }
  // Update any uniform data that might have changed between draw calls
  RenderStateGLES& state = *(static_cast<RenderStateGLES*>(render_state_.get()));
  // Viewport or scissor changed while the state was bound
  if (state.GetViewportScissorDirty()) {
    state.ApplyViewportAndScissor();
  }
  state.UpdateUniformData(true);

  glDrawArrays(state.GetPrimitiveType(), first_vertex, vertex_count);
  RENDERER_CHECK_GLES("glDrawArrays");
}

void RendererGLES::DrawIndexed(const uint32_t index_count, const uint32_t first_index) {
  if (!frame_active_) {
    // Skipped frame (see BeginFrame)
    return;
  }
  // Update any uniform data that might have changed between draw calls
  RenderStateGLES& state = *(static_cast<RenderStateGLES*>(render_state_.get()));
  // Viewport or scissor changed while the state was bound
  if (state.GetViewportScissorDirty()) {
    state.ApplyViewportAndScissor();
  }
  state.UpdateUniformData(true);

  // Currently fixed to 16-bit index values
  const void* first_index_offset = reinterpret_cast<const void*>((first_index * sizeof(uint16_t)));
  glDrawElements(state.GetPrimitiveType(),
                 index_count, GL_UNSIGNED_SHORT, first_index_offset);
  RENDERER_CHECK_GLES("glDrawElements");
}

void RendererGLES::SetRenderPass(std::shared_ptr<RenderPass> render_pass) {
  if (!frame_active_) {
    // Skipped frame (see BeginFrame)
    return;
  }
  // End any currently active render pass
  EndRenderPass();

  render_pass_ = render_pass;
  // Call BeginRenderPass on the new one
  if (render_pass != nullptr) {
    render_pass->BeginRenderPass();
  }
}

void RendererGLES::SetRenderState(std::shared_ptr<RenderState> render_state) {
  if (!frame_active_) {
    // Skipped frame (see BeginFrame)
    return;
  }
  // Exit early if we are setting a render state that is already the current one
  if (render_state_.get() == render_state.get()) {
    return;
  }

  // Unbind resources from any currently active render state
  if (render_state_ != nullptr) {
    RenderStateGLES& previous_state = *(static_cast<RenderStateGLES*>(render_state_.get()));
    previous_state.UnbindRenderState();
  }

  // Set the new state and bind its resources (none for a null state)
  render_state_ = render_state;
  if (render_state == nullptr) {
    return;
  }
  RenderStateGLES& state = *(static_cast<RenderStateGLES*>(render_state.get()));
  state.BindRenderState();
}

void RendererGLES::BindIndexBuffer(std::shared_ptr<IndexBuffer> index_buffer) {
  if (!frame_active_) {
    // Skipped frame (see BeginFrame)
    return;
  }
  if (index_buffer == nullptr) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    RENDERER_CHECK_GLES("glBindBuffer GL_ELEMENT_ARRAY_BUFFER reset");
  } else {
    const IndexBufferGLES& index = *static_cast<IndexBufferGLES *>(index_buffer.get());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index.GetIndexBufferObject());
    RENDERER_CHECK_GLES("glBindBuffer GL_ELEMENT_ARRAY_BUFFER");
  }
}

void RendererGLES::BindVertexBuffer(std::shared_ptr<VertexBuffer> vertex_buffer) {
  if (!frame_active_) {
    // Skipped frame (see BeginFrame)
    return;
  }
  if (vertex_buffer == nullptr) {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    RENDERER_CHECK_GLES("glBindBuffer GL_ARRAY_BUFFER reset");
  } else {
    const VertexBufferGLES& vertex = *static_cast<VertexBufferGLES *>(vertex_buffer.get());
    glBindBuffer(GL_ARRAY_BUFFER, vertex.GetVertexBufferObject());
    RENDERER_CHECK_GLES("glBindBuffer GL_ARRAY_BUFFER");
  }
}

void RendererGLES::BindTexture(std::shared_ptr<Texture> texture) {
  if (!frame_active_) {
    // Skipped frame (see BeginFrame)
    return;
  }
  if (texture == nullptr) {
    glBindTexture(GL_TEXTURE_2D, 0);
    RENDERER_CHECK_GLES("glBindTexture reset");
  } else {
    const TextureGLES& tex = *static_cast<TextureGLES *>(texture.get());
    glBindTexture(GL_TEXTURE_2D, tex.GetTextureObject());
    RENDERER_CHECK_GLES("glBindTexture");
  }
}

std::shared_ptr<IndexBuffer> RendererGLES::CreateIndexBuffer(
    const IndexBuffer::IndexBufferCreationParams& params) {
  return resources_.AddIndexBuffer(new IndexBufferGLES(params));
}

std::shared_ptr<RenderPass> RendererGLES::CreateRenderPass(
    const RenderPass::RenderPassCreationParams& params) {
  return resources_.AddRenderPass(new RenderPassGLES(params));
}

std::shared_ptr<RenderState> RendererGLES::CreateRenderState(
    const RenderState::RenderStateCreationParams& params) {
  return resources_.AddRenderState(new RenderStateGLES(params));
}

std::shared_ptr<ShaderProgram> RendererGLES::CreateShaderProgram(
    const ShaderProgram::ShaderProgramCreationParams& params) {
  return resources_.AddShaderProgram(new ShaderProgramGLES(params));
}

std::shared_ptr<Texture> RendererGLES::CreateTexture(const Texture::TextureCreationParams& params) {
  return resources_.AddTexture(new TextureGLES(params));
}

std::shared_ptr<UniformBuffer> RendererGLES::CreateUniformBuffer(
    const UniformBuffer::UniformBufferCreationParams& params) {
  return resources_.AddUniformBuffer(new UniformBufferGLES(params));
}

std::shared_ptr<VertexBuffer> RendererGLES::CreateVertexBuffer(
    const VertexBuffer::VertexBufferCreationParams& params) {
  return resources_.AddVertexBuffer(new VertexBufferGLES(params));
}

}
