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

RendererGLES::RendererGLES() {
  GraphicsAPIResourcesGLES graphics_api_resources_gles;
  SwapchainFrameResourcesGLES swapchain_frame_resources_gles;
  DisplayManager& display_manager = DisplayManager::GetInstance();
  display_manager.GetGraphicsAPIResourcesGLES(graphics_api_resources_gles);
  const base_game_framework::DisplayManager::SwapchainFrameHandle frame_handle =
      display_manager.GetCurrentSwapchainFrame(Renderer::GetSwapchainHandle());
  display_manager.GetSwapchainFrameResourcesGLES(frame_handle, swapchain_frame_resources_gles);
  egl_context_ = graphics_api_resources_gles.egl_context;
  egl_display_ = swapchain_frame_resources_gles.egl_display;
  egl_surface_ = swapchain_frame_resources_gles.egl_surface;

  // Call BeginFrame to make sure the context is set in case the user starts creating resources
  // immediately after initialization
  BeginFrame(Renderer::GetSwapchainHandle());
}

RendererGLES::~RendererGLES() {
}

void RendererGLES::PrepareShutdown() {
  render_pass_ = nullptr;
  render_state_ = nullptr;
  resources_.ProcessDeleteQueue();
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
    const base_game_framework::DisplayManager::SwapchainHandle /*swapchain_handle*/) {
  resources_.ProcessDeleteQueue();
  EGLBoolean result = eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_);
  if (result == EGL_FALSE) {
    RENDERER_ERROR("eglMakeCurrent failed: %d", eglGetError())
  }

  // Make sure errors are cleared at top of frame
  GLenum gl_error = glGetError();
  while (gl_error != GL_NO_ERROR) {
    gl_error = glGetError();
  }
}

void RendererGLES::EndFrame() {
  EndRenderPass();

  // Clear current render pass
  render_pass_ = nullptr;
}

void RendererGLES::SwapchainRecreated() {

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
  // Update any uniform data that might have changed between draw calls
  RenderStateGLES& state = *(static_cast<RenderStateGLES*>(render_state_.get()));
  state.UpdateUniformData(true);

  glDrawArrays(state.GetPrimitiveType(), first_vertex, vertex_count);
  RENDERER_CHECK_GLES("glDrawArrays");
}

void RendererGLES::DrawIndexed(const uint32_t index_count, const uint32_t first_index) {
  // Update any uniform data that might have changed between draw calls
  RenderStateGLES& state = *(static_cast<RenderStateGLES*>(render_state_.get()));
  state.UpdateUniformData(true);

  // Currently fixed to 16-bit index values
  const void* first_index_offset = reinterpret_cast<const void*>((first_index * sizeof(uint16_t)));
  glDrawElements(state.GetPrimitiveType(),
                 index_count, GL_UNSIGNED_SHORT, first_index_offset);
  RENDERER_CHECK_GLES("glDrawElements");
}

void RendererGLES::SetRenderPass(std::shared_ptr<RenderPass> render_pass) {
  // End any currently active render pass
  EndRenderPass();

  render_pass_ = render_pass;
  // Call BeginRenderPass on the new one
  if (render_pass != nullptr) {
    render_pass->BeginRenderPass();
  }
}

void RendererGLES::SetRenderState(std::shared_ptr<RenderState> render_state) {
  // Exit early if we are setting a render state that is already the current one
  if (render_state_.get() == render_state.get()) {
    return;
  }

  // Unbind resources from any currently active render state
  if (render_state_ != nullptr) {
    RenderStateGLES& previous_state = *(static_cast<RenderStateGLES*>(render_state_.get()));
    previous_state.UnbindRenderState();
  }

  // Set the new state and bind its resources
  render_state_ = render_state;
  RenderStateGLES& state = *(static_cast<RenderStateGLES*>(render_state.get()));
  state.BindRenderState();
}

void RendererGLES::BindIndexBuffer(std::shared_ptr<IndexBuffer> index_buffer) {
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

void RendererGLES::DestroyIndexBuffer(std::shared_ptr<IndexBuffer> index_buffer) {
  resources_.QueueDeleteIndexBuffer(index_buffer);
}

std::shared_ptr<RenderPass> RendererGLES::CreateRenderPass(
    const RenderPass::RenderPassCreationParams& params) {
  return resources_.AddRenderPass(new RenderPassGLES(params));
}

void RendererGLES::DestroyRenderPass(std::shared_ptr<RenderPass> render_pass) {
  resources_.QueueDeleteRenderPass(render_pass);
}

std::shared_ptr<RenderState> RendererGLES::CreateRenderState(
    const RenderState::RenderStateCreationParams& params) {
  return resources_.AddRenderState(new RenderStateGLES(params));
}

void RendererGLES::DestroyRenderState(std::shared_ptr<RenderState> render_state) {
  resources_.QueueDeleteRenderState(render_state);
}

std::shared_ptr<ShaderProgram> RendererGLES::CreateShaderProgram(
    const ShaderProgram::ShaderProgramCreationParams& params) {
  return resources_.AddShaderProgram(new ShaderProgramGLES(params));
}

void RendererGLES::DestroyShaderProgram(std::shared_ptr<ShaderProgram> shader_program) {
  resources_.QueueDeleteShaderProgram(shader_program);
}

std::shared_ptr<Texture> RendererGLES::CreateTexture(const Texture::TextureCreationParams& params) {
  return resources_.AddTexture(new TextureGLES(params));
}

void RendererGLES::DestroyTexture(std::shared_ptr<Texture> texture) {
  resources_.QueueDeleteTexture(texture);
}

std::shared_ptr<UniformBuffer> RendererGLES::CreateUniformBuffer(
    const UniformBuffer::UniformBufferCreationParams& params) {
  return resources_.AddUniformBuffer(new UniformBufferGLES(params));
}

void RendererGLES::DestroyUniformBuffer(std::shared_ptr<UniformBuffer> uniform_buffer) {
  resources_.QueueDeleteUniformBuffer(uniform_buffer);
}

std::shared_ptr<VertexBuffer> RendererGLES::CreateVertexBuffer(
    const VertexBuffer::VertexBufferCreationParams& params) {
  return resources_.AddVertexBuffer(new VertexBufferGLES(params));
}

void RendererGLES::DestroyVertexBuffer(std::shared_ptr<VertexBuffer> vertex_buffer) {
  resources_.QueueDeleteVertexBuffer(vertex_buffer);
}

}
