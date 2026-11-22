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
  // The base class validated the parameters; an invalid texture has no mip levels
  const bool valid_params = HasValidCreationParams();
  RENDERER_ASSERT(valid_params);
  if (!valid_params) {
    // Leave texture_object_ as 0; binding it unbinds any texture
    return;
  }
  const uint32_t mip_count = GetMipCount();

  // Don't disturb the caller's texture binding or pixel unpack state
  GLint previous_texture = 0;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);
  GLint previous_unpack_buffer = 0;
  glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &previous_unpack_buffer);
  GLint previous_unpack_alignment = 4;
  GLint previous_unpack_row_length = 0;
  GLint previous_unpack_skip_pixels = 0;
  GLint previous_unpack_skip_rows = 0;
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &previous_unpack_alignment);
  glGetIntegerv(GL_UNPACK_ROW_LENGTH, &previous_unpack_row_length);
  glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &previous_unpack_skip_pixels);
  glGetIntegerv(GL_UNPACK_SKIP_ROWS, &previous_unpack_skip_rows);

  // Upload from client memory (not a pixel unpack buffer), with tightly packed rows
  // (e.g. a 3 pixel wide RGB row is 9 bytes, not padded to 12)
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  RENDERER_CHECK_GLES("pixel unpack state");

  glGenTextures(1, &texture_object_);
  RENDERER_CHECK_GLES("glGenTextures");
  glBindTexture(GL_TEXTURE_2D, texture_object_);
  RENDERER_CHECK_GLES("glBindTexture");

  GLsizei width = params.base_width;
  GLsizei height = params.base_height;
  const uint8_t* data = static_cast<const uint8_t *>(params.texture_data);
  const bool uncompressed = (params.format == kTextureFormat_RGBA_8888 ||
                             params.format == kTextureFormat_RGB_888);
  const GLenum format = (params.format == kTextureFormat_RGBA_8888) ? GL_RGBA : GL_RGB;
  // Sized internal formats guarantee 8 bit channels (and GL_RGB8 is color-renderable)
  const GLint internal_format = (params.format == kTextureFormat_RGBA_8888) ? GL_RGBA8 : GL_RGB8;

  for (uint32_t mip_level = 0; mip_level < mip_count; ++mip_level) {
    const GLint level = static_cast<GLint>(mip_level);
    if (uncompressed) {
      glTexImage2D(GL_TEXTURE_2D, level, internal_format, width, height, 0, format,
                   GL_UNSIGNED_BYTE, data);
      RENDERER_CHECK_GLES("glTexImage2D");
    } else {
      glCompressedTexImage2D(GL_TEXTURE_2D, level,
                             kGLCompressedFormats[params.compression_type],
                             width, height, 0,
                             params.texture_sizes[mip_level], data);
      RENDERER_CHECK_GLES("glCompressedTexImage2D");
    }
    data += params.texture_sizes[mip_level];
    // Each mip dimension halves but never goes below 1 (non-square textures reach 1
    // along one axis first)
    width = (width > 1) ? (width >> 1) : 1;
    height = (height > 1) ? (height >> 1) : 1;
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, kGLMinFilters[params.min_filter]);
  RENDERER_CHECK_GLES("glTexParameteri GL_TEXTURE_MIN_FILTER");
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, kGLMagFilters[params.mag_filter]);
  RENDERER_CHECK_GLES("glTexParameteri GL_TEXTURE_MAG_FILTER");
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, kGLWrapS[params.wrap_s]);
  RENDERER_CHECK_GLES("glTexParameteri GL_TEXTURE_WRAP_S");
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, kGLWrapT[params.wrap_t]);
  RENDERER_CHECK_GLES("glTexParameteri GL_TEXTURE_WRAP_T");
  // Only the uploaded levels exist. Without this, a mipmapped min filter on a texture
  // with fewer levels than a full chain makes the texture incomplete (samples as black).
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(mip_count - 1));
  RENDERER_CHECK_GLES("glTexParameteri GL_TEXTURE_MAX_LEVEL");

  // Restore the caller's state
  glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previous_texture));
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, static_cast<GLuint>(previous_unpack_buffer));
  glPixelStorei(GL_UNPACK_ALIGNMENT, previous_unpack_alignment);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, previous_unpack_row_length);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, previous_unpack_skip_pixels);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, previous_unpack_skip_rows);
  RENDERER_CHECK_GLES("restore texture/unpack state");
}

TextureGLES::~TextureGLES() {
  // glDeleteTextures unbinds the texture wherever it is bound, so other bindings are
  // left alone
  if (texture_object_ != 0) {
    glDeleteTextures(1, &texture_object_);
    RENDERER_CHECK_GLES("glDeleteTextures");
    texture_object_ = 0;
  }
}
}