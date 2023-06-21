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

#include "renderer_texture_gles.h"
#include "renderer_debug.h"

namespace simple_renderer {

static constexpr GLenum COMPRESSED_RGBA_ASTC_4x4_KHR = 0x93B0;
static constexpr GLenum COMPRESSED_RGBA_ASTC_5x4_KHR = 0x93B1;
static constexpr GLenum COMPRESSED_RGBA_ASTC_5x5_KHR = 0x93B2;
static constexpr GLenum COMPRESSED_RGBA_ASTC_6x5_KHR = 0x93B3;
static constexpr GLenum COMPRESSED_RGBA_ASTC_6x6_KHR = 0x93B4;
static constexpr GLenum COMPRESSED_RGBA_ASTC_8x5_KHR = 0x93B5;
static constexpr GLenum COMPRESSED_RGBA_ASTC_8x6_KHR = 0x93B6;
static constexpr GLenum COMPRESSED_RGBA_ASTC_8x8_KHR = 0x93B7;
static constexpr GLenum COMPRESSED_RGBA_ASTC_10x5_KHR = 0x93B8;
static constexpr GLenum COMPRESSED_RGBA_ASTC_10x6_KHR = 0x93B9;
static constexpr GLenum COMPRESSED_RGBA_ASTC_10x8_KHR = 0x93BA;
static constexpr GLenum COMPRESSED_RGBA_ASTC_10x10_KHR = 0x93BB;
static constexpr GLenum COMPRESSED_RGBA_ASTC_12x10_KHR = 0x93BC;
static constexpr GLenum COMPRESSED_RGBA_ASTC_12x12_KHR = 0x93BD;

static constexpr GLenum kGLCompressedFormats[Texture::kTextureCompression_Count] = {
    GL_NONE,
    COMPRESSED_RGBA_ASTC_4x4_KHR,
    COMPRESSED_RGBA_ASTC_5x4_KHR,
    COMPRESSED_RGBA_ASTC_5x5_KHR,
    COMPRESSED_RGBA_ASTC_6x5_KHR,
    COMPRESSED_RGBA_ASTC_6x6_KHR,
    COMPRESSED_RGBA_ASTC_8x5_KHR,
    COMPRESSED_RGBA_ASTC_8x6_KHR,
    COMPRESSED_RGBA_ASTC_8x8_KHR,
    COMPRESSED_RGBA_ASTC_10x5_KHR,
    COMPRESSED_RGBA_ASTC_10x6_KHR,
    COMPRESSED_RGBA_ASTC_10x8_KHR,
    COMPRESSED_RGBA_ASTC_10x10_KHR,
    COMPRESSED_RGBA_ASTC_12x10_KHR,
    COMPRESSED_RGBA_ASTC_12x12_KHR,
    GL_COMPRESSED_RGB8_ETC2,
    GL_COMPRESSED_SRGB8_ETC2,
    GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,
    GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,
    GL_COMPRESSED_RGBA8_ETC2_EAC,
    GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC
};

static constexpr GLint kGLMinFilters[Texture::kMinFilter_Count] = {
    GL_NEAREST,
    GL_LINEAR,
    GL_NEAREST_MIPMAP_NEAREST,
    GL_LINEAR_MIPMAP_NEAREST,
    GL_NEAREST_MIPMAP_LINEAR,
    GL_LINEAR_MIPMAP_LINEAR
};

static constexpr GLint kGLMagFilters[Texture::kMagFilter_Count] = {
    GL_NEAREST,
    GL_LINEAR
};

static constexpr GLint kGLWrapS[Texture::kWrapS_Count] = {
    GL_REPEAT,
    GL_CLAMP_TO_EDGE,
    GL_MIRRORED_REPEAT
};

static constexpr GLint kGLWrapT[Texture::kWrapT_Count] = {
    GL_REPEAT,
    GL_CLAMP_TO_EDGE,
    GL_MIRRORED_REPEAT
};

TextureGLES::TextureGLES(const Texture::TextureCreationParams& params)
                         : Texture(params) {
  texture_object_ = 0;

  glGenTextures(1, &texture_object_);
  RENDERER_CHECK_GLES("glGenTextures");
  glBindTexture(GL_TEXTURE_2D, texture_object_);
  RENDERER_CHECK_GLES("glBindTexture");

  GLsizei width = params.base_width;
  GLsizei height = params.base_height;
  uint32_t current_mip_level = 0;
  const uint8_t* data = static_cast<const uint8_t *>(params.texture_data);
  const GLenum format = (params.format == kTextureFormat_RGBA_8888) ? GL_RGBA : GL_RGB;

  if (params.format == kTextureFormat_RGBA_8888 || params.format == kTextureFormat_RGB_888) {
    // Uncompressed
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    RENDERER_CHECK_GLES("glPixelStorei");
  }
  while (current_mip_level < params.mip_count) {
    if (params.format == kTextureFormat_RGBA_8888 || params.format == kTextureFormat_RGB_888) {
      glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                    GL_UNSIGNED_BYTE, data);
      RENDERER_CHECK_GLES("glTexImage2D");
    } else {
      glCompressedTexImage2D(GL_TEXTURE_2D, 0,
                             kGLCompressedFormats[params.compression_type],
                             width, height,0,
                             params.texture_sizes[current_mip_level], data);
      RENDERER_CHECK_GLES("glCompressedTexImage2D");
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, kGLMinFilters[params.min_filter]);
    RENDERER_CHECK_GLES("glTexParameteri");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, kGLMagFilters[params.mag_filter]);
    RENDERER_CHECK_GLES("glTexParameteri");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, kGLWrapS[params.wrap_s]);
    RENDERER_CHECK_GLES("glTexParameteri");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, kGLWrapT[params.wrap_t]);
    RENDERER_CHECK_GLES("glTexParameteri");

    data += params.texture_sizes[current_mip_level];
    width >>= 1;
    height >>= 1;
    ++current_mip_level;
  }
  glBindTexture(GL_TEXTURE_2D, 0);
  RENDERER_CHECK_GLES("glBindTexture");
}

TextureGLES::~TextureGLES() {
  glBindTexture(GL_TEXTURE_2D, 0);
  RENDERER_CHECK_GLES("glBindTexture");
  glDeleteTextures(1, &texture_object_);
  RENDERER_CHECK_GLES("glDeleteTextures");
  texture_object_ = 0;
}
}