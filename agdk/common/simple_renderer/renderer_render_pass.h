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

#ifndef SIMPLERENDERER_RENDER_PASS_H_
#define SIMPLERENDERER_RENDER_PASS_H_

#include <cstdint>
#include <memory>
#include <string>

namespace simple_renderer {

/**
 * @brief The base class definition for the `RenderPass` class of SimpleRenderer.
 * Use the `Renderer` class interface to create and destroy `RenderPass` objects.
 * The `RenderPass` class does not currently support configuring render targets,
 * it is assumed you are rendering to the default drawable attachments
 */
class RenderPass {
 public:
  /**
   * @brief Bitflags for attachment status for the render pass
   */
  enum RenderPassAttachmentFlags : uint32_t {
    /** @brief Has a color attachment if set */
    kRenderPassAttachment_Color = (1U << 0),
    /** @brief Has a color attachment if set */
    kRenderPassAttachment_Depth = (1U << 1),
    /** @brief Has a color attachment if set */
    kRenderPassAttachment_Stencil = (1U << 2)
  };

  /**
   * @brief Load options for the color attachment when beginning a render pass
   */
  enum RenderPassColorLoadOperation : uint32_t {
    /** @brief Don't care what happens when loading the color attachment */
    kRenderPassColorLoad_DontCare = 0,
    /** @brief Clear the color attachment when it is loaded */
    kRenderPassColorLoad_Clear
  };

  /**
   * @brief Store options for the color attachment when ending a render pass
   */
  enum RenderPassColorStoreOperation : uint32_t {
    /** @brief Don't care what happens when storing the color attachment */
    kRenderPassColorStore_DontCare = 0,
    /** @brief Write the color attachment data out when storing */
    kRenderPassColorStore_Write
  };

  /**
   * @brief Load options for the depth buffer attachment when beginning a render pass
   */
  enum RenderPassDepthLoadOperation : uint32_t {
    /** @brief Don't care what happens when loading the depth buffer attachment */
    kRenderPassDepthLoad_DontCare = 0,
    /** @brief Clear the depth buffer attachment when it is loaded */
    kRenderPassDepthLoad_Clear
  };

  /**
   * @brief Store options for the depth buffer attachment when ending a render pass
   */
  enum RenderPassDepthStoreOperation : uint32_t {
    /** @brief Don't care what happens when storing the depth buffer attachment */
    kRenderPassDepthStore_DontCare = 0,
    /** @brief Write the depth buffer attachment data out when storing */
    kRenderPassDepthStore_Write
  };

  /**
   * @brief Load options for the stencil buffer attachment when beginning a render pass
   */
  enum RenderPassStencilLoadOperation : uint32_t {
    /** @brief Don't care what happens when loading the stencil buffer attachment */
    kRenderPassStencilLoad_DontCare = 0,
    /** @brief Clear the depth buffer attachment when it is loaded */
    kRenderPassStencilLoad_Clear
  };

  /**
   * @brief Store options for the stencil buffer attachment when ending a render pass
   */
  enum RenderPassStencilStoreOperation : uint32_t {
    /** @brief Don't care what happens when storing the stencil buffer attachment */
    kRenderPassStencilStore_DontCare = 0,
    /** @brief Write the stencil buffer attachment data out when storing */
    kRenderPassStencilStore_Write
  };

  /**
   * @brief A structure holding required parameters to create a new `RenderPass`.
   * Passed to the Renderer::CreateRenderPass function.
   */
  struct RenderPassCreationParams {
    /** @brief The operation to perform when loading the color attachment */
    RenderPassColorLoadOperation color_load;
    /** @brief The operation to perform when storing the color attachment */
    RenderPassColorStoreOperation color_store;
    /** @brief The operation to perform when loading the depth buffer attachment */
    RenderPassDepthLoadOperation depth_load;
    /** @brief The operation to perform when storing the depth buffer attachment */
    RenderPassDepthStoreOperation depth_store;
    /** @brief The operation to perform when loading the stencil buffer attachment */
    RenderPassStencilLoadOperation stencil_load;
    /** @brief The operation to perform when storing the stencil buffer attachment */
    RenderPassStencilStoreOperation stencil_store;
    /** @brief Bitflags (RenderPassAttachmentFlags) of active attachments */
    uint32_t attachment_flags;
    /** @brief The RGBA values to use when clearing a color attachment */
    float color_clear[4];
    /** @brief The depth value to use when clearing a depth buffer attachment */
    float depth_clear;
    /** @brief The stencil value to use when clearing a stencil buffer attachment */
    int32_t stencil_clear;
  };

  /**
   * @brief Base class destructor, do not call directly.
   */
  virtual ~RenderPass() {}

  /**
   * @brief Begin render pass initialization function, do not call directly.
   */
  virtual void BeginRenderPass() = 0;
  /**
   * @brief End render pass completion function, do not call directly.
   */
  virtual void EndRenderPass() = 0;

  /**
   * @brief Retrieve the debug name string associated with the `RenderPass`
   * @result A string containing the debug name.
   */
  const std::string& GetRenderPassDebugName() const { return render_pass_debug_name_; }
  /**
   * @brief Set a debug name string to associate with the `RenderPass`
   * @param name A string containing the debug name.
   */
  void SetRenderPassDebugName(const std::string& name) { render_pass_debug_name_ = name; }

 protected:

  RenderPass() {
    render_pass_debug_name_ = "noname";
    memset(&pass_params_, 0, sizeof(RenderPassCreationParams));
  }

  RenderPassCreationParams pass_params_;

 private:
  std::string render_pass_debug_name_;
};
}

#endif // SIMPLERENDERER_RENDER_PASS_H_