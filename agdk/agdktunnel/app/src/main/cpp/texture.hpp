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

#ifndef agdktunnel_texture_hpp
#define agdktunnel_texture_hpp

#include "common.hpp"

/* Represents an OpenGL texture */
class Texture {
private:
    GLuint mTextureH;
    GLuint mTextureMipCount;

public:
    inline Texture() {
        mTextureH = 0;
        mTextureMipCount = 0;
    }

    inline Texture(const GLuint textureH, const GLuint textureMipCount) {
        mTextureH = textureH;
        mTextureMipCount = textureMipCount;
    }

    // Initialize from raw RGB data. If hasAlpha is true, then it's 4 bytes per pixel
    // (RGBA), otherwise it's interpreted as 3 bytes per pixel (RGB).
    void InitFromRawRGB(int width, int height, bool hasAlpha, const unsigned char *data);

    void Bind(int unit);

    void Unbind();
};

#endif
