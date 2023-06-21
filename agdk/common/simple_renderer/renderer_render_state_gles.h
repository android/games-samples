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

#ifndef SIMPLERENDERER_RENDER_STATE_GLES_H_
#define SIMPLERENDERER_RENDER_STATE_GLES_H_

#include "renderer_render_state.h"
#include "renderer_shader_program_gles.h"
#include "renderer_uniform_buffer_gles.h"

namespace simple_renderer {

class RenderStateGLES : public RenderState {
 public:
  enum VertexAttribute : uint32_t {
    kAttribute_Position = 0,
    kAttribute_TexCoord,
    kAttribute_Color,
    kAttribute_Count
  };

  RenderStateGLES(const RenderStateCreationParams& params);
  virtual ~RenderStateGLES();

  const ShaderProgramGLES& GetShaderProgram() const;
  const UniformBufferGLES& GetUniformBuffer() const;

  void BindRenderState();
  void UnbindRenderState();

  void UpdateUniformData(bool force_update);

  GLenum GetPrimitiveType() const { return primitive_type_;}

  virtual void SetViewport(const RenderState::Viewport& viewport) {
    viewport_ = viewport;
  }

  virtual void SetScissorRect(const RenderState::ScissorRect& scissor_rect) {
    scissor_rect_ = scissor_rect;
  }

 private:
  void InitializeAttributes(const GLuint program_handle);
  void InitializeUniforms(const GLuint program_handle);
  void BindVertexAttributes();

  RenderState::ScissorRect scissor_rect_;
  RenderState::Viewport viewport_;
  std::shared_ptr<ShaderProgram> state_program_;
  std::shared_ptr<UniformBuffer> state_uniform_;
  VertexBuffer::VertexFormat state_vertex_layout_;
  GLenum primitive_type_;
  GLint sampler_location_;
  GLint uniform_locations_[UniformBuffer::kMaxUniforms];
  GLint vertex_attribute_locations_[kAttribute_Count];
  bool vertex_attribute_enabled_[kAttribute_Count];
  GLenum cull_function_;
  GLenum depth_function_;
  GLenum front_face_;
  GLenum src_blend_;
  GLenum dst_blend_;
  GLfloat line_width_;
  bool blend_enabled_;
  bool cull_enabled_;
  bool depth_test_;
  bool depth_write_;
  bool scissor_test_;
};

}

#endif // SIMPLERENDERER_RENDER_STATE_GLES_H_