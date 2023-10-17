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

#include "renderer_texture_vk.h"
#include "renderer_debug.h"
#include "renderer_vk.h"

namespace simple_renderer {

static constexpr VkSamplerMipmapMode kVkMipmapMode[Texture::kMinFilter_Count] = {
    VK_SAMPLER_MIPMAP_MODE_NEAREST,
    VK_SAMPLER_MIPMAP_MODE_LINEAR,
    VK_SAMPLER_MIPMAP_MODE_NEAREST,
    VK_SAMPLER_MIPMAP_MODE_LINEAR,
    VK_SAMPLER_MIPMAP_MODE_NEAREST,
    VK_SAMPLER_MIPMAP_MODE_LINEAR
};

static constexpr VkFilter kVkMinFilters[Texture::kMinFilter_Count] = {
    VK_FILTER_NEAREST,
    VK_FILTER_LINEAR,
    VK_FILTER_NEAREST,
    VK_FILTER_LINEAR,
    VK_FILTER_NEAREST,
    VK_FILTER_LINEAR,
};

static constexpr VkFilter kVkMagFilters[Texture::kMagFilter_Count] = {
    VK_FILTER_NEAREST,
    VK_FILTER_LINEAR
};

static constexpr VkSamplerAddressMode kVkWrapS[Texture::kWrapS_Count] = {
    VK_SAMPLER_ADDRESS_MODE_REPEAT,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT
};

static constexpr VkSamplerAddressMode kVkWrapT[Texture::kWrapT_Count] = {
    VK_SAMPLER_ADDRESS_MODE_REPEAT,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT
};

static VkFormat GetTextureVkFormat(const Texture::TextureCreationParams& params) {
  VkFormat texture_format = VK_FORMAT_R8G8B8A8_UNORM;
  if (params.format == Texture::kTextureFormat_RGB_888) {
    texture_format = VK_FORMAT_R8G8B8_UNORM;
  } else if (params.format == Texture::kTextureFormat_ASTC) {
    switch (params.compression_type) {
      case Texture::kTextureCompression_ASTC_LDR_4x4:
        texture_format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK; break;
      case Texture::kTextureCompression_ASTC_LDR_5x4:
        texture_format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK; break;
      case Texture::kTextureCompression_ASTC_LDR_5x5:
        texture_format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK; break;
      case Texture::kTextureCompression_ASTC_LDR_6x5:
        texture_format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK; break;
      case Texture::kTextureCompression_ASTC_LDR_6x6:
        texture_format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK; break;
      case Texture::kTextureCompression_ASTC_LDR_8x5:
        texture_format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK; break;
      case Texture::kTextureCompression_ASTC_LDR_8x6:
        texture_format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK; break;
      case Texture::kTextureCompression_ASTC_LDR_8x8:
        texture_format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK; break;
      case Texture::kTextureCompression_ASTC_LDR_10x5:
        texture_format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK; break;
      case Texture::kTextureCompression_ASTC_LDR_10x6:
        texture_format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK; break;
      case Texture::kTextureCompression_ASTC_LDR_10x8:
        texture_format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK; break;
      case Texture::kTextureCompression_ASTC_LDR_10x10:
        texture_format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK; break;
      case Texture::kTextureCompression_ASTC_LDR_12x10:
        texture_format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK; break;
      case Texture::kTextureCompression_ASTC_LDR_12x12:
        texture_format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK; break;
      default:
        RENDERER_ASSERT(false)
        break;
    }
  } else if (params.format == Texture::kTextureFormat_ETC2) {
    switch (params.compression_type) {
      case Texture::kTextureCompression_ETC2_RGB8:
        texture_format = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK; break;
      case Texture::kTextureCompression_ETC2_SRGB8:
        texture_format = VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK; break;
      case Texture::kTextureCompression_ETC2_RGB8_PUNCH_ALPHA1:
        texture_format = VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK; break;
      case Texture::kTextureCompression_ETC2_SRGB8_PUNCH_ALPHA1:
        texture_format = VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK; break;
      case Texture::kTextureCompression_ETC2_RGBA8_EAC:
        texture_format = VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK; break;
      case Texture::kTextureCompression_ETC2_SRGB8_ALPHA8_EAC:
        texture_format = VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK; break;
      default:
        RENDERER_ASSERT(false)
        break;
    }
  }
  return texture_format;
}

TextureVk::TextureVk(const Texture::TextureCreationParams& params)
    : Texture(params)
    , image_(VK_NULL_HANDLE)
    , image_view_(VK_NULL_HANDLE)
    , image_alloc_(VK_NULL_HANDLE)
    , sampler_(VK_NULL_HANDLE) {

  RendererVk &renderer = RendererVk::GetInstanceVk();
  VmaAllocator allocator = renderer.GetAllocator();

  const VkFormat texture_format = GetTextureVkFormat(params);

  // Create a staging buffer for the texture data
  VkBufferCreateInfo create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  // TODO: only supports one mip level right now
  create_info.size = params.texture_sizes[0];
  create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  VmaAllocationCreateInfo staging_create_info = {};
  staging_create_info.usage = VMA_MEMORY_USAGE_AUTO;
  staging_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
      VMA_ALLOCATION_CREATE_MAPPED_BIT;
  VkBuffer staging_buffer = VK_NULL_HANDLE;
  VmaAllocation staging_alloc = VK_NULL_HANDLE;
  VmaAllocationInfo staging_info = {};
  const VkResult
      staging_alloc_result = vmaCreateBuffer(allocator, &create_info, &staging_create_info,
                                             &staging_buffer, &staging_alloc, &staging_info);
  RENDERER_CHECK_VK(staging_alloc_result, "vmaCreateBuffer (staging)");
  RENDERER_ASSERT(staging_info.pMappedData != nullptr)
  memcpy(staging_info.pMappedData, params.texture_data, params.texture_sizes[0]);

  VkImageCreateInfo image_create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
  image_create_info.imageType = VK_IMAGE_TYPE_2D;
  image_create_info.extent.width = params.base_width;
  image_create_info.extent.height = params.base_height;
  image_create_info.extent.depth = 1;
  image_create_info.mipLevels = 1;
  image_create_info.arrayLayers = 1;
  image_create_info.format = texture_format;
  image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_create_info.flags = 0;

  VmaAllocationCreateInfo image_alloc_create_info = {};
  image_alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;

  const VkResult image_alloc_result = vmaCreateImage(allocator, &image_create_info,
                                                     &image_alloc_create_info, &image_,
                                                     &image_alloc_, nullptr);
  RENDERER_CHECK_VK(image_alloc_result, "vmaCreateImage");

  VkCommandBuffer command_buffer = renderer.BeginStagingCommandBuffer();

  // Copy and translate the image data from the staging buffer to the final GPU memory and format
  VkImageMemoryBarrier image_memory_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
  image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  image_memory_barrier.subresourceRange.baseMipLevel = 0;
  image_memory_barrier.subresourceRange.levelCount = 1;
  image_memory_barrier.subresourceRange.baseArrayLayer = 0;
  image_memory_barrier.subresourceRange.layerCount = 1;
  image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  image_memory_barrier.image = image_;
  image_memory_barrier.srcAccessMask = 0;
  image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                       1, &image_memory_barrier);

  VkBufferImageCopy copy_region = {};
  copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy_region.imageSubresource.layerCount = 1;
  copy_region.imageExtent.width = params.base_width;
  copy_region.imageExtent.height = params.base_height;
  copy_region.imageExtent.depth = 1;

  vkCmdCopyBufferToImage(command_buffer, staging_buffer, image_,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

  image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image_memory_barrier.image = image_;
  image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                       1, &image_memory_barrier);
  renderer.EndStagingCommandBuffer();
  vmaDestroyBuffer(allocator, staging_buffer, staging_alloc);

  // Create the ImageView for the new Image
  VkImageViewCreateInfo view_create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  view_create_info.image = image_;
  view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format = texture_format;
  view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_create_info.subresourceRange.baseMipLevel = 0;
  view_create_info.subresourceRange.levelCount = 1;
  view_create_info.subresourceRange.baseArrayLayer = 0;
  view_create_info.subresourceRange.layerCount = 1;
  const VkResult create_view_result =  vkCreateImageView(renderer.GetDevice(), &view_create_info,
                                                         nullptr, &image_view_);
  RENDERER_CHECK_VK(create_view_result, "vkCreateImageView");

  VkSamplerCreateInfo sampler_create_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
  sampler_create_info.magFilter = kVkMagFilters[params.mag_filter];
  sampler_create_info.minFilter = kVkMinFilters[params.min_filter];
  sampler_create_info.addressModeU = kVkWrapS[params.wrap_s];
  sampler_create_info.addressModeV = kVkWrapT[params.wrap_t];
  sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_create_info.anisotropyEnable = VK_FALSE;
  sampler_create_info.maxAnisotropy = 1;
  sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_create_info.unnormalizedCoordinates = VK_FALSE;
  sampler_create_info.compareEnable = VK_FALSE;
  sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
  sampler_create_info.mipmapMode = kVkMipmapMode[params.min_filter];
  sampler_create_info.mipLodBias = 0.f;
  sampler_create_info.minLod = 0.f;
  sampler_create_info.maxLod = FLT_MAX;
  const VkResult sampler_result = vkCreateSampler(renderer.GetDevice(), &sampler_create_info,
                                                  nullptr, &sampler_);
  RENDERER_CHECK_VK(sampler_result, "vkCreateSampler");

}

TextureVk::~TextureVk() {
  RendererVk &renderer = RendererVk::GetInstanceVk();
  VmaAllocator allocator = renderer.GetAllocator();

  if (sampler_ != VK_NULL_HANDLE) {
    vkDestroySampler(renderer.GetDevice(), sampler_, nullptr);
    sampler_ = VK_NULL_HANDLE;
  }
  if (image_view_ != VK_NULL_HANDLE) {
    vkDestroyImageView(renderer.GetDevice(), image_view_, nullptr);
    image_view_ = VK_NULL_HANDLE;
  }
  if (image_ != VK_NULL_HANDLE) {
    vmaDestroyImage(allocator, image_, image_alloc_);
    image_ = VK_NULL_HANDLE;
    image_alloc_ = VK_NULL_HANDLE;
  }
}
}