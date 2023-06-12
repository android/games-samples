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

#ifndef nativegamepad_texturemanager_hpp
#define nativegamepad_texturemanager_hpp

#include <cstdint>
#include <vector>
#include "util.hpp"

class GameAssetManager;

/*
 * A very basic texture manager that handles loading and storing textures
 * for display by ImGui
 */
class TextureManager {
public:
    static const uint64_t INVALID_TEXTURE_REF = 0xFFFFFFFFFFFFFFFFULL;

    enum TextureFormat {
        TEXTUREFORMAT_ETC2 = 0,
        TEXTUREFORMAT_ASTC
    };

    TextureManager();

    ~TextureManager();

    bool IsTextureLoaded(const char *textureName);

    bool LoadTexture(const char *textureName);

    uint64_t GetTextureReference(const char *textureName);

    TextureFormat GetTextureFormatInUse() { return mLastTextureFormat; }

private:

    struct TextureReference {
        TextureReference() = default;

        TextureReference(const char *textureName, const uint64_t textureReference) :
                mTextureName(textureName), mTextureReference(textureReference) {}

        const char *mTextureName;
        uint64_t mTextureReference;
    };

    TextureReference FindReferenceForName(const char *textureName);

    std::vector<TextureReference> mTextures;
    TextureFormat mLastTextureFormat;
    bool mDeviceSupportsASTC;
};

#endif
