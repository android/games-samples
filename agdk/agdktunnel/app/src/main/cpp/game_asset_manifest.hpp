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

#ifndef agdktunnel_gameassetmanifest_hpp
#define agdktunnel_gameassetmanifest_hpp

#include <stddef.h>
#include "game_asset_manager.hpp"
#include "util.hpp"

namespace GameAssetManifest {

    static const char *MAIN_ASSETPACK_NAME = "TextureAssets";
    static const char *EXPANSION_ASSETPACK_NAME = "AdditionalTextureAssets";

    struct AssetPackDefinition {
        GameAssetManager::GameAssetPackType mPackType;
        size_t mPackFileCount;
        const char *mPackName;
        const char **mPackFiles;
    };

    class AssetPackInfo {
    public:
        AssetPackInfo(const AssetPackDefinition *definition) :
                mDefinition(definition),
                mAssetPackBasePath(NULL),
                mAssetPackDownloadSize(0),
                mAssetPackStatus(GameAssetManager::GAMEASSET_NOT_FOUND),
                mAssetPackCompletion(0.0f) {
        }

        ~AssetPackInfo() {
            if (mAssetPackBasePath != NULL) {
                delete[] mAssetPackBasePath;
            }
        }

        const AssetPackDefinition *mDefinition;
        const char *mAssetPackBasePath;
        uint64_t mAssetPackDownloadSize;
        GameAssetManager::GameAssetStatus mAssetPackStatus;
        float mAssetPackCompletion;
    };

    size_t AssetManifest_GetAssetPackCount();

    const AssetPackDefinition *AssetManifest_GetAssetPackDefinitions();
}

#endif
