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

#include "renderer_texture.h"
#include <algorithm>
#include "renderer_debug.h"

namespace simple_renderer {

// ASTC block dimensions, indexed from kTextureCompression_ASTC_LDR_4x4
static constexpr uint32_t kASTCBlockSizes[][2] = {
    {4, 4}, {5, 4}, {5, 5}, {6, 5}, {6, 6}, {8, 5}, {8, 6}, {8, 8},
    {10, 5}, {10, 6}, {10, 8}, {10, 10}, {12, 10}, {12, 12}
};

static bool IsASTC(const Texture::TextureCompressionType compression) {
  return compression >= Texture::kTextureCompression_ASTC_LDR_4x4 &&
         compression <= Texture::kTextureCompression_ASTC_LDR_12x12;
}

static bool IsETC2(const Texture::TextureCompressionType compression) {
  return compression >= Texture::kTextureCompression_ETC2_RGB8 &&
         compression <= Texture::kTextureCompression_ETC2_SRGB8_ALPHA8_EAC;
}

// Sizes are computed in 64 bits: width + block - 1 can overflow 32 bits, and so can the
// byte count (size_t is 32 bits on 32-bit ABIs)
uint64_t Texture::GetRequiredMipSize(const TextureFormat format,
                                     const TextureCompressionType compression,
                                     const uint32_t width, const uint32_t height) {
  const uint64_t w = width;
  const uint64_t h = height;
  switch (format) {
    case kTextureFormat_RGBA_8888:
      return w * h * 4;
    case kTextureFormat_RGB_888:
      return w * h * 3;
    case kTextureFormat_ASTC: {
      if (!IsASTC(compression)) {
        return 0;
      }
      // Every ASTC block is 16 bytes, partial blocks at the edges are whole blocks
      const uint32_t* block = kASTCBlockSizes[compression - kTextureCompression_ASTC_LDR_4x4];
      return ((w + block[0] - 1) / block[0]) * ((h + block[1] - 1) / block[1]) * 16;
    }
    case kTextureFormat_ETC2: {
      if (!IsETC2(compression)) {
        return 0;
      }
      // 4x4 blocks: 16 bytes with EAC alpha, 8 bytes otherwise
      const uint64_t block_bytes =
          (compression == kTextureCompression_ETC2_RGBA8_EAC ||
           compression == kTextureCompression_ETC2_SRGB8_ALPHA8_EAC) ? 16 : 8;
      return ((w + 3) / 4) * ((h + 3) / 4) * block_bytes;
    }
    default:
      return 0;
  }
}

bool Texture::ValidateCreationParams(const TextureCreationParams& params) {
  if (params.format >= kTextureFormat_Count ||
      params.compression_type >= kTextureCompression_Count ||
      params.min_filter >= kMinFilter_Count || params.mag_filter >= kMagFilter_Count ||
      params.wrap_s >= kWrapS_Count || params.wrap_t >= kWrapT_Count) {
    RENDERER_ERROR("Texture: invalid format/compression/filter/wrap enum");
    return false;
  }
  if (params.base_width == 0 || params.base_height == 0 || params.mip_count == 0 ||
      params.texture_data == nullptr || params.texture_sizes == nullptr) {
    RENDERER_ERROR("Texture: missing size or data (%ux%u, %u mips)", params.base_width,
                   params.base_height, params.mip_count);
    return false;
  }
  if (params.mip_count > kMaxMipCount) {
    RENDERER_ERROR("Texture: %u mips, the maximum is %u", params.mip_count, kMaxMipCount);
    return false;
  }
  if ((params.format == kTextureFormat_RGBA_8888 || params.format == kTextureFormat_RGB_888) &&
      params.compression_type != kTextureCompression_None) {
    RENDERER_ERROR("Texture: uncompressed texture with compression %u",
                   params.compression_type);
    return false;
  }
  if (params.format == kTextureFormat_ASTC && !IsASTC(params.compression_type)) {
    RENDERER_ERROR("Texture: ASTC texture with non-ASTC compression %u",
                   params.compression_type);
    return false;
  }
  if (params.format == kTextureFormat_ETC2 && !IsETC2(params.compression_type)) {
    RENDERER_ERROR("Texture: ETC2 texture with non-ETC2 compression %u",
                   params.compression_type);
    return false;
  }
  // A full mip chain ends at 1x1
  uint32_t full_chain = 1;
  for (uint32_t size = std::max(params.base_width, params.base_height); size > 1; size >>= 1) {
    ++full_chain;
  }
  if (params.mip_count > full_chain) {
    RENDERER_ERROR("Texture: %u mips for %ux%u, a full chain is %u", params.mip_count,
                   params.base_width, params.base_height, full_chain);
    return false;
  }
  // Uploads read the required byte count for each level, whatever size is passed in, so
  // make sure that much data is there
  uint32_t width = params.base_width;
  uint32_t height = params.base_height;
  for (uint32_t level = 0; level < params.mip_count; ++level) {
    const uint64_t required = GetRequiredMipSize(params.format, params.compression_type,
                                                 width, height);
    if (params.texture_sizes[level] < required) {
      RENDERER_ERROR("Texture: mip %u has %u bytes, needs %llu", level,
                     params.texture_sizes[level], static_cast<unsigned long long>(required));
      return false;
    }
    width = (width > 1) ? (width >> 1) : 1;
    height = (height > 1) ? (height >> 1) : 1;
  }
  return true;
}

} // namespace simple_renderer
