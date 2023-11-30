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

#ifndef agdktunnel_gfx_manager_hpp
#define agdktunnel_gfx_manager_hpp

#include "simplegeom.hpp"
#include "simple_renderer/renderer_interface.h"

class GfxManager {
 public:
  enum GfxType : int32_t {
    kGfxType_BasicLines = 0,             // Basic geometry, lines with colors
    kGfxType_BasicLinesNoDepthTest,      // kGfxType_BasicLines, no depth testing
    kGfxType_BasicThickLinesNoDepthTest, // kGfxType_BasicLines, thicker idth, no depth testing
    kGfxType_BasicTris,             // Basic geometry, tris with colors (depth testing)
    kGfxType_BasicTrisNoDepthTest,  // kGfxType_BasicTris, but with depth testing disabled
    kGfxType_OurTris,               // Triangle rendering with 'our' shader (color/texture/lighting)
    kGfxType_OurTrisNoDepthTest,    // OurTris, but no depth test
    kGfxType_Count
  };

  enum BasicUniformElements : int32_t {
    kBasicUniform_MVP = 0,
    kBasicUniform_Tint
  };

  enum OurUniformElements : int32_t {
    kOurUniform_MVP = 0,
    kOurUniform_PointLightPos,
    kOurUniform_PointLightColor,
    kOurUniform_Tint
  };

  GfxManager(bool useVulkan, const int32_t width, const int32_t height);
  ~GfxManager();

  void SetMainRenderPass();

  void SetRenderState(GfxType gfxType);

  std::shared_ptr<simple_renderer::UniformBuffer> GetUniformBuffer(GfxType gfxType);

  void RenderSimpleGeom(const GfxType gfxType, const float *mvpMat, SimpleGeom *sg);

  void UpdateDisplaySize(const int32_t width, const int32_t height);

 private:
  void CreateRenderResources(bool useVulkan, const int32_t width, const int32_t height);
  void CreateShaderPrograms(bool useVulkan);
  void CreateRenderPasses();
  void CreateRenderStates(const int32_t width, const int32_t height);
  void CreateUniformBuffers();
  void DestroyRenderResources();

  std::shared_ptr<simple_renderer::RenderPass> mMainRenderPass;
  std::shared_ptr<simple_renderer::RenderState> mRenderStates[kGfxType_Count];
  std::shared_ptr<simple_renderer::ShaderProgram> mTrivialShaderProgram;
  std::shared_ptr<simple_renderer::ShaderProgram> mOurShaderProgram;
  std::shared_ptr<simple_renderer::UniformBuffer> mUniformBuffers[kGfxType_Count];
};
#endif // agdktunnel_gfx_manager_hpp
