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

#ifndef SIMPLERENDERER_RENDER_PASS_VK_H_
#define SIMPLERENDERER_RENDER_PASS_VK_H_

#include "renderer_render_pass.h"
#include "renderer_vk_includes.h"
#include <vector>

namespace simple_renderer {
class RenderPassVk : public RenderPass {
 public:

  RenderPassVk(const RenderPassCreationParams& params);
  virtual ~RenderPassVk();

  virtual void BeginRenderPass();
  virtual void EndRenderPass();

  VkRenderPass GetRenderPassVk() const { return render_pass_; }

  void PurgeFramebufferCache();

 private:

  struct FramebufferCache {
    VkImageView swapchain_image_view;
    VkFramebuffer framebuffer;
  };

  enum ClearAttachment {
    kClearAttachmentColor = 0,
    kClearAttachmentDepthStencil,
    kClearAttachmentCount
  };

  VkRenderPass render_pass_;
  VkClearValue clear_values_[kClearAttachmentCount];
  std::vector<FramebufferCache> framebuffer_cache_;
};
}

#endif // SIMPLERENDERER_RENDER_PASS_VK_H_
