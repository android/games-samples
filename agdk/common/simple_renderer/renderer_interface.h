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

#ifndef SIMPLERENDERER_INTERFACE_H_
#define SIMPLERENDERER_INTERFACE_H_

#include "renderer_index_buffer.h"
#include "renderer_render_pass.h"
#include "renderer_render_state.h"
#include "renderer_shader_program.h"
#include "renderer_texture.h"
#include "renderer_uniform_buffer.h"
#include "renderer_vertex_buffer.h"
#include "display_manager.h"

#include <cstdint>
#include <memory>

namespace simple_renderer {

/**
 * @brief The base class definition for the Renderer class of SimpleRenderer.
 * This class is used to create and destroy rendering resources, bind them and
 * render primitives in the context of a render frame.
 */
class Renderer {
 public:
 /**
  * @brief Which graphics API the Renderer should select for use at initialization
  */
  enum RendererAPI : int32_t {
    kAPI_GLES = 0, ///< Use the OpenGL ES 3.0 API
    kAPI_Vulkan ///< Use the Vulkan 1.0 API
  };

  /**
   * @brief RenderFeature enum to pass to ::GetFeatureAvailable to determine
   * if the device supports a particular feature
   */
  enum RendererFeature : int32_t {
    kFeature_ASTC = 0 ///< Does the device support ASTC textures
  };

/**
 * @brief Retrieve the graphics API in use by the renderer.
 * @return `RendererAPI` enum value of the active graphics API
 */
  static RendererAPI GetRendererAPI() { return Renderer::renderer_api_; }

/**
 * @brief Set the graphics API that the renderer should use. This should be called
 * prior to calling ::GetInstance for the first time.
 * @param api `RendererAPI` enum value of the graphics API to use
 */
  static void SetRendererAPI(const RendererAPI api);

/**
 * @brief Retrieve the Display Manager swapchain in use by the renderer.
 * @return Handle to a Display Manager swapchain
 */
  static base_game_framework::DisplayManager::SwapchainHandle GetSwapchainHandle() {
    return Renderer::swapchain_handle_;
  }

/**
 * @brief Set a handle to the Display Manager swapchain  that the renderer should use.
 * This should be called prior to calling ::GetInstance for the first time.
 * @param api `swapchain_handle` Handle to a Display Manager swapchain
 */
  static void SetSwapchainHandle(
      base_game_framework::DisplayManager::SwapchainHandle swapchain_handle);

/**
 * @brief Retrieve an instance of the renderer interface. The first time this is called
 * it will construct and initialize the renderer. Before calling for the first time you must
 * call Renderer::SetRendererAPI to set the graphics API to use.
 * @return Reference to the `Renderer` interface class.
 */
  static Renderer& GetInstance();
/**
 * @brief Shuts down the renderer and frees all graphical resources in the pending deletion queue.
 */
  static void ShutdownInstance();
/**
 * @brief Class destructor, do not call directly, use ::ShutdownInstance.
 */
  virtual ~Renderer();

/**
 * @brief Determine if a feature is available for the current API on the currently
 * running device
 * @param feature Enum of the feature to query
 * @return true if supported, false if unsupported
 */
  virtual bool GetFeatureAvailable(const RendererFeature feature) = 0;

/**
 * @brief Tell the renderer to set up to begin rendering a frame of draw calls.
 */
  virtual void BeginFrame(
      const base_game_framework::DisplayManager::SwapchainHandle swapchain_handle) = 0;
/**
 * @brief Tell the renderer all draw calls for this frame have been completed.
 */
  virtual void EndFrame() = 0;

/**
 * @brief Tell the renderer the swapchain was recreated
 */
  virtual void SwapchainRecreated() = 0;

/**
 * @brief Draw a sequence of vertices using bound resources and the current render state.
 * @param vertex_count Number of vertices to draw from the bound vertex buffer.
 * @param first_vertex Vertex offset into the bound vertex buffer to begin drawing from.
 */
  virtual void Draw(const uint32_t vertex_count, const uint32_t first_vertex) = 0;
/**
 * @brief Draw a sequence of indexed vertices using bound resources and the curent render state.
 * @param index_count Number of indices to draw from the bound index buffer.
 * @param first_index Index offset into the bound index buffer to begin drawing from.
 */
  virtual void DrawIndexed(const uint32_t index_count, const uint32_t first_index) = 0;

/**
 * @brief Set a render pass as the current one for rendering. Binds the drawable resources
 * associated with the render pass.
 * @param render_pass A shared pointer to a renderer `RenderPass`.
 */
  virtual void SetRenderPass(std::shared_ptr<RenderPass> render_pass) = 0;
/**
 * @brief Set the current render state to use for rendering. Binds resources associated with
 * the render state. Note that the `RenderPass` object associated with the `RenderState` being
 * used must have previously been set as current using the ::SetRenderPass function.
 * @param render_state A shared pointer to a renderer `RenderState`.
 */
  virtual void SetRenderState(std::shared_ptr<RenderState> render_state) = 0;

/**
 * @brief Bind an index buffer for use in draw calls.
 * @param index_buffer A shared pointer to a renderer `IndexBuffer`.
 */
  virtual void BindIndexBuffer(std::shared_ptr<IndexBuffer> index_buffer) = 0;
/**
 * @brief Bind a vertex buffer for use in draw calls.
 * @param vertex_buffer A shared pointer to a renderer `VertexBuffer`.
 */
  virtual void BindVertexBuffer(std::shared_ptr<VertexBuffer> vertex_buffer) = 0;
/**
 * @brief Bind a texture for use in draw calls.
 * @param texture A shared pointer to a renderer `Texture`.
 */
  virtual void BindTexture(std::shared_ptr<Texture> texture) = 0;

/**
 * @brief Create a renderer `IndexBuffer`.
 * @param params A reference to a `IndexBufferCreationParams` struct with creation parameters.
 * @return A shared pointer to a renderer `IndexBuffer`.
 */
  virtual std::shared_ptr<IndexBuffer> CreateIndexBuffer(
      const IndexBuffer::IndexBufferCreationParams& params) = 0;
/**
 * @brief Destroy a renderer `IndexBuffer`.
 * @param index_buffer A shared pointer to a renderer `IndexBuffer`. Do not retain any other
 * instances of the shared pointer after calling the destroy function. The resources
 * is not immediately deleted, but put in a delete queue. Deletion will happen at the
 * next ::BeginFrame or ::ShutdownInstance.
 */
  virtual void DestroyIndexBuffer(std::shared_ptr<IndexBuffer> index_buffer) = 0;

/**
 * @brief Create a renderer `RenderPass`.
 * @param params A reference to a `RenderPassCreationParams` struct with creation parameters.
 * @return A shared pointer to a renderer `RenderPass`.
 */
  virtual std::shared_ptr<RenderPass> CreateRenderPass(
      const RenderPass::RenderPassCreationParams& params) = 0;
/**
 * @brief Destroy a renderer `RenderPass`.
 * @param render_pass A shared pointer to a renderer `RenderPass`. Do not retain any other
 * instances of the shared pointer after calling the destroy function. The resources
 * is not immediately deleted, but put in a delete queue. Deletion will happen at the
 * next ::BeginFrame or ::ShutdownInstance.
 */
  virtual void DestroyRenderPass(std::shared_ptr<RenderPass> render_pass) = 0;

/**
 * @brief Create a renderer `RenderState`.
 * @param params A reference to a `RenderStateCreationParams` struct with creation parameters.
 * @return A shared pointer to a renderer `RenderState`.
 */
  virtual std::shared_ptr<RenderState> CreateRenderState(
      const RenderState::RenderStateCreationParams& params) = 0;
/**
 * @brief Destroy a renderer `RenderState`.
 * @param render_state A shared pointer to a renderer `RenderState`. Do not retain any other
 * instances of the shared pointer after calling the destroy function. The resources
 * is not immediately deleted, but put in a delete queue. Deletion will happen at the
 * next ::BeginFrame or ::ShutdownInstance.
 */
  virtual void DestroyRenderState(std::shared_ptr<RenderState> render_state) = 0;

/**
 * @brief Create a renderer `ShaderProgram`.
 * @param params A reference to a `ShaderProgramCreationParams` struct with creation parameters.
 * @return A shared pointer to a renderer `ShaderProgram`.
 */
  virtual std::shared_ptr<ShaderProgram> CreateShaderProgram(
      const ShaderProgram::ShaderProgramCreationParams& params) = 0;
/**
 * @brief Destroy a renderer `ShaderProgram`.
 * @param shader_program A shared pointer to a renderer `ShaderProgram`. Do not retain any other
 * instances of the shared pointer after calling the destroy function. The resources
 * is not immediately deleted, but put in a delete queue. Deletion will happen at the
 * next ::BeginFrame or ::ShutdownInstance.
 */
  virtual void DestroyShaderProgram(std::shared_ptr<ShaderProgram> shader_program) = 0;

/**
 * @brief Create a renderer `Texture`.
 * @param params A reference to a `TextureCreationParams` struct with creation parameters.
 * @return A shared pointer to a renderer `Texture`.
 */
  virtual std::shared_ptr<Texture> CreateTexture(
      const Texture::TextureCreationParams& params) = 0;
/**
 * @brief Destroy a renderer `Texture`.
 * @param texture A shared pointer to a renderer `Texture`. Do not retain any other
 * instances of the shared pointer after calling the destroy function. The resources
 * is not immediately deleted, but put in a delete queue. Deletion will happen at the
 * next ::BeginFrame or ::ShutdownInstance.
 */
  virtual void DestroyTexture(std::shared_ptr<Texture> texture) = 0;

/**
 * @brief Create a renderer `UniformBuffer`.
 * @param params A reference to a `UniformBufferCreationParams` struct with creation parameters.
 * @return A shared pointer to a renderer `UniformBuffer`.
 */
  virtual std::shared_ptr<UniformBuffer> CreateUniformBuffer(
      const UniformBuffer::UniformBufferCreationParams& params) = 0;
/**
 * @brief Destroy a renderer `UniformBuffer`.
 * @param uniform_buffer A shared pointer to a renderer `UniformBuffer`. Do not retain any other
 * instances of the shared pointer after calling the destroy function. The resources
 * is not immediately deleted, but put in a delete queue. Deletion will happen at the
 * next ::BeginFrame or ::ShutdownInstance.
 */
  virtual void DestroyUniformBuffer(std::shared_ptr<UniformBuffer> uniform_buffer) = 0;

/**
 * @brief Create a renderer `VertexBuffer`.
 * @param params A reference to a `VertexBufferCreationParams` struct with creation parameters.
 * @return A shared pointer to a renderer `VertexBuffer`.
 */
  virtual std::shared_ptr<VertexBuffer> CreateVertexBuffer(
      const VertexBuffer::VertexBufferCreationParams& params) = 0;
/**
 * @brief Destroy a renderer `VertexBuffer`.
 * @param vertex_buffer A shared pointer to a renderer `VertexBuffer`. Do not retain any other
 * instances of the shared pointer after calling the destroy function. The resources
 * is not immediately deleted, but put in a delete queue. Deletion will happen at the
 * next ::BeginFrame or ::ShutdownInstance.
 */
  virtual void DestroyVertexBuffer(std::shared_ptr<VertexBuffer> vertex_buffer) = 0;

 protected:
  Renderer();

  virtual void PrepareShutdown() = 0;

  static Renderer* GetInstancePtr() { return instance_.get(); }

 private:
  static RendererAPI renderer_api_;
  static base_game_framework::DisplayManager::SwapchainHandle swapchain_handle_;
  static std::unique_ptr<Renderer> instance_;
};
}

#endif //SIMPLERENDERER_INTERFACE_H_
