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

// Mipmap mode for each min filter (the min filter itself is in kVkMinFilters). The non-mip
// filters only sample level 0, see the sampler's maxLod.
static constexpr VkSamplerMipmapMode kVkMipmapMode[Texture::kMinFilter_Count] = {
    VK_SAMPLER_MIPMAP_MODE_NEAREST,  // kMinFilter_Nearest
    VK_SAMPLER_MIPMAP_MODE_NEAREST,  // kMinFilter_Linear
    VK_SAMPLER_MIPMAP_MODE_NEAREST,  // kMinFilter_Nearest_Mipmap_Nearest
    VK_SAMPLER_MIPMAP_MODE_NEAREST,  // kMinFilter_Linear_Mipmap_Nearest
    VK_SAMPLER_MIPMAP_MODE_LINEAR,   // kMinFilter_Nearest_Mipmap_Linear
    VK_SAMPLER_MIPMAP_MODE_LINEAR    // kMinFilter_Linear_Mipmap_Linear
};

static constexpr VkFilter kVkMinFilters[Texture::kMinFilter_Count] = {
    VK_FILTER_NEAREST,  // kMinFilter_Nearest
    VK_FILTER_LINEAR,   // kMinFilter_Linear
    VK_FILTER_NEAREST,  // kMinFilter_Nearest_Mipmap_Nearest
    VK_FILTER_LINEAR,   // kMinFilter_Linear_Mipmap_Nearest
    VK_FILTER_NEAREST,  // kMinFilter_Nearest_Mipmap_Linear
    VK_FILTER_LINEAR,   // kMinFilter_Linear_Mipmap_Linear
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

// ASTC formats, indexed from kTextureCompression_ASTC_LDR_4x4
static constexpr VkFormat kVkASTCFormats[] = {
    VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
    VK_FORMAT_ASTC_5x4_UNORM_BLOCK,
    VK_FORMAT_ASTC_5x5_UNORM_BLOCK,
    VK_FORMAT_ASTC_6x5_UNORM_BLOCK,
    VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
    VK_FORMAT_ASTC_8x5_UNORM_BLOCK,
    VK_FORMAT_ASTC_8x6_UNORM_BLOCK,
    VK_FORMAT_ASTC_8x8_UNORM_BLOCK,
    VK_FORMAT_ASTC_10x5_UNORM_BLOCK,
    VK_FORMAT_ASTC_10x6_UNORM_BLOCK,
    VK_FORMAT_ASTC_10x8_UNORM_BLOCK,
    VK_FORMAT_ASTC_10x10_UNORM_BLOCK,
    VK_FORMAT_ASTC_12x10_UNORM_BLOCK,
    VK_FORMAT_ASTC_12x12_UNORM_BLOCK
};

// ETC2 formats, indexed from kTextureCompression_ETC2_RGB8
static constexpr VkFormat kVkETC2Formats[] = {
    VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
    VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
    VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
    VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
    VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
    VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK
};

// Only called with validated parameters (see Texture::ValidateCreationParams)
static VkFormat GetTextureVkFormat(const Texture::TextureCreationParams& params) {
  switch (params.format) {
    case Texture::kTextureFormat_RGBA_8888:
      return VK_FORMAT_R8G8B8A8_UNORM;
    case Texture::kTextureFormat_RGB_888:
      return VK_FORMAT_R8G8B8_UNORM;
    case Texture::kTextureFormat_ASTC:
      return kVkASTCFormats[params.compression_type - Texture::kTextureCompression_ASTC_LDR_4x4];
    case Texture::kTextureFormat_ETC2:
      return kVkETC2Formats[params.compression_type - Texture::kTextureCompression_ETC2_RGB8];
    default:
      return VK_FORMAT_UNDEFINED;
  }
}

static bool IsFormatSampleable(VkPhysicalDevice physical_device, const VkFormat format) {
  VkFormatProperties properties{};
  vkGetPhysicalDeviceFormatProperties(physical_device, format, &properties);
  constexpr VkFormatFeatureFlags kRequired =
      VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
  return (properties.optimalTilingFeatures & kRequired) == kRequired;
}

static constexpr VkImageUsageFlags kTextureImageUsage =
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

// Check the image size and mip count against the device's limits for the format
static bool IsImageSupported(VkPhysicalDevice physical_device, const VkFormat format,
                             const uint32_t width, const uint32_t height,
                             const uint32_t mip_count) {
  VkImageFormatProperties properties{};
  const VkResult result = vkGetPhysicalDeviceImageFormatProperties(
      physical_device, format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, kTextureImageUsage,
      0, &properties);
  if (result != VK_SUCCESS) {
    RENDERER_ERROR("TextureVk: format %d not supported for textures (%d)", format, result);
    return false;
  }
  if (width > properties.maxExtent.width || height > properties.maxExtent.height ||
      mip_count > properties.maxMipLevels) {
    RENDERER_ERROR("TextureVk: %ux%u with %u mips exceeds the format %d limit of %ux%u "
                   "with %u mips", width, height, mip_count, format,
                   properties.maxExtent.width, properties.maxExtent.height,
                   properties.maxMipLevels);
    return false;
  }
  return true;
}

TextureVk::TextureVk(const Texture::TextureCreationParams& params)
    : Texture(params)
    , image_(VK_NULL_HANDLE)
    , image_view_(VK_NULL_HANDLE)
    , image_alloc_(VK_NULL_HANDLE)
    , sampler_(VK_NULL_HANDLE) {
  // The base class validated the parameters; an invalid texture has no mip levels
  const bool valid_params = HasValidCreationParams();
  RENDERER_ASSERT(valid_params);
  if (!valid_params) {
    // Leave the texture empty (null image view and sampler)
    return;
  }
  const uint32_t mip_count = GetMipCount();

  RendererVk &renderer = RendererVk::GetInstanceVk();
  VmaAllocator allocator = renderer.GetAllocator();

  VkFormat texture_format = GetTextureVkFormat(params);
  // R8G8B8 is often not sampleable (e.g. not on PowerVR), store it as RGBA8 instead
  const bool expand_rgb_to_rgba = (texture_format == VK_FORMAT_R8G8B8_UNORM &&
      !IsFormatSampleable(renderer.GetPhysicalDevice(), texture_format));
  if (expand_rgb_to_rgba) {
    texture_format = VK_FORMAT_R8G8B8A8_UNORM;
  }
  // Valid parameters the device can't handle (e.g. a compressed format it doesn't support,
  // or a size over its limits) are reported, and leave the texture empty. Check before
  // staging anything.
  if (!IsFormatSampleable(renderer.GetPhysicalDevice(), texture_format)) {
    RENDERER_ERROR("TextureVk: format %d can't be sampled on this device", texture_format);
    return;
  }
  if (!IsImageSupported(renderer.GetPhysicalDevice(), texture_format, params.base_width,
                        params.base_height, mip_count)) {
    return;
  }

  // Lay out every mip level in the staging buffer, tightly packed. Each level only takes
  // the bytes it needs, which keeps every level offset a multiple of the texel block size.
  const TextureFormat staging_format = expand_rgb_to_rgba ? kTextureFormat_RGBA_8888 :
                                       params.format;
  VkBufferImageCopy copy_regions[kMaxMipCount] = {};
  VkDeviceSize staging_size = 0;
  uint32_t width = params.base_width;
  uint32_t height = params.base_height;
  for (uint32_t level = 0; level < mip_count; ++level) {
    VkBufferImageCopy &region = copy_regions[level];
    region.bufferOffset = staging_size;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = level;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {width, height, 1};
    staging_size += GetRequiredMipSize(staging_format, params.compression_type, width, height);
    width = (width > 1) ? (width >> 1) : 1;
    height = (height > 1) ? (height >> 1) : 1;
  }

  // Create a staging buffer for the texture data
  VkBufferCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  create_info.size = staging_size;
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
  if (!RENDERER_CHECK_VK(staging_alloc_result, "vmaCreateBuffer (staging)")) {
    return;
  }
  RENDERER_ASSERT(staging_info.pMappedData != nullptr);
  if (staging_info.pMappedData == nullptr) {
    vmaDestroyBuffer(allocator, staging_buffer, staging_alloc);
    return;
  }

  // Copy each level's data (texture_sizes[level] bytes apart in the source) to its
  // staging offset, expanding RGB to RGBA if needed
  const uint8_t* source = static_cast<const uint8_t*>(params.texture_data);
  uint8_t* staging = static_cast<uint8_t*>(staging_info.pMappedData);
  for (uint32_t level = 0; level < mip_count; ++level) {
    const VkBufferImageCopy &region = copy_regions[level];
    uint8_t* destination = staging + region.bufferOffset;
    const size_t pixel_count = static_cast<size_t>(region.imageExtent.width) *
                               region.imageExtent.height;
    if (expand_rgb_to_rgba) {
      for (size_t pixel = 0; pixel < pixel_count; ++pixel) {
        destination[pixel * 4 + 0] = source[pixel * 3 + 0];
        destination[pixel * 4 + 1] = source[pixel * 3 + 1];
        destination[pixel * 4 + 2] = source[pixel * 3 + 2];
        destination[pixel * 4 + 3] = 255;
      }
    } else {
      // At most texture_sizes[level] bytes (validated), so it fits in size_t
      memcpy(destination, source,
             static_cast<size_t>(GetRequiredMipSize(params.format, params.compression_type,
                                                    region.imageExtent.width,
                                                    region.imageExtent.height)));
    }
    source += params.texture_sizes[level];
  }

  VkImageCreateInfo image_create_info{};
  image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_create_info.imageType = VK_IMAGE_TYPE_2D;
  image_create_info.extent.width = params.base_width;
  image_create_info.extent.height = params.base_height;
  image_create_info.extent.depth = 1;
  image_create_info.mipLevels = mip_count;
  image_create_info.arrayLayers = 1;
  image_create_info.format = texture_format;
  image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_create_info.usage = kTextureImageUsage;
  image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_create_info.flags = 0;

  VmaAllocationCreateInfo image_alloc_create_info = {};
  image_alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;

  const VkResult image_alloc_result = vmaCreateImage(allocator, &image_create_info,
                                                     &image_alloc_create_info, &image_,
                                                     &image_alloc_, nullptr);
  if (!RENDERER_CHECK_VK(image_alloc_result, "vmaCreateImage")) {
    image_ = VK_NULL_HANDLE;
    image_alloc_ = VK_NULL_HANDLE;
    vmaDestroyBuffer(allocator, staging_buffer, staging_alloc);
    return;
  }

  VkCommandBuffer command_buffer = renderer.BeginStagingCommandBuffer();

  // Copy and translate the image data from the staging buffer to the final GPU memory and
  // format, for all mip levels
  VkImageMemoryBarrier image_memory_barrier{};
  image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  image_memory_barrier.subresourceRange.baseMipLevel = 0;
  image_memory_barrier.subresourceRange.levelCount = mip_count;
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

  vkCmdCopyBufferToImage(command_buffer, staging_buffer, image_,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_count, copy_regions);

  image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                       1, &image_memory_barrier);
  renderer.EndStagingCommandBuffer();
  vmaDestroyBuffer(allocator, staging_buffer, staging_alloc);

  // Create the ImageView for the new Image, covering all mip levels
  VkImageViewCreateInfo view_create_info{};
  view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.image = image_;
  view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format = texture_format;
  view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_create_info.subresourceRange.baseMipLevel = 0;
  view_create_info.subresourceRange.levelCount = mip_count;
  view_create_info.subresourceRange.baseArrayLayer = 0;
  view_create_info.subresourceRange.layerCount = 1;
  const VkResult create_view_result = vkCreateImageView(renderer.GetDevice(), &view_create_info,
                                                        nullptr, &image_view_);
  if (!RENDERER_CHECK_VK(create_view_result, "vkCreateImageView")) {
    // Leave the texture empty rather than holding an image nothing can sample
    image_view_ = VK_NULL_HANDLE;
    vmaDestroyImage(allocator, image_, image_alloc_);
    image_ = VK_NULL_HANDLE;
    image_alloc_ = VK_NULL_HANDLE;
    return;
  }

  // The non-mipmapped min filters (as in GLES) only sample level 0: a maxLod of 0.25
  // keeps the magnification/minification switch while clamping to the base level
  const bool mipmapped_filter = !(params.min_filter == kMinFilter_Nearest ||
                                  params.min_filter == kMinFilter_Linear);
  VkSamplerCreateInfo sampler_create_info{};
  sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
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
  sampler_create_info.maxLod = mipmapped_filter ? VK_LOD_CLAMP_NONE : 0.25f;
  const VkResult sampler_result = vkCreateSampler(renderer.GetDevice(), &sampler_create_info,
                                                  nullptr, &sampler_);
  if (!RENDERER_CHECK_VK(sampler_result, "vkCreateSampler")) {
    // A combined image sampler needs both, so leave the texture empty
    sampler_ = VK_NULL_HANDLE;
    vkDestroyImageView(renderer.GetDevice(), image_view_, nullptr);
    image_view_ = VK_NULL_HANDLE;
    vmaDestroyImage(allocator, image_, image_alloc_);
    image_ = VK_NULL_HANDLE;
    image_alloc_ = VK_NULL_HANDLE;
  }
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