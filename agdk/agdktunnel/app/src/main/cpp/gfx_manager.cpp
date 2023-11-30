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

#include "gfx_manager.hpp"
#include "common.hpp"
#include "tunnel_engine.hpp"
#include "data/our_shader.inl"

#define ARRAY_COUNTOF(array) (sizeof(array) / sizeof(array[0]))

using namespace simple_renderer;

#define NORMAL_LINE_WIDTH (1.0f)
#define TEXT_LINE_WIDTH (4.0f)

static const char* kOur_SPIRV_Vertex = "shaders/our.vert.spv";
static const char* kOur_SPIRV_Fragment = "shaders/our.frag.spv";
static const char* kTrivial_SPIRV_Vertex = "shaders/trivial.vert.spv";
static const char* kTrivial_SPIRV_Fragment = "shaders/trivial.frag.spv";

// Static utility functions
static const char* GetOurVertShaderSourceGLES() {
  return OUR_VERTEX_SHADER_SOURCE;
}

static const char* GetOurFragShaderSourceGLES() {
  return OUR_FRAG_SHADER_SOURCE;
}

static const char *GetTrivialVertShaderSourceGLES() {
  return "uniform mat4 u_MVP;            \n"
         "uniform vec4 u_Tint;           \n"
         "attribute vec4 a_Position;     \n"
         "attribute vec4 a_Color;        \n"
         "varying vec4 v_Color;          \n"
         "void main()                    \n"
         "{                              \n"
         "   v_Color = a_Color * u_Tint; \n"
         "   gl_Position = u_MVP         \n"
         "               * a_Position;   \n"
         "}                              \n";
}

static const char *GetTrivialFragShaderSourceGLES() {
  return "precision mediump float;       \n"
         "varying vec4 v_Color;          \n"
         "void main()                    \n"
         "{                              \n"
         "   gl_FragColor = v_Color;     \n"
         "}";
}

static void LoadSPIRVAsset(const char* asset_filename, void** asset_ptr, size_t* asset_size) {
  GameAssetManager* asset_manager = TunnelEngine::GetInstance()->GetGameAssetManager();
  *asset_size = asset_manager->GetGameAssetSize(asset_filename);
  if (*asset_size > 0) {
    *asset_ptr = malloc(*asset_size);
    MY_ASSERT(asset_manager->LoadGameAsset(asset_filename, *asset_size, *asset_ptr));
  } else {
    MY_ASSERT(false);
  }
}

// Uniform buffer data declarations, these need to be persistent for the renderer
static constexpr UniformBuffer::UniformBufferElement basic_uniform_elements[] = {
    { UniformBuffer::kBufferElement_Matrix44, UniformBuffer::kElementStageVertexFlag,
      0, 0, "u_MVP" },
    { UniformBuffer::kBufferElement_Float4, UniformBuffer::kElementStageVertexFlag,
      0, 1, "u_Tint" }
};
static constexpr size_t kBasicUniformSize = 64 + 16;
static constexpr uint32_t kBasicUniformOffset = 0;

static constexpr UniformBuffer::UniformBufferElement our_uniform_elements[] = {
    { UniformBuffer::kBufferElement_Matrix44, UniformBuffer::kElementStageVertexFlag,
      0, 0, "u_MVP" },
    { UniformBuffer::kBufferElement_Float4,
      UniformBuffer::kElementStageVertexFlag,
      0, 1, "u_PointLightPos" },
    { UniformBuffer::kBufferElement_Float4,
      UniformBuffer::kElementStageVertexFlag | UniformBuffer::kElementStageFragmentFlag,
      0, 2, "u_PointLightColor" },
    { UniformBuffer::kBufferElement_Float4, UniformBuffer::kElementStageFragmentFlag,
      0, 3, "u_Tint" }
};
static constexpr size_t kOurUniformSize = 64 + 16 + 16 + 16;
static constexpr uint32_t kOurUniformVertexOffset = 0;
static constexpr uint32_t kOurUniformVertexSize = 64 + 16;
static constexpr uint32_t kOurUniformFragmentOffset = 64 + 16;
static constexpr uint32_t kOurUniformFragmentSize = 16 + 16;

GfxManager::GfxManager(bool useVulkan, const int32_t width, const int32_t height) {
  CreateRenderResources(useVulkan, width, height);
}

GfxManager::~GfxManager() {
  DestroyRenderResources();
}

void GfxManager::CreateRenderResources(bool useVulkan, const int32_t width, const int32_t height) {
  CreateRenderPasses();
  CreateShaderPrograms(useVulkan);
  CreateUniformBuffers();
  CreateRenderStates(width, height);
}

void GfxManager::CreateShaderPrograms(bool useVulkan) {
  Renderer& renderer = Renderer::GetInstance();
  if (useVulkan) {
    ShaderProgram::ShaderProgramCreationParams trivialShaderParams = {
        0, 0,
        0, 0};
    LoadSPIRVAsset(kTrivial_SPIRV_Vertex,
                   &trivialShaderParams.vertex_shader_data,
                   &trivialShaderParams.vertex_data_byte_count);
    LoadSPIRVAsset(kTrivial_SPIRV_Fragment,
                   &trivialShaderParams.fragment_shader_data,
                   &trivialShaderParams.fragment_data_byte_count);
    mTrivialShaderProgram = renderer.CreateShaderProgram(trivialShaderParams);
    free(trivialShaderParams.vertex_shader_data);
    free(trivialShaderParams.fragment_shader_data);

    ShaderProgram::ShaderProgramCreationParams ourShaderParams = {
        0, 0,
        0, 0};
    LoadSPIRVAsset(kOur_SPIRV_Vertex,
                   &ourShaderParams.vertex_shader_data,
                   &ourShaderParams.vertex_data_byte_count);
    LoadSPIRVAsset(kOur_SPIRV_Fragment,
                   &ourShaderParams.fragment_shader_data,
                   &ourShaderParams.fragment_data_byte_count);
    mOurShaderProgram = renderer.CreateShaderProgram(ourShaderParams);
    free(ourShaderParams.vertex_shader_data);
    free(ourShaderParams.fragment_shader_data);
  } else {
    ShaderProgram::ShaderProgramCreationParams trivialShaderParams = {
        (void*) GetTrivialFragShaderSourceGLES(),
        (void*) GetTrivialVertShaderSourceGLES(),
        strlen(GetTrivialFragShaderSourceGLES()),
        strlen(GetTrivialVertShaderSourceGLES())};
    mTrivialShaderProgram = renderer.CreateShaderProgram(trivialShaderParams);
    ShaderProgram::ShaderProgramCreationParams ourShaderParams = {
        (void*)GetOurFragShaderSourceGLES(),
        (void*)GetOurVertShaderSourceGLES(),
        strlen(GetOurFragShaderSourceGLES()),
        strlen(GetOurVertShaderSourceGLES())};
    mOurShaderProgram = renderer.CreateShaderProgram(ourShaderParams);
  }
}

void GfxManager::CreateRenderPasses() {
  Renderer& renderer = Renderer::GetInstance();
  RenderPass::RenderPassCreationParams mainRenderPassParams = {
      RenderPass::kRenderPassColorLoad_Clear,
      RenderPass::kRenderPassColorStore_Write,
      RenderPass::kRenderPassDepthLoad_Clear,
      RenderPass::kRenderPassDepthStore_DontCare,
      RenderPass::kRenderPassStencilLoad_DontCare,
      RenderPass::kRenderPassStencilStore_DontCare,
      (RenderPass::kRenderPassAttachment_Color | RenderPass::kRenderPassAttachment_Depth |
       RenderPass::kRenderPassAttachment_Stencil),
      { 0.0f, 0.0f, 0.0f, 1.0f },
      1.0f,
      0
  };
  mMainRenderPass = renderer.CreateRenderPass(mainRenderPassParams);
}

void GfxManager::CreateRenderStates(const int32_t width, const int32_t height) {
  Renderer& renderer = Renderer::GetInstance();

  const base_game_framework::GraphicsAPIFeatures& api_features =
      base_game_framework::DisplayManager::GetInstance().GetGraphicsAPIFeatures();
  const float text_line_width =
      api_features.HasGraphicsFeature(
          base_game_framework::GraphicsAPIFeatures::kGraphicsFeature_Wide_Lines) ?
          TEXT_LINE_WIDTH : NORMAL_LINE_WIDTH;

  RenderState::RenderStateCreationParams basicStateParams {
      {0, 0, width, height},
      {0, 0, width, height, 0.0f, 1.0f},
      mMainRenderPass,
      mTrivialShaderProgram,
      mUniformBuffers[kGfxType_BasicLines],
      VertexBuffer::kVertexFormat_P3C4,
      RenderState::kBlendOne, RenderState::kBlendZero,
      RenderState::kCullBack,
      RenderState::kLess,
      RenderState::kFrontFaceCounterclockwise,
      RenderState::kLineList,
      NORMAL_LINE_WIDTH, false, false, true, true, false
  };
  mRenderStates[kGfxType_BasicLines] = renderer.CreateRenderState(basicStateParams);

  basicStateParams.depth_test = false;
  basicStateParams.state_uniform = mUniformBuffers[kGfxType_BasicLinesNoDepthTest];
  mRenderStates[kGfxType_BasicLinesNoDepthTest] = renderer.CreateRenderState(basicStateParams);

  basicStateParams.line_width = text_line_width;
  basicStateParams.state_uniform = mUniformBuffers[kGfxType_BasicThickLinesNoDepthTest];
  mRenderStates[kGfxType_BasicThickLinesNoDepthTest] = renderer.CreateRenderState(basicStateParams);

  basicStateParams.line_width = NORMAL_LINE_WIDTH;
  basicStateParams.primitive_type = RenderState::kTriangleList;
  basicStateParams.depth_test = true;
  basicStateParams.state_uniform = mUniformBuffers[kGfxType_BasicTris];
  mRenderStates[kGfxType_BasicTris] = renderer.CreateRenderState(basicStateParams);

  basicStateParams.depth_test = false;
  basicStateParams.state_uniform = mUniformBuffers[kGfxType_BasicTrisNoDepthTest];
  mRenderStates[kGfxType_BasicTrisNoDepthTest] = renderer.CreateRenderState(basicStateParams);

  RenderState::RenderStateCreationParams our_state_params {
      {0, 0, width, height},
      {0, 0, width, height, 0.0f, 1.0f},
      mMainRenderPass,
      mOurShaderProgram,
      mUniformBuffers[kGfxType_OurTris],
      VertexBuffer::kVertexFormat_P3T2C4,
      RenderState::kBlendOne, RenderState::kBlendZero,
      RenderState::kCullBack,
      RenderState::kLess,
      RenderState::kFrontFaceCounterclockwise,
      RenderState::kTriangleList,
      NORMAL_LINE_WIDTH, false, false, true, true, false
  };
  mRenderStates[kGfxType_OurTris] = renderer.CreateRenderState(our_state_params);

  our_state_params.depth_test = false;
  our_state_params.state_uniform = mUniformBuffers[kGfxType_OurTrisNoDepthTest];
  mRenderStates[kGfxType_OurTrisNoDepthTest] = renderer.CreateRenderState(our_state_params);
}

void GfxManager::CreateUniformBuffers() {
  Renderer& renderer = Renderer::GetInstance();

  UniformBuffer::UniformBufferCreationParams basicUniformParams = {
      basic_uniform_elements, ARRAY_COUNTOF(basic_uniform_elements),
      (UniformBuffer::kBufferFlag_UpdateDynamicPerDraw |
          UniformBuffer::kBufferFlag_UsePushConstants),
      {kBasicUniformOffset, kBasicUniformSize,
       UniformBuffer::kUnusedStageRange, UniformBuffer::kUnusedStageRange},
      kBasicUniformSize
  };
  // Separate buffer object for each render type, but they share the same layout
  mUniformBuffers[kGfxType_BasicLines] = renderer.CreateUniformBuffer(basicUniformParams);
  mUniformBuffers[kGfxType_BasicLinesNoDepthTest] =
      renderer.CreateUniformBuffer(basicUniformParams);
  mUniformBuffers[kGfxType_BasicThickLinesNoDepthTest] =
      renderer.CreateUniformBuffer(basicUniformParams);
  mUniformBuffers[kGfxType_BasicTris] = renderer.CreateUniformBuffer(basicUniformParams);
  mUniformBuffers[kGfxType_BasicTrisNoDepthTest] = renderer.CreateUniformBuffer(basicUniformParams);

  UniformBuffer::UniformBufferCreationParams ourUniformParams = {
      our_uniform_elements, ARRAY_COUNTOF(our_uniform_elements),
      (UniformBuffer::kBufferFlag_UpdateDynamicPerDraw |
                  UniformBuffer::kBufferFlag_UsePushConstants),
      {kOurUniformVertexOffset, kOurUniformVertexSize,
       kOurUniformFragmentOffset, kOurUniformFragmentSize},
      kOurUniformSize
  };
  mUniformBuffers[kGfxType_OurTris] = renderer.CreateUniformBuffer(ourUniformParams);
  mUniformBuffers[kGfxType_OurTrisNoDepthTest] = renderer.CreateUniformBuffer(ourUniformParams);
}

void GfxManager::DestroyRenderResources() {
  Renderer& renderer = Renderer::GetInstance();
  for (int32_t i = kGfxType_BasicLines; i < kGfxType_Count; ++i) {
    renderer.DestroyRenderState(mRenderStates[i]);
    mRenderStates[i] = nullptr;
    renderer.DestroyUniformBuffer(mUniformBuffers[i]);
    mUniformBuffers[i] = nullptr;
  }
  renderer.DestroyShaderProgram(mTrivialShaderProgram);
  mTrivialShaderProgram = nullptr;
  renderer.DestroyShaderProgram(mOurShaderProgram);
  mOurShaderProgram = nullptr;
  renderer.DestroyRenderPass(mMainRenderPass);
  mMainRenderPass = nullptr;
}

void GfxManager::SetMainRenderPass() {
  Renderer& renderer = Renderer::GetInstance();
  renderer.SetRenderPass(mMainRenderPass);
}

void GfxManager::SetRenderState(GfxType gfxType) {
  if (gfxType < kGfxType_Count) {
    Renderer &renderer = Renderer::GetInstance();
    renderer.SetRenderState(mRenderStates[gfxType]);
  } else {
    MY_ASSERT(false);
  }
}

std::shared_ptr<simple_renderer::UniformBuffer> GfxManager::GetUniformBuffer(GfxType gfxType) {
  if (gfxType < kGfxType_Count) {
    return mUniformBuffers[gfxType];
  }
  MY_ASSERT(false);
  return nullptr;
}

void GfxManager::RenderSimpleGeom(const GfxType gfxType, const float *mvpMat, SimpleGeom *sg) {
  MY_ASSERT(gfxType < kGfxType_Count);
  simple_renderer::Renderer& renderer = simple_renderer::Renderer::GetInstance();
  SetRenderState(gfxType);

  mUniformBuffers[gfxType]->SetBufferElementData(kBasicUniform_MVP,
                                       mvpMat, sizeof(float) * 16);
  const float tintData[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  mUniformBuffers[gfxType]->SetBufferElementData(kBasicUniform_Tint,
                                       tintData, UniformBuffer::kElementSize_Float4);
  renderer.BindVertexBuffer(sg->vertex_buffer_);
  if (sg->index_buffer_.get() != NULL) {
    renderer.BindIndexBuffer(sg->index_buffer_);
    renderer.DrawIndexed(sg->index_buffer_->GetBufferElementCount(), 0);
  } else {
    renderer.Draw(sg->vertex_buffer_->GetBufferElementCount(), 0);
  }
}

void GfxManager::UpdateDisplaySize(const int32_t width, const int32_t height) {
  RenderState::ScissorRect scissor_rect = {0, 0, width, height};
  RenderState::Viewport viewport = {0, 0, width, height, 0.0f, 1.0f};

  for (int32_t i = 0; i < kGfxType_Count; ++i) {
    mRenderStates[i]->SetScissorRect(scissor_rect);
    mRenderStates[i]->SetViewport(viewport);
  }
}