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

#include "renderer_render_pass_vk.h"
#include "renderer_debug.h"
#include "renderer_vk.h"

#define ARRAY_COUNTOF(array) (sizeof(array) / sizeof(array[0]))

namespace simple_renderer {

RenderPassVk::RenderPassVk(const RenderPassCreationParams& params)
    : RenderPass()
    , framebuffer_cache_() {
  pass_params_ = params;
  RendererVk &renderer = RendererVk::GetInstanceVk();

  VkAttachmentDescription attachment_array[2];
  memset(attachment_array, 0, sizeof(attachment_array));

  attachment_array[0].format = renderer.GetSwapchainColorFormat();
  attachment_array[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachment_array[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachment_array[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachment_array[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment_array[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment_array[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachment_array[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  attachment_array[1].format = renderer.GetSwapchainDepthStencilFormat();
  attachment_array[1].samples = VK_SAMPLE_COUNT_1_BIT;
  attachment_array[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachment_array[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment_array[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment_array[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment_array[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachment_array[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference color_attachment_reference = {};
  color_attachment_reference.attachment = 0;
  color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_attachment_reference = {};
  depth_attachment_reference.attachment = 1;
  depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass_description = {};
  subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass_description.colorAttachmentCount = 1;
  subpass_description.pColorAttachments = &color_attachment_reference;
  subpass_description.pDepthStencilAttachment = &depth_attachment_reference;

  VkSubpassDependency subpass_dependency{};
  subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  subpass_dependency.dstSubpass = 0;
  subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpass_dependency.srcAccessMask = 0;
  subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo render_pass_create_info =
      { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
  render_pass_create_info.attachmentCount = ARRAY_COUNTOF(attachment_array);
  render_pass_create_info.pAttachments = attachment_array;
  render_pass_create_info.subpassCount = 1;
  render_pass_create_info.pSubpasses = &subpass_description;
  render_pass_create_info.dependencyCount = 1;
  render_pass_create_info.pDependencies = &subpass_dependency;
  const VkResult create_result = vkCreateRenderPass(renderer.GetDevice(), &render_pass_create_info,
                                                    nullptr, &render_pass_);
  RENDERER_CHECK_VK(create_result, "vkCreateRenderPass");

  for (uint32_t i = 0; i < kClearAttachmentCount; ++i) {
    clear_values_[i].color.float32[0] = 0.0f;
    clear_values_[i].color.float32[1] = 0.0f;
    clear_values_[i].color.float32[2] = 0.0f;
    clear_values_[i].color.float32[3] = 0.0f;
    clear_values_[i].depthStencil.depth = 0.0f;
    clear_values_[i].depthStencil.stencil = 0;
  }
  if ((params.attachment_flags & RenderPass::kRenderPassAttachment_Color) != 0) {
    clear_values_[kClearAttachmentColor].color.float32[0] = params.color_clear[0];
    clear_values_[kClearAttachmentColor].color.float32[1] = params.color_clear[1];
    clear_values_[kClearAttachmentColor].color.float32[2] = params.color_clear[2];
    clear_values_[kClearAttachmentColor].color.float32[3] = params.color_clear[3];
  }
  if ((params.attachment_flags &
      (RenderPass::kRenderPassAttachment_Depth | RenderPass::kRenderPassAttachment_Stencil)) != 0) {
    clear_values_[kClearAttachmentDepthStencil].depthStencil.depth = params.depth_clear;
    clear_values_[kClearAttachmentDepthStencil].depthStencil.stencil = params.stencil_clear;
  }
}

RenderPassVk::~RenderPassVk() {
  PurgeFramebufferCache();
  RendererVk &renderer = RendererVk::GetInstanceVk();
  if (render_pass_ != VK_NULL_HANDLE) {
    vkDestroyRenderPass(renderer.GetDevice(), render_pass_, nullptr);
    render_pass_ = VK_NULL_HANDLE;
  }
}

void RenderPassVk::BeginRenderPass() {
  RendererVk &renderer = RendererVk::GetInstanceVk();
  const base_game_framework::SwapchainFrameResourcesVk& swap_resources =
      renderer.GetSwapchainResources();

  // Create framebuffers on demand for a given swapchain image view and cache them
  VkFramebuffer framebuffer = VK_NULL_HANDLE;
  for (const FramebufferCache& cache : framebuffer_cache_) {
    if (cache.swapchain_image_view == swap_resources.swapchain_color_image_view) {
      framebuffer = cache.framebuffer;
      break;
    }
  }

  if (framebuffer == VK_NULL_HANDLE) {
    VkImageView attachments[] = {swap_resources.swapchain_color_image_view,
                                 swap_resources.swapchain_depth_stencil_image_view};

    VkFramebufferCreateInfo framebuffer_create_info{};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass = render_pass_;
    framebuffer_create_info.attachmentCount =
        (swap_resources.swapchain_depth_stencil_image_view != VK_NULL_HANDLE) ? 2 : 1;
    framebuffer_create_info.pAttachments = attachments;
    framebuffer_create_info.width = swap_resources.swapchain_extent.width;
    framebuffer_create_info.height = swap_resources.swapchain_extent.height;
    framebuffer_create_info.layers = 1;

    VkResult create_result = vkCreateFramebuffer(renderer.GetDevice(), &framebuffer_create_info,
                                                 nullptr, &framebuffer);
    RENDERER_CHECK_VK(create_result, "vkCreateFramebuffer");

    framebuffer_cache_.push_back({swap_resources.swapchain_color_image_view, framebuffer});
  }

  VkRenderPassBeginInfo render_pass_begin_info = {};
  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.renderPass = render_pass_;
  render_pass_begin_info.framebuffer = framebuffer;
  render_pass_begin_info.renderArea.offset.x = 0;
  render_pass_begin_info.renderArea.offset.y = 0;
  render_pass_begin_info.renderArea.extent = swap_resources.swapchain_extent;

  uint32_t clear_count = 0;
  VkClearValue* clear_values = nullptr;

  if ((pass_params_.attachment_flags & RenderPass::kRenderPassAttachment_Color) != 0) {
    ++clear_count;
    clear_values = clear_values_;
    if ((pass_params_.attachment_flags &
        (RenderPass::kRenderPassAttachment_Depth |
         RenderPass::kRenderPassAttachment_Stencil)) != 0) {
      ++clear_count;
    }
  } else if ((pass_params_.attachment_flags &
      (RenderPass::kRenderPassAttachment_Depth | RenderPass::kRenderPassAttachment_Stencil)) != 0) {
    // Only depth/stencil, no color
    clear_count = 1;
    clear_values = &clear_values_[kClearAttachmentDepthStencil];
  }
  render_pass_begin_info.clearValueCount = clear_count;
  render_pass_begin_info.pClearValues = clear_values;

  vkCmdBeginRenderPass(renderer.GetRenderCommandBuffer(),
                       &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderPassVk::EndRenderPass() {
  RendererVk &renderer = RendererVk::GetInstanceVk();
  vkCmdEndRenderPass(renderer.GetRenderCommandBuffer());
}

void RenderPassVk::PurgeFramebufferCache() {
  RendererVk &renderer = RendererVk::GetInstanceVk();
  for (const FramebufferCache& cache : framebuffer_cache_) {
    vkDestroyFramebuffer(renderer.GetDevice(), cache.framebuffer, nullptr);
  }
  framebuffer_cache_.clear();
}

}