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

#ifndef agdktunnel_texturemanager_hpp
#define agdktunnel_texturemanager_hpp

#include <cstdint>
#include <memory>
#include <vector>
#include "simple_renderer/renderer_texture.h"
#include "util.hpp"

class GameAssetManager;

/*
 * A very basic texture manager that handles loading compressed texture
 * files and generating GLES textures.
 */
class TextureManager {
public:
    static const uint64_t INVALID_TEXTURE_REF = 0xFFFFFFFFFFFFFFFFULL;

    enum TextureFormat {
        TEXTUREFORMAT_RGBA8888 = 0,
        TEXTUREFORMAT_ETC2,
        TEXTUREFORMAT_ASTC
    };

    TextureManager();

    ~TextureManager();

    bool IsTextureLoaded(const char *textureName);

    bool LoadTexture(const char *textureName);

    bool
    CreateTexture(const char *textureName, const size_t textureSize, const uint8_t *textureData);

    uint32_t GetTextureMipCount(const char *textureName);

    std::shared_ptr<simple_renderer::Texture> GetTexture(const char *textureName);

    TextureFormat GetTextureFormatInUse() { return mLastTextureFormat; }

private:

    struct TextureReference {
        TextureReference() = default;

        TextureReference(const uint32_t textureMipCount,
                         const char *textureName,
                         std::shared_ptr<simple_renderer::Texture> textureReference) :
                mTextureMipCount(textureMipCount),
                mTextureName(textureName), mTextureReference(textureReference) {}

        uint32_t mTextureMipCount;
        const char *mTextureName;
        std::shared_ptr<simple_renderer::Texture> mTextureReference;
    };

    TextureReference FindReferenceForName(const char *textureName);

    std::vector<TextureReference> mTextures;
    TextureFormat mLastTextureFormat;
    bool mDeviceSupportsASTC;
};

#endif
