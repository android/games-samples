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

#include "renderer_render_state_gles.h"
#include "renderer_debug.h"

namespace simple_renderer {

static const char* vertex_attribute_names[RenderStateGLES::kAttribute_Count] = {
 "a_Position",
 "a_TexCoord",
 "a_Color"
};

static constexpr GLboolean vertex_attribute_normalized[RenderStateGLES::kAttribute_Count] = {
  GL_FALSE, // kAttribute_Position
  GL_FALSE, // kAttribute_TexCoord
  GL_FALSE  // kAttribute_Color
};

static constexpr GLsizei vertex_format_strides[VertexBuffer::kVertexFormat_Count] = {
 12, // kVertexFormat_P3
 20, // kVertexFormat_P3T2
 28, // kVertexFormat_P3C4
 36  // kVertexFormat_P3T2C4
};

static constexpr GLenum blend_function_values[RenderState::kBlendFunctionCount] = {
  GL_ZERO,
  GL_ONE,
  GL_SRC_COLOR,
  GL_ONE_MINUS_SRC_COLOR,
  GL_DST_COLOR,
  GL_ONE_MINUS_DST_COLOR,
  GL_SRC_ALPHA,
  GL_ONE_MINUS_SRC_ALPHA,
  GL_DST_ALPHA,
  GL_ONE_MINUS_DST_ALPHA,
  GL_CONSTANT_COLOR,
  GL_ONE_MINUS_CONSTANT_COLOR,
  GL_CONSTANT_ALPHA,
  GL_ONE_MINUS_CONSTANT_ALPHA,
  GL_SRC_ALPHA_SATURATE
};

static constexpr GLenum cull_function_values[RenderState::kCullFaceCount] = {
  GL_FRONT,
  GL_BACK,
  GL_FRONT_AND_BACK
};

static constexpr GLenum depth_function_values[RenderState::kDepthCompareCount] = {
  GL_NEVER,
  GL_ALWAYS,
  GL_EQUAL,
  GL_NOTEQUAL,
  GL_LESS,
  GL_LEQUAL,
  GL_GREATER,
  GL_GEQUAL
};

static constexpr GLenum front_facing_values[RenderState::kFrontFaceCount] = {
  GL_CW,
  GL_CCW
};

static constexpr GLenum primitive_values[RenderState::kPrimitiveCount] = {
  GL_TRIANGLES,
  GL_LINES
};

static constexpr size_t kPositionAttributeOffset = 0;
static constexpr size_t kTextureAttributeOffset = 12;
static constexpr size_t kColorAttributeNoTextureOffset = 12;
static constexpr size_t kColorAttributeWithTextureOffset = 20;

static const char* sampler_name = "u_Sampler";

RenderStateGLES::RenderStateGLES(const RenderStateCreationParams& params) :
    scissor_rect_(params.scissor_rect),
    viewport_(params.viewport),
    state_program_(params.state_program),
    state_uniform_(params.state_uniform),
    state_vertex_layout_(params.state_vertex_layout),
    primitive_type_(primitive_values[params.primitive_type]),
    cull_function_(cull_function_values[params.cull_face]),
    depth_function_(depth_function_values[params.depth_function]),
    front_face_(front_facing_values[params.front_face]),
    src_blend_(blend_function_values[params.blend_source_function]),
    dst_blend_(blend_function_values[params.blend_dest_function]),
    line_width_(params.line_width),
    blend_enabled_(params.blend_enabled),
    cull_enabled_(params.cull_enabled),
    depth_test_(params.depth_test),
    depth_write_(params.depth_write),
    scissor_test_(params.scissor_test) {
  for (uint32_t i = 0; i < kAttribute_Count; ++i) {
    vertex_attribute_enabled_[i] = false;
  }

  const ShaderProgramGLES& program = GetShaderProgram();
  const GLuint program_handle = program.GetProgramHandle();
  glUseProgram(program_handle);
  RENDERER_CHECK_GLES("glUseProgram");

  InitializeAttributes(program_handle);
  InitializeUniforms(program_handle);

  glUseProgram(0);
  RENDERER_CHECK_GLES("glUseProgram");
}

RenderStateGLES::~RenderStateGLES() {
  state_program_ = nullptr;
  state_uniform_ = nullptr;
}

const ShaderProgramGLES& RenderStateGLES::GetShaderProgram() const {
  return *(static_cast<ShaderProgramGLES *>(state_program_.get()));
}

const UniformBufferGLES& RenderStateGLES::GetUniformBuffer() const {
  return *(static_cast<UniformBufferGLES *>(state_uniform_.get()));
}

void RenderStateGLES::InitializeAttributes(const GLuint program_handle) {
  vertex_attribute_locations_[kAttribute_Position] = glGetAttribLocation(
      program_handle, vertex_attribute_names[kAttribute_Position]);
  RENDERER_CHECK_GLES("glGetUniformLocation (position)");
  RENDERER_ASSERT(vertex_attribute_locations_[kAttribute_Position] >= 0)

  if (state_vertex_layout_ == VertexBuffer::kVertexFormat_P3T2 ||
      state_vertex_layout_ == VertexBuffer::kVertexFormat_P3T2C4) {
    vertex_attribute_locations_[kAttribute_TexCoord] = glGetAttribLocation(
        program_handle, vertex_attribute_names[kAttribute_TexCoord]);
    RENDERER_CHECK_GLES("glGetUniformLocation (texture)");
    RENDERER_ASSERT(vertex_attribute_locations_[kAttribute_TexCoord] >= 0)

    sampler_location_ = glGetUniformLocation(
        program_handle, sampler_name);
    RENDERER_CHECK_GLES("glGetUniformLocation (sampler)");
    RENDERER_ASSERT(sampler_location_ >= 0)
  } else {
    vertex_attribute_locations_[kAttribute_TexCoord] = -1;
    sampler_location_ = -1;
  }

  if (state_vertex_layout_ == VertexBuffer::kVertexFormat_P3C4 ||
      state_vertex_layout_ == VertexBuffer::kVertexFormat_P3T2C4) {
    vertex_attribute_locations_[kAttribute_Color] = glGetAttribLocation(
        program_handle, vertex_attribute_names[kAttribute_Color]);
    RENDERER_CHECK_GLES("glGetUniformLocation (color)");
    RENDERER_ASSERT(vertex_attribute_locations_[kAttribute_Color] >= 0)
  } else {
    vertex_attribute_locations_[kAttribute_Color] = -1;
  }
}

void RenderStateGLES::InitializeUniforms(const GLuint program_handle) {
  const UniformBufferGLES& buffer = GetUniformBuffer();
  uint32_t i = 0;
  RENDERER_ASSERT(buffer.GetBufferElementCount() <= UniformBuffer::kMaxUniforms)
  while (i < buffer.GetBufferElementCount()) {
    const char* element_name = buffer.GetElement(i).element_name;
    uniform_locations_[i] = glGetUniformLocation(program_handle, element_name);
    RENDERER_CHECK_GLES("glGetUniformLocation (shader uniform)");
    RENDERER_ASSERT(uniform_locations_[i] >= 0)
    ++i;
  }

  while (i < UniformBuffer::kMaxUniforms) {
    uniform_locations_[i] = -1;
    ++i;
  }
}

void RenderStateGLES::BindRenderState() {
  if (blend_enabled_) {
    glEnable(GL_BLEND);
    glBlendFunc(src_blend_, dst_blend_);
  } else {
    glDisable(GL_BLEND);
  }

  if (cull_enabled_) {
    glEnable(GL_CULL_FACE);
    glCullFace(cull_function_);
    glFrontFace(front_face_);
  } else {
    glDisable(GL_CULL_FACE);
  }

  if (depth_test_) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(depth_function_);
  } else {
    glDisable(GL_DEPTH_TEST);
  }
  glDepthMask(depth_write_);

  if (scissor_test_) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(scissor_rect_.x, scissor_rect_.y,
              scissor_rect_.width, scissor_rect_.height);
  } else {
    glDisable(GL_SCISSOR_TEST);
  }

  glLineWidth(line_width_);

  glViewport(viewport_.x, viewport_.y, viewport_.width, viewport_.height);
  glDepthRangef(viewport_.min_depth, viewport_.max_depth);

  const ShaderProgramGLES& program = GetShaderProgram();
  const GLuint program_handle = program.GetProgramHandle();
  glUseProgram(program_handle);
  RENDERER_CHECK_GLES("glUseProgram");
}

void RenderStateGLES::UnbindRenderState() {
  for (uint32_t i = 0; i < kAttribute_Count; ++i) {
    if (vertex_attribute_enabled_[i]) {
      glDisableVertexAttribArray(vertex_attribute_locations_[i]);
      RENDERER_CHECK_GLES("glDisableVertexAttribArray");
      vertex_attribute_enabled_[i] = false;
    }
  }
  glUseProgram(0);
  RENDERER_CHECK_GLES("glUseProgram");
}

void RenderStateGLES::BindVertexAttributes() {
  const GLsizei vertex_stride = vertex_format_strides[state_vertex_layout_];
  // Configure vertex attributes based on the active vertex buffer format
  // We always have position, and may have texture, color, or texture+color
  glVertexAttribPointer(vertex_attribute_locations_[kAttribute_Position],
                        3, GL_FLOAT, vertex_attribute_normalized[kAttribute_Position],
                        vertex_stride,
                        reinterpret_cast<void*>(kPositionAttributeOffset));
  RENDERER_CHECK_GLES("glVertexAttribPointer (pos)");
  glEnableVertexAttribArray(vertex_attribute_locations_[kAttribute_Position]);
  RENDERER_CHECK_GLES("glEnableVertexAttribArray (pos)");
  vertex_attribute_enabled_[kAttribute_Position] = true;

  if (state_vertex_layout_ == VertexBuffer::kVertexFormat_P3T2 ||
      state_vertex_layout_ == VertexBuffer::kVertexFormat_P3T2C4) {
    glVertexAttribPointer(vertex_attribute_locations_[kAttribute_TexCoord],
                          2, GL_FLOAT, vertex_attribute_normalized[kAttribute_TexCoord],
                          vertex_stride,
                          reinterpret_cast<void*>(kTextureAttributeOffset));
    RENDERER_CHECK_GLES("glVertexAttribPointer (tex)");
    glEnableVertexAttribArray(vertex_attribute_locations_[kAttribute_TexCoord]);
    RENDERER_CHECK_GLES("glEnableVertexAttribArray (tex)");
    vertex_attribute_enabled_[kAttribute_TexCoord] = true;

    // Also update sampler location here, we only use texture stage 0
    glUniform1i(sampler_location_, 0);
    RENDERER_CHECK_GLES("glUniform1i (sampler)");
  }

  if (state_vertex_layout_ == VertexBuffer::kVertexFormat_P3C4 ||
      state_vertex_layout_ == VertexBuffer::kVertexFormat_P3T2C4) {
    const size_t color_offset = (state_vertex_layout_ == VertexBuffer::kVertexFormat_P3C4) ?
        kColorAttributeNoTextureOffset : kColorAttributeWithTextureOffset;

    glVertexAttribPointer(vertex_attribute_locations_[kAttribute_Color],
                          4, GL_FLOAT, vertex_attribute_normalized[kAttribute_Color],
                          vertex_stride,
                          reinterpret_cast<void*>(color_offset));
    RENDERER_CHECK_GLES("glVertexAttribPointer (color)");
    glEnableVertexAttribArray(vertex_attribute_locations_[kAttribute_Color]);
    RENDERER_CHECK_GLES("glEnableVertexAttribArray (color)");
    vertex_attribute_enabled_[kAttribute_Color] = true;
  }
}

void RenderStateGLES::UpdateUniformData(bool force_update) {
  UniformBufferGLES& buffer = *(static_cast<UniformBufferGLES *>(state_uniform_.get()));
  // We don't need to rebind vertex attributes if we didn't change the vertex buffer,
  // but at the moment we always do it
  BindVertexAttributes();
  if (buffer.GetBufferDirty() || force_update) {
    const float* buffer_data = buffer.GetBufferData();

    for (uint32_t i = 0; i < buffer.GetBufferElementCount(); ++i) {
      const UniformBuffer::UniformBufferElement& element = buffer.GetElement(i);
      const float* element_data = &buffer_data[buffer.GetElementOffset(i)];
      switch (element.element_type) {
        case UniformBuffer::kBufferElement_Float4:
          glUniform4f(uniform_locations_[i],
                      element_data[0],
                      element_data[1],
                      element_data[2],
                      element_data[3]);
          RENDERER_CHECK_GLES("glUniform4f");
          break;
        case UniformBuffer::kBufferElement_Matrix44:
          glUniformMatrix4fv(uniform_locations_[i], 1, GL_FALSE, element_data);
          RENDERER_CHECK_GLES("glUniformMatrix4fv");
          break;
      }
    }
    buffer.SetBufferDirty(false);
  }
}

}