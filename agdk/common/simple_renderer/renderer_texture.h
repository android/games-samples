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

#ifndef SIMPLERENDERER_TEXTURE_H_
#define SIMPLERENDERER_TEXTURE_H_

#include <cstdint>
#include <string>

namespace simple_renderer
{
/**
 * @brief The base class definition for the `Texture` class of SimpleRenderer.
 * Use the `Renderer` class interface to create and destroy `Texture` objects.
 * `Texture` does not currently support dynamically updating texture data after
 * initial creation.
 */
class Texture {
 public:

  /**
   * @brief The texture formats supported by `Texture`
   * For compressed textures, the compression parameters are defined by the
   * `TextureCompressionType` enum
   */
  enum TextureFormat : uint32_t {
    /** @brief Uncompressed 32 bit RGBA texture, 8 bits per channel */
    kTextureFormat_RGBA_8888 = 0,
    /** @brief Uncompressed 24 bit RGB texture, 8 bits per channel */
    kTextureFormat_RGB_888,
    /** @brief Compressed ETC2 texture */
    kTextureFormat_ETC2,
    /** @brief Compressed ASTC texture */
    kTextureFormat_ASTC,
    /** @brief Count of texture formats */
    kTextureFormat_Count
  };

  /**
   * @brief The compression type of a compressed texture
   */
  enum TextureCompressionType : uint32_t {
    /** @brief No compression on texture */
    kTextureCompression_None = 0,
    /** @brief ASTC LDR 4x4 block compression */
    kTextureCompression_ASTC_LDR_4x4,
    /** @brief ASTC LDR 5x4 block compression */
    kTextureCompression_ASTC_LDR_5x4,
    /** @brief ASTC LDR 5x5 block compression */
    kTextureCompression_ASTC_LDR_5x5,
    /** @brief ASTC LDR 6x5 block compression */
    kTextureCompression_ASTC_LDR_6x5,
    /** @brief ASTC LDR 6x6 block compression */
    kTextureCompression_ASTC_LDR_6x6,
    /** @brief ASTC LDR 8x5 block compression */
    kTextureCompression_ASTC_LDR_8x5,
    /** @brief ASTC LDR 8x6 block compression */
    kTextureCompression_ASTC_LDR_8x6,
    /** @brief ASTC LDR 8x8 block compression */
    kTextureCompression_ASTC_LDR_8x8,
    /** @brief ASTC LDR 10x5 block compression */
    kTextureCompression_ASTC_LDR_10x5,
    /** @brief ASTC LDR 10x6 block compression */
    kTextureCompression_ASTC_LDR_10x6,
    /** @brief ASTC LDR 10x8 block compression */
    kTextureCompression_ASTC_LDR_10x8,
    /** @brief ASTC LDR 10x10 block compression */
    kTextureCompression_ASTC_LDR_10x10,
    /** @brief ASTC LDR 12x10 block compression */
    kTextureCompression_ASTC_LDR_12x10,
    /** @brief ASTC LDR 12x12 block compression */
    kTextureCompression_ASTC_LDR_12x12,
    /** @brief ETC2 RGB888 */
    kTextureCompression_ETC2_RGB8,
    /** @brief ETC2 RGB888 (SRGB) */
    kTextureCompression_ETC2_SRGB8,
    /** @brief ETC2 RGB888 1 bit alpha*/
    kTextureCompression_ETC2_RGB8_PUNCH_ALPHA1,
    /** @brief ETC2 RGB888 1 bit alpha (SRGB) */
    kTextureCompression_ETC2_SRGB8_PUNCH_ALPHA1,
    /** @brief ETC2 RGBA888 */
    kTextureCompression_ETC2_RGBA8_EAC,
    /** @brief ETC2 RGBA888 (SRGB) */
    kTextureCompression_ETC2_SRGB8_ALPHA8_EAC,
    /** @brief Count of texture compression types */
    kTextureCompression_Count
  };

  /**
   * @brief The minified filters applicable to a texture
   */
  enum TextureMinFilter : uint32_t {
    /** @brief Nearest texel to coordinates */
    kMinFilter_Nearest = 0,
    /** @brief Linear 2x2 average to coordinates */
    kMinFilter_Linear,
    /** @brief Nearest texel to coordinates of closest mipmap */
    kMinFilter_Nearest_Mipmap_Nearest,
    /** @brief Linear 2x2 average to coordinates of closest mipmap */
    kMinFilter_Linear_Mipmap_Nearest,
    /** @brief Nearest texel to coordinates of closest two mipmaps */
    kMinFilter_Nearest_Mipmap_Linear,
    /** @brief Linear 2x2 average to coordinates of closest two mipmaps */
    kMinFilter_Linear_Mipmap_Linear,
    /** @brief Count of min filters */
    kMinFilter_Count
  };

  /**
   * @brief The magnified filters applicable to a texture
   */
  enum TextureMagFilter : uint32_t {
    /** @brief Nearest texel to coordinates */
    kMagFilter_Nearest = 0,
    /** @brief Linear 2x2 average to coordinates */
    kMagFilter_Linear,
    /** @brief Count of mag filters */
    kMagFilter_Count
  };

  /**
   * @brief The S coordinate wrap options available for a texture
   */
  enum TextureWrapS : uint32_t {
    /** @brief Repeat texture (wrap) */
    kWrapS_Repeat = 0,
    /** @brief Clamp coordinate to edge of texture */
    kWrapS_ClampToEdge,
    /** @brief Repeat texture (mirrored) */
    kWrapS_MirroredRepeat,
    /** @brief Count of S wraps */
    kWrapS_Count
  };

  /**
   * @brief The T coordinate wrap options available for a texture
   */
  enum TextureWrapT : uint32_t {
    /** @brief Repeat texture (wrap) */
    kWrapT_Repeat = 0,
    /** @brief Clamp coordinate to edge of texture */
    kWrapT_ClampToEdge,
    /** @brief Repeat texture (mirrored) */
    kWrapT_MirroredRepeat,
    /** @brief Count of T wraps */
    kWrapT_Count
  };

  /**
   * @brief A structure holding required parameters to create a new `Texture`.
   * Passed to the Renderer::CreateTextureBuffer function.
   */
  struct TextureCreationParams {
    /** @brief Pixel format of the texture */
    Texture::TextureFormat format;
    /** @brief Compression format of the texture */
    Texture::TextureCompressionType compression_type;
    /** @brief Minified function of the texture */
    Texture::TextureMinFilter min_filter;
    /** @brief Magnified function of the texture */
    Texture::TextureMagFilter mag_filter;
    /** @brief Coordinate S wrap mode of the texture */
    Texture::TextureWrapS wrap_s;
    /** @brief Coordinate T wrap mode of the texture */
    Texture::TextureWrapT wrap_t;
    /** @brief Base width of the texture in pixels at mip level 0 */
    uint32_t base_width;
    /** @brief Base height of the texture in pixels at mip level 0 */
    uint32_t base_height;
    /** @brief Number of mip levels */
    uint32_t mip_count;
    /** @brief Pointer to an array with `mip_count` entries of byte sizes per mip level */
    const uint32_t* texture_sizes;
    /** @brief Pointer to an array of pixel data for all mip levels of the texture */
    const void* texture_data;
  };

  /**
   * @brief Retrieve the debug name string associated with the `Texture`
   * @result A string containing the debug name.
   */
  const std::string& GetTextureDebugName() const { return texture_debug_name_; }
  /**
   * @brief Set a debug name string to associate with the `Texture`
   * @param name A string containing the debug name.
   */
  void SetTextureDebugName(std::string name) { texture_debug_name_ = name; }

  /**
   * @brief Get the pixel format used by the `Texture`.
   * @return The pixel format used by the `Texture`
   */
  TextureFormat GetTextureFormat() const { return texture_format_; }

  /**
   * @brief Get the compression format used by the `Texture`.
   * @return The compression format used by the `Texture`
   */
  TextureCompressionType GetTextureCompressionType() const { return texture_compression_type_; }

  /**
   * @brief Get the mip 0 pixel width of the `Texture`.
   * @return The mip 0 pixel width of the `Texture`
   */
  uint32_t GetTextureWidth() const { return texture_base_width_; }

  /**
   * @brief Get the mip 0 pixel height of the `Texture`.
   * @return The mip 0 pixel height of the `Texture`
   */
  uint32_t GetTextureHeight() const { return texture_base_height_; }

  /**
   * @brief Get the mipmap count of the `Texture`.
   * @return The number of mipmaps in the `Texture`
   */
  uint32_t GetMipCount() const { return texture_mip_count_; }

  /**
   * @brief Get the size of the texture data in bytes of the `Texture` at the specified mip level
   * @return The size of the texture data in bytes of the `Texture` at the specified mip level
   */
  size_t GetTextureSize(const uint32_t mip_level) const {
    if (mip_level < texture_mip_count_) {
      return texture_sizes_[mip_level];
    }
    return 0;
  }

  /**
   * @brief Base class destructor, do not call directly.
   */
  virtual ~Texture() {}

 protected:
  Texture(const TextureCreationParams& params)
      : texture_format_(params.format),
        texture_compression_type_(params.compression_type),
        texture_base_width_(params.base_width),
        texture_base_height_(params.base_height),
        texture_debug_name_("noname)") {
    texture_mip_count_ = params.mip_count;
    if (params.mip_count > kMaxMipCount) {
      texture_mip_count_ = kMaxMipCount;
    }
    uint32_t i = 0;
    while (i < texture_mip_count_) {
      texture_sizes_[i] = params.texture_sizes[i];
      ++i;
    }
    while (i < kMaxMipCount) {
      texture_sizes_[i] = 0;
      ++i;
    }
  }

 private:
  Texture()
  : texture_format_(kTextureFormat_Count),
    texture_compression_type_(kTextureCompression_Count),
    texture_mip_count_(0) {
      for (uint32_t i = 0; i < kMaxMipCount; ++i) {
        texture_sizes_[i] = 0;
      }
  }

  static constexpr uint32_t kMaxMipCount = 16;
  TextureFormat texture_format_;
  TextureCompressionType texture_compression_type_;
  uint32_t texture_base_width_;
  uint32_t texture_base_height_;
  uint32_t texture_mip_count_;
  size_t texture_sizes_[kMaxMipCount];

  std::string texture_debug_name_;
};
} // namespace simple_renderer

#endif // SIMPLERENDERER_TEXTURE_H_
