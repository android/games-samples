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

#include "texture_asset_loader.h"
#include "Log.h"

#define LOG_TAG "TextureAssetLoader"

#include <png.h>

#include <android/asset_manager.h>
#include <cstring>
#include <stdlib.h>

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>


namespace {
    AAssetManager *assetManager = nullptr;

    constexpr size_t PNG_SIG_LENGTH = 8;

    struct PNG_Memory_Stream {
        uint8_t *base;
        size_t offset;
        size_t totalSize;
    };

    void PNG_Memory_Read(png_structp pngStruct, png_bytep destBuffer, png_size_t bytesToRead) {
        PNG_Memory_Stream *pngStream = static_cast<PNG_Memory_Stream *>(png_get_io_ptr(pngStruct));
        if (pngStream != NULL) {
            if ((bytesToRead + pngStream->offset) < pngStream->totalSize) {
                memcpy(destBuffer, pngStream->base + pngStream->offset, bytesToRead);
                pngStream->offset += bytesToRead;
            } else {
                ALOGE("PNG_Memory_Read beyond buffer %lu + %lu - %lu",
                      static_cast<unsigned long>(pngStream->offset),
                      static_cast<unsigned long>(bytesToRead),
                      static_cast<unsigned long>(pngStream->totalSize));
            }

        }
    }

    TextureAssetHandle createTexture(uint32_t texWidth, uint32_t texHeight, void *texPixels) {
        TextureAssetHandle returnHandle = TextureAssetLoader::INVALID_TEXTURE;
        // Reset GL error
        glGetError();
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, texPixels);
        GLenum glErr = glGetError();
        if (glErr == GL_NO_ERROR) {
            returnHandle = static_cast<TextureAssetHandle>(textureID);
        } else {
            ALOGE("glTexImage2D error %d", glErr);
        }
        return returnHandle;
    }

    TextureAssetHandle
    loadTextureAsset_LIBPNG(const char *filename, uint32_t *textureWidth,
                            uint32_t *textureHeight) {
        TextureAssetHandle returnHandle = TextureAssetLoader::INVALID_TEXTURE;
        if (assetManager != nullptr && filename != nullptr) {
            AAsset *textureAsset = AAssetManager_open(assetManager, filename,
                                                      AASSET_MODE_STREAMING);
            if (textureAsset != nullptr) {
                const size_t textureFileSize = AAsset_getLength(textureAsset);
                void *textureFileBytes = malloc(textureFileSize);
                if (textureFileBytes != nullptr) {
                    AAsset_read(textureAsset, textureFileBytes, textureFileSize);
                    AAsset_close(textureAsset);
                    if (png_sig_cmp(static_cast<png_const_bytep>(textureFileBytes), 0,
                                    PNG_SIG_LENGTH) == 0) {
                        // Valid PNG file
                        png_structp pngStruct = nullptr;
                        pngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                                           NULL, NULL, NULL);
                        if (pngStruct != nullptr) {
                            png_infop pngInfo = nullptr;
                            pngInfo = png_create_info_struct(pngStruct);
                            if (pngInfo != nullptr) {
                                PNG_Memory_Stream memoryStream{
                                        static_cast<uint8_t *>(textureFileBytes), PNG_SIG_LENGTH,
                                        textureFileSize};
                                png_set_read_fn(pngStruct, &memoryStream, PNG_Memory_Read);
                                png_set_sig_bytes(pngStruct, PNG_SIG_LENGTH);

                                png_read_info(pngStruct, pngInfo);
                                png_uint_32 pngWidth = 0;
                                png_uint_32 pngHeight = 0;
                                int pngDepth = 0;
                                int pngColorType = -1;
                                if (png_get_IHDR(pngStruct, pngInfo, &pngWidth, &pngHeight,
                                                 &pngDepth, &pngColorType,
                                                 NULL, NULL, NULL) == 1) {
                                    // Only supporting 8bpp RGBA files
                                    if (pngWidth > 0 && pngHeight > 0 && pngDepth == 8 &&
                                        pngColorType == PNG_COLOR_TYPE_RGB_ALPHA) {
                                        const size_t rowBytes = png_get_rowbytes(pngStruct,
                                                                                 pngInfo);
                                        const size_t texSize = pngWidth * pngHeight * 4;
                                        void *rowPixels = malloc(rowBytes);
                                        void *texPixels = malloc(texSize);
                                        uint8_t *writePixel = static_cast<uint8_t *>(texPixels);
                                        for (png_uint_32 yIdx = 0; yIdx < pngHeight; ++yIdx) {
                                            png_read_row(pngStruct,
                                                         static_cast<png_bytep>(rowPixels), NULL);
                                            const uint8_t *rowPixel =
                                                    static_cast<const uint8_t *>(rowPixels);
                                            for (png_uint_32 xIndex = 0;
                                                 xIndex < pngWidth; ++xIndex) {
                                                *writePixel++ = *rowPixel++;
                                                *writePixel++ = *rowPixel++;
                                                *writePixel++ = *rowPixel++;
                                                *writePixel++ = *rowPixel++;
                                            }
                                        }

                                        returnHandle = createTexture(pngWidth, pngHeight,
                                                                     texPixels);
                                        if (returnHandle != TextureAssetLoader::INVALID_TEXTURE) {
                                            if (textureWidth != nullptr) {
                                                *textureWidth = pngWidth;
                                            }
                                            if (textureHeight != nullptr) {
                                                *textureHeight = pngHeight;
                                            }
                                        }

                                        free(texPixels);
                                        free(rowPixels);
                                    } else {
                                        ALOGE("Not a 8bpp RGBA png file");
                                    }
                                }
                                png_destroy_read_struct(&pngStruct, &pngInfo, NULL);
                            } else {
                                png_destroy_read_struct(&pngStruct, NULL, NULL);
                            }
                        }
                    }
                    free(textureFileBytes);
                }
            }
        }
        return returnHandle;
    }
}

void TextureAssetLoader::setAssetManager(AAssetManager *appAssetManager) {
    assetManager = appAssetManager;
}

TextureAssetHandle
TextureAssetLoader::loadTextureAsset(const char *filename, uint32_t *textureWidth,
                                     uint32_t *textureHeight) {

    return loadTextureAsset_LIBPNG(filename, textureWidth, textureHeight);
}

void TextureAssetLoader::unloadTextureAsset(const uint64_t textureReference) {
    if (textureReference != INVALID_TEXTURE) {
        GLuint textureID = static_cast<GLuint>(textureReference);
        glDeleteTextures(1, &textureID);
    }
}
