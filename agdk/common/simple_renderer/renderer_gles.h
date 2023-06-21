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

#ifndef SIMPLERENDERER_GLES_H_
#define SIMPLERENDERER_GLES_H_

#include "renderer_interface.h"
#include "renderer_resources.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>

namespace simple_renderer {

/**
 * @brief A subclass implementation of the base Renderer class for the
 * OpenGL ES API. This class should not be used directly, except to call the
 * static function ::SetEGLResources to configure OpenGL resources prior to
 * the first call to Renderer::GetInstance.
 */
class RendererGLES : public Renderer {
 public:
/**
 * @brief Set pointers to resources needed by the renderer to use the OpenGL ES API.
 * @param context Pointer to the `EGLContext` that should be used by the renderer.
 * @param display Pointer to the `EGLDisplay` that should be used by the renderer.
 * @param surface Pointer to the `EGLSurface` that should be used by the renderer.
 */
  static void SetEGLResources(EGLContext context, EGLDisplay display, EGLSurface surface);

  RendererGLES();
  virtual ~RendererGLES();

  virtual bool GetFeatureAvailable(const RendererFeature feature);

  virtual void BeginFrame();
  virtual void EndFrame();

  virtual void Draw(const uint32_t vertex_count, const uint32_t first_vertex);
  virtual void DrawIndexed(const uint32_t index_count, const uint32_t first_index);

  virtual void SetRenderPass(std::shared_ptr<RenderPass> render_pass);
  virtual void SetRenderState(std::shared_ptr<RenderState> render_state);

  // Resource binds
  virtual void BindIndexBuffer(std::shared_ptr<IndexBuffer> index_buffer);
  virtual void BindVertexBuffer(std::shared_ptr<VertexBuffer> vertex_buffer);
  virtual void BindTexture(std::shared_ptr<Texture> texture);

  // Resource creation and destruction
  virtual std::shared_ptr<IndexBuffer> CreateIndexBuffer(
      const IndexBuffer::IndexBufferCreationParams& params);
  virtual void DestroyIndexBuffer(std::shared_ptr<IndexBuffer> index_buffer);

  virtual std::shared_ptr<RenderPass> CreateRenderPass(
      const RenderPass::RenderPassCreationParams& params);
  virtual void DestroyRenderPass(std::shared_ptr<RenderPass> render_pass);

  virtual std::shared_ptr<RenderState> CreateRenderState(
      const RenderState::RenderStateCreationParams& params);
  virtual void DestroyRenderState(std::shared_ptr<RenderState> render_state);

  virtual std::shared_ptr<ShaderProgram> CreateShaderProgram(
      const ShaderProgram::ShaderProgramCreationParams& params);
  virtual void DestroyShaderProgram(std::shared_ptr<ShaderProgram> shader_program);

  virtual std::shared_ptr<Texture> CreateTexture(
      const Texture::TextureCreationParams& params);
  virtual void DestroyTexture(std::shared_ptr<Texture> texture);

  virtual std::shared_ptr<UniformBuffer> CreateUniformBuffer(
      const UniformBuffer::UniformBufferCreationParams& params);
  virtual void DestroyUniformBuffer(std::shared_ptr<UniformBuffer> uniform_buffer);

  virtual std::shared_ptr<VertexBuffer> CreateVertexBuffer(
      const VertexBuffer::VertexBufferCreationParams& params);
  virtual void DestroyVertexBuffer(std::shared_ptr<VertexBuffer> vertex_buffer);

 private:
  void EndRenderPass();

  RendererResources resources_;

  std::shared_ptr<RenderPass> render_pass_;
  std::shared_ptr<RenderState> render_state_;

  static EGLContext egl_context_;
  static EGLDisplay egl_display_;
  static EGLSurface egl_surface_;
};

} // namespace simple_renderer

#endif // SIMPLERENDERER_GLES_H_
