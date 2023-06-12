/*
 * Copyright 2020 Google LLC
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

#include "common.hpp"
#include "game_asset_manager.hpp"
#include "native_engine.hpp"
#include "texture_manager.hpp"
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

namespace {
    enum TextureFileFormat {
        TEXTUREFILE_KTX = 0,
        TEXTUREFILE_ASTC,
        TEXTUREFILE_UNKNOWN
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
    static bool
    CreateFromASTCFile(const uint8_t *fileData, const size_t fileSize, GLuint *textureID) {
        bool success = false;
        const ASTCHeader *header = reinterpret_cast<const ASTCHeader *>(fileData);
        if (header->blockDepth == 1 && header->texDepth[0] == 1) {
            GLsizei textureWidth =
                    header->texWidth[0] | (header->texWidth[1] << 8) | (header->texWidth[2] << 8);
            GLsizei textureHeight = header->texHeight[0] | (header->texHeight[1] << 8) |
                                    (header->texHeight[2] << 8);
            GLenum textureFormat = 0;
            const GLsizei textureSize = static_cast<const GLsizei>(fileSize - sizeof(ASTCHeader));

            switch (header->blockWidth) {
                case 4:
                    if (header->blockHeight == 4) textureFormat = COMPRESSED_RGBA_ASTC_4x4_KHR;
                    break;
                case 5:
                    if (header->blockHeight == 4) textureFormat = COMPRESSED_RGBA_ASTC_5x4_KHR;
                    else if (header->blockHeight == 5) textureFormat = COMPRESSED_RGBA_ASTC_5x5_KHR;
                    break;
                case 6:
                    if (header->blockHeight == 5) textureFormat = COMPRESSED_RGBA_ASTC_6x5_KHR;
                    else if (header->blockHeight == 6) textureFormat = COMPRESSED_RGBA_ASTC_6x6_KHR;
                    break;
                case 8:
                    if (header->blockHeight == 5) textureFormat = COMPRESSED_RGBA_ASTC_8x5_KHR;
                    else if (header->blockHeight == 6) textureFormat = COMPRESSED_RGBA_ASTC_8x6_KHR;
                    else if (header->blockHeight == 8) textureFormat = COMPRESSED_RGBA_ASTC_8x8_KHR;
                    break;
                case 10:
                    if (header->blockHeight == 5) textureFormat = COMPRESSED_RGBA_ASTC_10x5_KHR;
                    else if (header->blockHeight == 6)
                        textureFormat = COMPRESSED_RGBA_ASTC_10x6_KHR;
                    else if (header->blockHeight == 8)
                        textureFormat = COMPRESSED_RGBA_ASTC_10x8_KHR;
                    else if (header->blockHeight == 10)
                        textureFormat = COMPRESSED_RGBA_ASTC_10x10_KHR;
                    break;
                case 12:
                    if (header->blockHeight == 10) textureFormat = COMPRESSED_RGBA_ASTC_12x10_KHR;
                    else if (header->blockHeight == 12)
                        textureFormat = COMPRESSED_RGBA_ASTC_12x12_KHR;
                    break;
                default:
                    break;
            }

            if (textureFormat > 0) {
                const void *textureData = reinterpret_cast<const void *>(header + 1);
                glGetError();
                glGenTextures(1, textureID);
                glBindTexture(GL_TEXTURE_2D, *textureID);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glCompressedTexImage2D(GL_TEXTURE_2D, 0, textureFormat, textureWidth, textureHeight,
                                       0, textureSize, textureData);
                GLenum glErr = glGetError();
                if (glErr == GL_NO_ERROR) {
                    success = true;
                } else {
                    LOGE("CreateFromASTCFile glCompressedTexImage2D error %d", glErr);
                }
            }
        }
        return success;
    }

    // .ktx texture file loader
    // This is not a robust KTX loader, ala libktx. It is only intended to load the KTX 1.1
    // ETC2 format, non mip-mapped 2D texture files included with this example
    static bool
    CreateFromKTXFile(const uint8_t *fileData, const size_t fileSize, GLuint *textureID) {
        bool success = false;
        const KTXHeader *header = reinterpret_cast<const KTXHeader *>(fileData);
        // end of key-value data is padded to four-byte alignment
        const size_t textureOffset = (sizeof(KTXHeader) + header->bytesOfKeyValueData + 3) & (~3);
        const uint32_t *textureSize = reinterpret_cast<const uint32_t *>(fileData + textureOffset);
        const void *textureData = reinterpret_cast<const void *>(textureSize + 1);

        if (header->glInternalFormat >= GL_COMPRESSED_RGB8_ETC2 &&
            header->glInternalFormat <= GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC) {
            // Clear error
            glGetError();
            glGenTextures(1, textureID);
            glBindTexture(GL_TEXTURE_2D, *textureID);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, header->glInternalFormat, header->pixelWidth,
                                   header->pixelHeight, 0, *textureSize, textureData);
            GLenum glErr = glGetError();
            if (glErr == GL_NO_ERROR) {
                success = true;
            } else {
                LOGE("CreateFromKTXFile glCompressedTexImage2D error %d", glErr);
            }
        }
        return success;
    }
}

TextureManager::TextureManager() {
    mDeviceSupportsASTC = false;
    mLastTextureFormat = TEXTUREFORMAT_ETC2;

    GLint extentionCount = 0;

    glGetIntegerv(GL_NUM_EXTENSIONS, &extentionCount);
    for (GLint i = 0; i < extentionCount; i++) {
        const GLubyte *extensionString = glGetStringi(GL_EXTENSIONS, i);
        if (strcmp(reinterpret_cast<const char *>(extensionString), ASTC_EXTENSION_STRING) == 0) {
            mDeviceSupportsASTC = true;
            break;
        }
    }
    LOGD("ASTC Textures: %s", (mDeviceSupportsASTC ? "Supported" : "Not Supported"));
}

TextureManager::~TextureManager() {
    glBindTexture(GL_TEXTURE_2D, 0);
    for (std::vector<TextureReference>::iterator iter = mTextures.begin(); iter != mTextures.end();
         ++iter) {
        GLuint textureID = static_cast<GLuint>(iter->mTextureReference);
        glDeleteTextures(1, &textureID);
    }
    mTextures.clear();
}

bool TextureManager::IsTextureLoaded(const char *textureName) {
    TextureManager::TextureReference textureRef = FindReferenceForName(textureName);
    return (textureRef.mTextureReference != TextureManager::INVALID_TEXTURE_REF);
}

bool TextureManager::LoadTexture(const char *textureName) {
    bool success = false;
    if (!IsTextureLoaded(textureName)) {
        GameAssetManager *gameAssetManager = NativeEngine::GetInstance()->GetGameAssetManager();
        uint64_t fileSize = gameAssetManager->GetGameAssetSize(textureName);
        if (fileSize > 0) {
            uint8_t *fileBuffer = static_cast<uint8_t *>(malloc(fileSize));
            if (gameAssetManager->LoadGameAsset(textureName, fileSize, fileBuffer)) {
                const TextureFileFormat fileFormat = GetFileFormat(fileBuffer, fileSize);
                GLuint textureID = 0;

                if (fileFormat == TEXTUREFILE_ASTC && mDeviceSupportsASTC) {
                    success = CreateFromASTCFile(fileBuffer, fileSize, &textureID);
                    mLastTextureFormat = TEXTUREFORMAT_ASTC;
                } else if (fileFormat == TEXTUREFILE_KTX) {
                    success = CreateFromKTXFile(fileBuffer, fileSize, &textureID);
                    mLastTextureFormat = TEXTUREFORMAT_ETC2;
                } else {
                    LOGE("TextureManager: unknown texture file format in file: %s", textureName);
                }

                if (success) {
                    mTextures.push_back(TextureManager::TextureReference(textureName,
                                                                         static_cast<uint64_t>(textureID)));
                }
                free(fileBuffer);
            } else {
                LOGE("TextureManager: failed to load texture file: %s", textureName);
            }
        }
    }
    return success;
}

uint64_t TextureManager::GetTextureReference(const char *textureName) {
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
    TextureManager::TextureReference emptyReference(NULL, INVALID_TEXTURE_REF);
    return emptyReference;
}
