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

#ifndef SIMPLERENDERER_RENDER_STATE_H_
#define SIMPLERENDERER_RENDER_STATE_H_

#include <cstdint>
#include <memory>
#include <string>

#include "renderer_render_pass.h"
#include "renderer_shader_program.h"
#include "renderer_uniform_buffer.h"
#include "renderer_vertex_buffer.h"

namespace simple_renderer {

/**
 * @brief The base class definition for the `RenderState` class of SimpleRenderer.
 * Use the `Renderer` class interface to create and destroy `RenderState` objects.
 * The `RenderState` class does not currently support dynamic modification of any of its
 * state parameters. All values are fixed at creation time.
 */
class RenderState {
 public:
  /**
   * @brief Blend function options for source and destination blend factors
   * when pixel blending is enabled
   */
  enum BlendFunction {
    /** @brief blend func (0) */
    kBlendZero = 0,
    /** @brief blend func (1) */
    kBlendOne,
    /** @brief blend func (source color) */
    kBlendSourceColor,
    /** @brief blend func (1 - source color) */
    kBlendOneMinusSourceColor,
    /** @brief blend func (destination color) */
    kBlendDestColor,
    /** @brief blend func (1 - destination color) */
    kBlendOneMinusDestColor,
    /** @brief blend func (source alpha) */
    kBlendSourceAlpha,
    /** @brief blend func (1 - source alpha) */
    kBlendOneMinusSourceAlpha,
    /** @brief blend func (destination alpha) */
    kBlendDestAlpha,
    /** @brief blend func (1 - destination alpha) */
    kBlendOneMinusDestAlpha,
    /** @brief blend func (constant color) */
    kBlendConstantColor,
    /** @brief blend func (1 - constant color) */
    kBlendOneMinusConstantColor,
    /** @brief blend func (constant alpha) */
    kBlendConstantAlpha,
    /** @brief blend func (1 - constant alpha) */
    kBlendOneMinusConstantAlpha,
    /** @brief blend func (src saturate alpha) */
    kBlendAlphaSaturate,
    /** @brief Blend function count */
    kBlendFunctionCount
  };
  /**
   * @brief Face culling mode when culling enabled
   */
  enum CullFace {
    /** @brief Cull front face */
    kCullFront = 0,
    /** @brief Cull back face */
    kCullBack,
    /** @brief Cull front and back faces */
    kCullFrontAndBack,
    /** @brief Cull mode count */
    kCullFaceCount
  };

  /**
   * @brief Depth compare functions for depth buffer compare operations
   */
  enum DepthCompareFunction {
    /** @brief Pass depth test never */
    kNever = 0,
    /** @brief Pass depth test always */
    kAlways,
    /** @brief Pass depth test if depth == depth_buffer */
    kEqual,
    /** @brief Pass depth test if depth != depth_buffer */
    kNotEqual,
    /** @brief Pass depth test if depth < depth_buffer */
    kLess,
    /** @brief Pass depth test if depth <= depth_buffer */
    kLessEqual,
    /** @brief Pass depth test if depth > depth_buffer */
    kGreater,
    /** @brief Pass depth test if depth > depth_buffer */
    kGreaterEqual,
    /** @brief Count of depth compare functions */
    kDepthCompareCount
  };

  /**
   * @brief Front facing orientation of polygons when culling enabled
   */
  enum FrontFace {
    /** @brief Front face clockwise order */
    kFrontFaceClockwise = 0,
    /** @brief Front face counterclockwise order */
    kFrontFaceCounterclockwise,
    /** @brief Front face count */
    kFrontFaceCount
  };

  /**
   * @brief Primitive type to use (triangles, lines) in a draw call for this render state
   */
  enum RenderPrimitiveType {
    /** @brief Draw calls render a list of triangles */
    kTriangleList = 0,
    /** @brief Draw calls render a list of lines */
    kLineList,
    /** @brief Count of available primitive types */
    kPrimitiveCount
  };

  /**
   * @brief Structure that defines the viewport that will be used when rendering using
   * this render state
   */
  struct Viewport {
    /** @brief X pixel coordinate of the left edge of the viewport */
    int32_t x;
    /** @brief Y pixel coordinate of the bottom edge of the viewport */
    int32_t y;
    /** @brief pixel width of the viewport */
    int32_t width;
    /** @brief height width of the viewport */
    int32_t height;
    /** @brief minimum value for the near clipping plane */
    float min_depth;
    /** @brief maximum value for the far clipping plane */
    float max_depth;
  };

  /**
   * @brief Structure that defines a scissor rect that can be used to act as a cutout
   * in the rendering window
   */
  struct ScissorRect {
    /** @brief X pixel coordinate of the left edge of the viewport */
    int32_t x;
    /** @brief Y pixel coordinate of the bottom edge of the viewport */
    int32_t y;
    /** @brief pixel width of the viewport */
    int32_t width;
    /** @brief pixel height of the viewport */
    int32_t height;
  };

  /**
   * @brief A structure holding required parameters to create a new `RenderState`.
   * Passed to the Renderer::CreateRenderState function.
   */  struct RenderStateCreationParams {
    /** @brief The scissor rect to apply to the rendering window */
    ScissorRect scissor_rect;
    /** @brief The viewport to use for rendering */
    Viewport viewport;
    /** @brief A pointer to the render pass used for this render state */
    std::shared_ptr<RenderPass> render_pass;
    /** @brief A pointer to the shader program (vertex+fragment) used for this render state */
    std::shared_ptr<ShaderProgram> state_program;
    /** @brief A shared pointer to the uniform buffer used for this render state */
    std::shared_ptr<UniformBuffer> state_uniform;
    /** @brief The vertex format used by the shader program in this render state */
    VertexBuffer::VertexFormat state_vertex_layout;
    /** @brief Function to apply to source values when blending in this render state */
    BlendFunction blend_source_function;
    /** @brief Function to apply to destination values when blending in this render state */
    BlendFunction blend_dest_function;
    /** @brief The face mode for face culling in this render state */
    CullFace cull_face;
    /** @brief The depth compare function used for depth testing in this render state */
    DepthCompareFunction depth_function;
    /** @brief The front face orientation for polygons in this render state */
    FrontFace front_face;
    /** @brief The primitive type used by draw calls in this render state */
    RenderPrimitiveType primitive_type;
    /** @brief Line width for lines rendered in this render state */
    float line_width;
    /** @brief Whether src/dest pixel blending is enabled in this render state */
    bool blend_enabled;
    /** @brief Cull polygons based on cull_face_ setting in this render state */
    bool cull_enabled;
    /** @brief Whether depth testing is enabled for draw calls in this render state */
    bool depth_test;
    /** @brief Whether writing depth values to the depth buffer is enabled in this render state */
    bool depth_write;
    /** @brief Whether scissor testing is enabled for draw calls in this render state */
    bool scissor_test;
  };

   virtual void SetViewport(const RenderState::Viewport& viewport) = 0;

   virtual void SetScissorRect(const RenderState::ScissorRect& scissor_rect) = 0;

  /**
   * @brief Base class destructor, do not call directly.
   */
  virtual ~RenderState() {}

  /**
   * @brief Retrieve the debug name string associated with the `RenderState`
   * @result A string containing the debug name.
   */
  const std::string& GetRenderStateDebugName() const { return render_state_debug_name_; }
  /**
   * @brief Set a debug name string to associate with the `RenderState`
   * @param name A string containing the debug name.
   */
  void SetRenderStateDebugName(const std::string& name) { render_state_debug_name_ = name; }

 protected:
  RenderState() {}

 private:
  std::string render_state_debug_name_;
};
}

#endif // SIMPLERENDERER_RENDER_STATE_H_