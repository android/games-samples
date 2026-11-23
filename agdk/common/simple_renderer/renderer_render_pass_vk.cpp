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


namespace simple_renderer {

// Load/store operations, indexed by the RenderPass load/store enums
static constexpr VkAttachmentLoadOp kColorLoadOps[] = {
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,  // kRenderPassColorLoad_DontCare
    VK_ATTACHMENT_LOAD_OP_CLEAR       // kRenderPassColorLoad_Clear
};
static constexpr VkAttachmentStoreOp kColorStoreOps[] = {
    VK_ATTACHMENT_STORE_OP_DONT_CARE,  // kRenderPassColorStore_DontCare
    VK_ATTACHMENT_STORE_OP_STORE       // kRenderPassColorStore_Write
};
static constexpr VkAttachmentLoadOp kDepthLoadOps[] = {
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,  // kRenderPassDepthLoad_DontCare
    VK_ATTACHMENT_LOAD_OP_CLEAR       // kRenderPassDepthLoad_Clear
};
static constexpr VkAttachmentStoreOp kDepthStoreOps[] = {
    VK_ATTACHMENT_STORE_OP_DONT_CARE,  // kRenderPassDepthStore_DontCare
    VK_ATTACHMENT_STORE_OP_STORE       // kRenderPassDepthStore_Write
};
static constexpr VkAttachmentLoadOp kStencilLoadOps[] = {
    VK_ATTACHMENT_LOAD_OP_DONT_CARE,  // kRenderPassStencilLoad_DontCare
    VK_ATTACHMENT_LOAD_OP_CLEAR       // kRenderPassStencilLoad_Clear
};
static constexpr VkAttachmentStoreOp kStencilStoreOps[] = {
    VK_ATTACHMENT_STORE_OP_DONT_CARE,  // kRenderPassStencilStore_DontCare
    VK_ATTACHMENT_STORE_OP_STORE       // kRenderPassStencilStore_Write
};

template <typename T, size_t N>
static T LookupOp(const T (&ops)[N], const uint32_t index) {
  RENDERER_ASSERT(index < N);
  return (index < N) ? ops[index] : ops[0];
}

RenderPassVk::RenderPassVk(const RenderPassCreationParams& params)
    : RenderPass()
    , render_pass_(VK_NULL_HANDLE)
    , has_depth_stencil_attachment_(false)
    , pass_active_(false)
    , framebuffer_cache_() {
  pass_params_ = params;
  RendererVk &renderer = RendererVk::GetInstanceVk();

  // Render passes render to the swapchain: attachment 0 is always its color image. The
  // depth/stencil attachment is only declared if the pass asks for depth or stencil and the
  // swapchain has a depth/stencil image (it has none if its format is VK_FORMAT_UNDEFINED).
  const VkFormat depth_stencil_format = renderer.GetSwapchainDepthStencilFormat();
  has_depth_stencil_attachment_ =
      (depth_stencil_format != VK_FORMAT_UNDEFINED) &&
      ((params.attachment_flags & (RenderPass::kRenderPassAttachment_Depth |
                                   RenderPass::kRenderPassAttachment_Stencil)) != 0);

  // Load/store operations only apply to the aspects the pass has (attachment_flags); the
  // others are don't care and their parameters aren't read. This matches RenderPassGLES.
  const bool use_color = (params.attachment_flags & RenderPass::kRenderPassAttachment_Color) != 0;
  const bool use_depth = (params.attachment_flags & RenderPass::kRenderPassAttachment_Depth) != 0;
  const bool use_stencil =
      (params.attachment_flags & RenderPass::kRenderPassAttachment_Stencil) != 0;

  VkAttachmentDescription attachment_array[kClearAttachmentCount] = {};

  // The color image's previous contents are never loaded (there is no load operation),
  // so it starts in an undefined layout
  attachment_array[kClearAttachmentColor].format = renderer.GetSwapchainColorFormat();
  attachment_array[kClearAttachmentColor].samples = VK_SAMPLE_COUNT_1_BIT;
  // Without the color flag the color image (which is always there, it's the swapchain image)
  // isn't cleared, but what is drawn is still stored
  attachment_array[kClearAttachmentColor].loadOp =
      use_color ? LookupOp(kColorLoadOps, params.color_load) : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment_array[kClearAttachmentColor].storeOp =
      use_color ? LookupOp(kColorStoreOps, params.color_store) : VK_ATTACHMENT_STORE_OP_STORE;
  attachment_array[kClearAttachmentColor].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment_array[kClearAttachmentColor].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment_array[kClearAttachmentColor].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachment_array[kClearAttachmentColor].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  attachment_array[kClearAttachmentDepthStencil].format = depth_stencil_format;
  attachment_array[kClearAttachmentDepthStencil].samples = VK_SAMPLE_COUNT_1_BIT;
  attachment_array[kClearAttachmentDepthStencil].loadOp =
      use_depth ? LookupOp(kDepthLoadOps, params.depth_load) : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment_array[kClearAttachmentDepthStencil].storeOp =
      use_depth ? LookupOp(kDepthStoreOps, params.depth_store) :
                  VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment_array[kClearAttachmentDepthStencil].stencilLoadOp =
      use_stencil ? LookupOp(kStencilLoadOps, params.stencil_load) :
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment_array[kClearAttachmentDepthStencil].stencilStoreOp =
      use_stencil ? LookupOp(kStencilStoreOps, params.stencil_store) :
                    VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment_array[kClearAttachmentDepthStencil].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachment_array[kClearAttachmentDepthStencil].finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference color_attachment_reference = {};
  color_attachment_reference.attachment = kClearAttachmentColor;
  color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_attachment_reference = {};
  depth_attachment_reference.attachment = kClearAttachmentDepthStencil;
  depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass_description = {};
  subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass_description.colorAttachmentCount = 1;
  subpass_description.pColorAttachments = &color_attachment_reference;
  subpass_description.pDepthStencilAttachment =
      has_depth_stencil_attachment_ ? &depth_attachment_reference : nullptr;

  // Wait for the swapchain image (acquired at color attachment output) and any earlier color
  // writes (e.g. a previous pass this frame) before the color load, reads (blending) and
  // writes. With a depth/stencil attachment, also order its load (clear) after earlier
  // depth/stencil writes to the same image.
  VkSubpassDependency subpass_dependency{};
  subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  subpass_dependency.dstSubpass = 0;
  subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpass_dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpass_dependency.dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  if (has_depth_stencil_attachment_) {
    constexpr VkPipelineStageFlags kFragmentTests =
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    subpass_dependency.srcStageMask |= kFragmentTests;
    subpass_dependency.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    subpass_dependency.dstStageMask |= kFragmentTests;
    subpass_dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  }

  VkRenderPassCreateInfo render_pass_create_info{};
  render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_create_info.attachmentCount = GetAttachmentCount();
  render_pass_create_info.pAttachments = attachment_array;
  render_pass_create_info.subpassCount = 1;
  render_pass_create_info.pSubpasses = &subpass_description;
  render_pass_create_info.dependencyCount = 1;
  render_pass_create_info.pDependencies = &subpass_dependency;
  const VkResult create_result = vkCreateRenderPass(renderer.GetDevice(), &render_pass_create_info,
                                                    nullptr, &render_pass_);
  if (!RENDERER_CHECK_VK(create_result, "vkCreateRenderPass")) {
    render_pass_ = VK_NULL_HANDLE;
  }

  // Clear values are indexed by attachment; unused ones are ignored
  clear_values_[kClearAttachmentColor] = {};
  clear_values_[kClearAttachmentColor].color.float32[0] = params.color_clear[0];
  clear_values_[kClearAttachmentColor].color.float32[1] = params.color_clear[1];
  clear_values_[kClearAttachmentColor].color.float32[2] = params.color_clear[2];
  clear_values_[kClearAttachmentColor].color.float32[3] = params.color_clear[3];
  clear_values_[kClearAttachmentDepthStencil] = {};
  clear_values_[kClearAttachmentDepthStencil].depthStencil.depth = params.depth_clear;
  clear_values_[kClearAttachmentDepthStencil].depthStencil.stencil =
      static_cast<uint32_t>(params.stencil_clear);
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
  pass_active_ = false;
  if (render_pass_ == VK_NULL_HANDLE) {
    return;
  }

  // Create framebuffers on demand for a given swapchain image view and cache them
  VkFramebuffer framebuffer = VK_NULL_HANDLE;
  for (const FramebufferCache& cache : framebuffer_cache_) {
    if (cache.swapchain_image_view == swap_resources.swapchain_color_image_view) {
      framebuffer = cache.framebuffer;
      break;
    }
  }

  if (framebuffer == VK_NULL_HANDLE) {
    VkImageView attachments[kClearAttachmentCount] = {
        swap_resources.swapchain_color_image_view,
        swap_resources.swapchain_depth_stencil_image_view};
    // The framebuffer must have the render pass's attachments
    const bool have_views = swap_resources.swapchain_color_image_view != VK_NULL_HANDLE &&
        (!has_depth_stencil_attachment_ ||
         swap_resources.swapchain_depth_stencil_image_view != VK_NULL_HANDLE);
    RENDERER_ASSERT(have_views);
    if (!have_views) {
      return;
    }
    VkFramebufferCreateInfo framebuffer_create_info{};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass = render_pass_;
    framebuffer_create_info.attachmentCount = GetAttachmentCount();
    framebuffer_create_info.pAttachments = attachments;
    framebuffer_create_info.width = swap_resources.swapchain_extent.width;
    framebuffer_create_info.height = swap_resources.swapchain_extent.height;
    framebuffer_create_info.layers = 1;

    VkResult create_result = vkCreateFramebuffer(renderer.GetDevice(), &framebuffer_create_info,
                                                 nullptr, &framebuffer);
    if (!RENDERER_CHECK_VK(create_result, "vkCreateFramebuffer")) {
      // Don't cache or begin with a failed framebuffer
      return;
    }

    framebuffer_cache_.push_back({swap_resources.swapchain_color_image_view, framebuffer});
  }

  VkRenderPassBeginInfo render_pass_begin_info = {};
  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.renderPass = render_pass_;
  render_pass_begin_info.framebuffer = framebuffer;
  render_pass_begin_info.renderArea.offset.x = 0;
  render_pass_begin_info.renderArea.offset.y = 0;
  render_pass_begin_info.renderArea.extent = swap_resources.swapchain_extent;

  // One clear value per attachment (only used by attachments that clear)
  render_pass_begin_info.clearValueCount = GetAttachmentCount();
  render_pass_begin_info.pClearValues = clear_values_;

  vkCmdBeginRenderPass(renderer.GetRenderCommandBuffer(),
                       &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
  pass_active_ = true;
}

void RenderPassVk::EndRenderPass() {
  // Only end a render pass that BeginRenderPass began
  if (!pass_active_) {
    return;
  }
  RendererVk &renderer = RendererVk::GetInstanceVk();
  vkCmdEndRenderPass(renderer.GetRenderCommandBuffer());
  pass_active_ = false;
}

void RenderPassVk::PurgeFramebufferCache() {
  RendererVk &renderer = RendererVk::GetInstanceVk();
  for (const FramebufferCache& cache : framebuffer_cache_) {
    vkDestroyFramebuffer(renderer.GetDevice(), cache.framebuffer, nullptr);
  }
  framebuffer_cache_.clear();
}

}