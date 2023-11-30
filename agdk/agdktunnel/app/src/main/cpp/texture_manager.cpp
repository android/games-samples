/*
 * Copyright 2021 The Android Open Source Project
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

#include "texture_manager.hpp"
#include "common.hpp"
#include "game_asset_manager.hpp"
#include "tunnel_engine.hpp"
#include "simple_renderer/renderer_interface.h"

using namespace simple_renderer;

enum TextureFileFormat {
    TEXTUREFILE_KTX = 0,
    TEXTUREFILE_ASTC,
    TEXTUREFILE_UNKNOWN
};

// ETC2 bpp sizes
static const uint32_t ETC2_BitsPerPixel[] = {
        4, // GL_COMPRESSED_RGB8_ETC2
        4, // GL_COMPRESSED_SRGB8_ETC2
        4, // GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
        4, // GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2
        8, // GL_COMPRESSED_RGBA8_ETC2_EAC
        8  // GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC
};

// ASTC GL internalformat values
static const GLenum COMPRESSED_RGBA_ASTC_4x4_KHR = 0x93B0;
static const GLenum COMPRESSED_RGBA_ASTC_5x4_KHR = 0x93B1;
static const GLenum COMPRESSED_RGBA_ASTC_5x5_KHR = 0x93B2;
static const GLenum COMPRESSED_RGBA_ASTC_6x5_KHR = 0x93B3;
static const GLenum COMPRESSED_RGBA_ASTC_6x6_KHR = 0x93B4;
static const GLenum COMPRESSED_RGBA_ASTC_8x5_KHR = 0x93B5;
static const GLenum COMPRESSED_RGBA_ASTC_8x6_KHR = 0x93B6;
static const GLenum COMPRESSED_RGBA_ASTC_8x8_KHR = 0x93B7;
static const GLenum COMPRESSED_RGBA_ASTC_10x5_KHR = 0x93B8;
static const GLenum COMPRESSED_RGBA_ASTC_10x6_KHR = 0x93B9;
static const GLenum COMPRESSED_RGBA_ASTC_10x8_KHR = 0x93BA;
static const GLenum COMPRESSED_RGBA_ASTC_10x10_KHR = 0x93BB;
static const GLenum COMPRESSED_RGBA_ASTC_12x10_KHR = 0x93BC;
static const GLenum COMPRESSED_RGBA_ASTC_12x12_KHR = 0x93BD;

// .astc file format info
static const char *ASTC_EXTENSION_STRING = "GL_OES_texture_compression_astc";
static const int ASTC_MAGIC_SIZE = 4;
static const uint8_t ASTC_MAGIC[ASTC_MAGIC_SIZE] = {0x13, 0xAB, 0xA1, 0x5C};

struct ASTCHeader {
    uint8_t magic[ASTC_MAGIC_SIZE];
    uint8_t blockWidth;
    uint8_t blockHeight;
    uint8_t blockDepth;
    uint8_t texWidth[3];
    uint8_t texHeight[3];
    uint8_t texDepth[3];
};


// .ktx file format info
static const uint32_t ETC2FORMAT_START = 0x9274; // GL_COMPRESSED_RGB8_ETC2
static const uint32_t ETC2FORMAT_END = 0x9279; // GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC
static const int KTX_IDENTIFIER_SIZE = 12;
static const uint8_t KTX_11_IDENTIFIER[KTX_IDENTIFIER_SIZE] = {
        0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
};

struct KTXHeader {
    uint8_t identifier[12];
    uint32_t endianness;
    uint32_t glType;
    uint32_t glTypeSize;
    uint32_t glFormat;
    uint32_t glInternalFormat;
    uint32_t glBaseInternalFormat;
    uint32_t pixelWidth;
    uint32_t pixelHeight;
    uint32_t pixelDepth;
    uint32_t numberOfArrayElements;
    uint32_t numberOfFaces;
    uint32_t numberOfMipmapLevels;
    uint32_t bytesOfKeyValueData;
};

// File format identification utility function
static TextureFileFormat GetFileFormat(const uint8_t *fileData, const size_t fileSize) {
    if (fileSize > sizeof(ASTCHeader)) {
        bool bValidMagic = true;
        for (int i = 0; i < ASTC_MAGIC_SIZE; ++i) {
            if (fileData[i] != ASTC_MAGIC[i]) {
                bValidMagic = false;
                break;
            }
        }
        if (bValidMagic) {
            return TEXTUREFILE_ASTC;
        }
    }

    if (fileSize > sizeof(KTXHeader)) {
        bool bValidIdentifier = true;
        for (int i = 0; i < KTX_IDENTIFIER_SIZE; ++i) {
            if (fileData[i] != KTX_11_IDENTIFIER[i]) {
                bValidIdentifier = false;
                break;
            }
        }
        if (bValidIdentifier) {
            return TEXTUREFILE_KTX;
        }
    }
    return TEXTUREFILE_UNKNOWN;
}

// .astc texture file loader
static std::shared_ptr<simple_renderer::Texture> CreateFromASTCFile(const char* textureName,
                                                                    const uint8_t* fileData,
                                                                    const size_t fileSize) {
    std::shared_ptr<simple_renderer::Texture> texture = nullptr;
    const ASTCHeader* header = reinterpret_cast<const ASTCHeader *>(fileData);
    if (header->blockDepth == 1 && header->texDepth[0] == 1) {
        const uint32_t texture_size = static_cast<const uint32_t>(fileSize - sizeof(ASTCHeader));
        Texture::TextureCreationParams params;
        params.format = Texture::kTextureFormat_ASTC;
        params.compression_type = Texture::kTextureCompression_Count;
        params.min_filter = Texture::kMinFilter_Linear;
        params.mag_filter = Texture::kMagFilter_Linear;
        params.wrap_s = Texture::kWrapS_Repeat;
        params.wrap_t = Texture::kWrapT_Repeat;
        params.base_width =
            header->texWidth[0] | (header->texWidth[1] << 8) |
                (header->texWidth[2] << 8);
        params.base_height =
            header->texHeight[0] | (header->texHeight[1] << 8) |
                (header->texHeight[2] << 8);
        params.mip_count = 1;
        params.texture_sizes = &texture_size;
        params.texture_data = (fileData + sizeof(ASTCHeader));

        switch (header->blockWidth) {
            case 4:
                if (header->blockHeight == 4)
                    params.compression_type = Texture::kTextureCompression_ASTC_LDR_4x4;
            break;
            case 5:
                if (header->blockHeight == 4)
                    params.compression_type = Texture::kTextureCompression_ASTC_LDR_5x4;
                else if (header->blockHeight == 5)
                    params.compression_type = Texture::kTextureCompression_ASTC_LDR_5x5;
            break;
            case 6:
                if (header->blockHeight == 5)
                    params.compression_type = Texture::kTextureCompression_ASTC_LDR_6x5;
                else if (header->blockHeight == 6)
                    params.compression_type = Texture::kTextureCompression_ASTC_LDR_6x6;
            break;
            case 8:
                if (header->blockHeight == 5)
                    params.compression_type = Texture::kTextureCompression_ASTC_LDR_8x5;
                else if (header->blockHeight == 6)
                    params.compression_type = Texture::kTextureCompression_ASTC_LDR_8x6;
                else if (header->blockHeight == 8)
                    params.compression_type = Texture::kTextureCompression_ASTC_LDR_8x8;
            break;
            case 10:
                if (header->blockHeight == 5)
                    params.compression_type = Texture::kTextureCompression_ASTC_LDR_10x5;
                else if (header->blockHeight == 6)
                    params.compression_type = Texture::kTextureCompression_ASTC_LDR_10x6;
                else if (header->blockHeight == 8)
                    params.compression_type = Texture::kTextureCompression_ASTC_LDR_10x8;
                else if (header->blockHeight == 10)
                    params.compression_type = Texture::kTextureCompression_ASTC_LDR_10x10;
            break;
            case 12:
                if (header->blockHeight == 10)
                    params.compression_type = Texture::kTextureCompression_ASTC_LDR_12x10;
                else if (header->blockHeight == 12)
                    params.compression_type = Texture::kTextureCompression_ASTC_LDR_12x12;
            break;
            default:
                break;
        }
        // Check for valid compression type
        if (params.compression_type != Texture::kTextureCompression_Count) {
            texture = simple_renderer::Renderer::GetInstance().CreateTexture(params);
            if (texture.get() != nullptr) {
                texture->SetTextureDebugName(textureName);
            }
        }
    }
    return texture;
}

// .ktx texture file loader
// This is not a robust KTX loader, ala libktx. It is only intended to load the KTX 1.1
// ETC2 format, mip-mapped 2D texture files included with this example
static std::shared_ptr<simple_renderer::Texture> CreateFromKTXFile(const char* texture_name,
                                                                   const uint8_t* file_data, const size_t file_size) {
    std::shared_ptr<simple_renderer::Texture> texture = nullptr;
    const KTXHeader* header = reinterpret_cast<const KTXHeader *>(file_data);
    if (header->glInternalFormat >= ETC2FORMAT_START && header->glInternalFormat <= ETC2FORMAT_END) {
        // end of key-value data is padded to four-byte alignment
        const size_t texture_offset = (sizeof(KTXHeader) + header->bytesOfKeyValueData + 3) & (~3);
        const uint32_t* texture_size = reinterpret_cast<const uint32_t *>(file_data + texture_offset);
        const uint8_t* texture_data = reinterpret_cast<const uint8_t *>(texture_size + 1);

        Texture::TextureCreationParams params;
        params.format = simple_renderer::Texture::kTextureFormat_ETC2;
        params.compression_type = static_cast<Texture::TextureCompressionType>(
            Texture::kTextureCompression_ETC2_RGB8 +
                (header->glInternalFormat - ETC2FORMAT_START));
        params.min_filter = Texture::kMinFilter_Linear;
        params.mag_filter = Texture::kMagFilter_Linear;
        params.wrap_s = Texture::kWrapS_Repeat;
        params.wrap_t = Texture::kWrapT_Repeat;
        params.base_width = header->pixelWidth;
        params.base_height = header->pixelHeight;
        params.mip_count = 1; // only load the first mip (header->number_of_mipmap_levels);
        params.texture_sizes = texture_size;
        params.texture_data = texture_data;
        texture = simple_renderer::Renderer::GetInstance().CreateTexture(params);
        if (texture.get() != nullptr) {
            texture->SetTextureDebugName(texture_name);
        }
    }
    return texture;
}

TextureManager::TextureManager() {
    mDeviceSupportsASTC = false;
    mLastTextureFormat = TEXTUREFORMAT_ETC2;

    mDeviceSupportsASTC = simple_renderer::Renderer::GetInstance().GetFeatureAvailable(
        simple_renderer::Renderer::kFeature_ASTC);
    ALOGI("ASTC Textures: %s", (mDeviceSupportsASTC ? "Supported" : "Not Supported"));
}

TextureManager::~TextureManager() {
    for (std::vector<TextureReference>::iterator iter = mTextures.begin(); iter != mTextures.end();
         ++iter) {
        simple_renderer::Renderer::GetInstance().DestroyTexture(iter->mTextureReference);
    }
    mTextures.clear();
}

bool TextureManager::IsTextureLoaded(const char *textureName) {
    TextureManager::TextureReference textureRef = FindReferenceForName(textureName);
    return (textureRef.mTextureReference != nullptr);
}

bool TextureManager::LoadTexture(const char *textureName) {
    bool success = false;
    if (!IsTextureLoaded(textureName)) {
        GameAssetManager *gameAssetManager = TunnelEngine::GetInstance()->GetGameAssetManager();
        uint64_t fileSize = gameAssetManager->GetGameAssetSize(textureName);
        if (fileSize > 0) {
            uint8_t *fileBuffer = static_cast<uint8_t *>(malloc(fileSize));
            if (gameAssetManager->LoadGameAsset(textureName, fileSize, fileBuffer)) {
                success = CreateTexture(textureName, fileSize, fileBuffer);
            } else {
                ALOGE("TextureManager: failed to load texture file: %s", textureName);
            }
        }
    }
    return success;
}

bool TextureManager::CreateTexture(const char *textureName, const size_t textureSize,
                                   const uint8_t *textureData) {
    std::shared_ptr<simple_renderer::Texture> newTexture = nullptr;
    const TextureFileFormat fileFormat = GetFileFormat(textureData, textureSize);

    if (fileFormat == TEXTUREFILE_ASTC && mDeviceSupportsASTC) {
      newTexture = CreateFromASTCFile(textureName, textureData, textureSize);
        mLastTextureFormat = TEXTUREFORMAT_ASTC;
    } else if (fileFormat == TEXTUREFILE_KTX) {
      newTexture = CreateFromKTXFile(textureName, textureData, textureSize);
        mLastTextureFormat = TEXTUREFORMAT_ETC2;
    } else {
        ALOGE("TextureManager: unknown texture file format in file: %s", textureName);
    }

    if (newTexture.get() != nullptr) {
        mTextures.push_back(TextureManager::TextureReference(1, textureName,
                                                             newTexture));
    }
    free((void *) textureData);
    return (newTexture.get() != nullptr);
}

uint32_t TextureManager::GetTextureMipCount(const char *textureName) {
    TextureManager::TextureReference textureRef = FindReferenceForName(textureName);
    return textureRef.mTextureMipCount;
}

std::shared_ptr<simple_renderer::Texture> TextureManager::GetTexture(const char *textureName) {
    TextureManager::TextureReference textureRef = FindReferenceForName(textureName);
    return textureRef.mTextureReference;
}

TextureManager::TextureReference TextureManager::FindReferenceForName(const char *textureName) {
    for (std::vector<TextureReference>::iterator iter = mTextures.begin();
         iter != mTextures.end(); ++iter) {
        if (strcmp(textureName, iter->mTextureName) == 0) {
            return *iter;
        }
    }
    TextureManager::TextureReference emptyReference(0, NULL, nullptr);
    return emptyReference;
}
