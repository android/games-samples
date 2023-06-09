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

#include <android/asset_manager.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <vector>
#include "common.hpp"
#include "game_asset_manager.hpp"

#define ELEMENTS_OF(x) (sizeof(x) / sizeof(x[0]))
#define MAX_ASSET_PATH_LENGTH 512

namespace {
    /*
     * This is a rudimentary 'asset manifest' embedded in the source code for simplicity.
     * A real world project might generate a manifest file as part of the asset pipeline.
     */

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

    const char *InstallFileList[] = {"textures/InstallTime1.tex", "textures/InstallTime2.tex"};
    const char *FastFollowFileList[] = {"textures/FastFollow1.tex", "textures/FastFollow2.tex"};
    const char *OnDemandFileList[] = {"textures/OnDemand1.tex", "textures/OnDemand2.tex",
                                      "textures/OnDemand3.tex", "textures/OnDemand4.tex",};

    const AssetPackDefinition AssetPacks[] = {
            {
                    GameAssetManager::GAMEASSET_PACKTYPE_INTERNAL,
                    ELEMENTS_OF(InstallFileList),
                    INSTALL_ASSETPACK_NAME,
                    InstallFileList
            },
            {
                    GameAssetManager::GAMEASSET_PACKTYPE_INTERNAL,
                    ELEMENTS_OF(FastFollowFileList),
                    FASTFOLLOW_ASSETPACK_NAME,
                    FastFollowFileList
            },
            {
                    GameAssetManager::GAMEASSET_PACKTYPE_INTERNAL,
                    ELEMENTS_OF(OnDemandFileList),
                    ONDEMAND_ASSETPACK_NAME,
                    OnDemandFileList
            }
    };
}

// Internal implementation utility class
class GameAssetManagerInternals {
public:
    GameAssetManagerInternals(AAssetManager *assetManager, JavaVM *jvm, jobject nativeActivity);

    ~GameAssetManagerInternals();

    uint64_t GetExternalGameAssetSize(const char *assetName, AssetPackInfo *packInfo);

    uint64_t GetInternalGameAssetSize(const char *assetName);

    bool LoadExternalGameAsset(const char *assetName, const uint64_t bufferSize, void *loadBuffer,
                               AssetPackInfo *packInfo);

    bool LoadInternalGameAsset(const char *assetName, const uint64_t bufferSize, void *loadBuffer);

    void ChangeAssetPackStatus(AssetPackInfo *packInfo,
                               const GameAssetManager::GameAssetStatus newStatus) {
        if (packInfo->mAssetPackStatus != newStatus) {
            packInfo->mAssetPackStatus = newStatus;
        }
    }

    AssetPackInfo *GetAssetPack(const int index) {
        return index < mAssetPackCount ? mAssetPacks[index] : NULL;
    }

    AssetPackInfo *GetAssetPackByName(const char *assetPackName);

    int GetAssetPackCount() const {
        return mAssetPackCount;
    }

    const char *GetAssetPackErrorMessage() const {
        return mAssetPackErrorMessage;
    }

    AssetPackInfo *GetAssetPackForAssetName(const char *assetName);

    bool GenerateFullAssetPath(const char *assetName, const AssetPackInfo *packInfo,
                               char *pathBuffer, const size_t bufferSize);

    void SetAssetPackInitialStatus(AssetPackInfo &info);

private:
    AAssetManager *mAssetManager;
    std::vector<AssetPackInfo *> mAssetPacks;
    const char *mAssetPackErrorMessage;
    jobject mNativeActivity;
    int mAssetPackCount;
    bool mRequestingMobileDownload;
};

GameAssetManagerInternals::GameAssetManagerInternals(AAssetManager *assetManager, JavaVM *jvm,
                                                     jobject nativeActivity) {
    mAssetPackErrorMessage = "Generic Asset Error";

    mAssetManager = assetManager;
    mNativeActivity = nativeActivity;
    mAssetPackCount = ELEMENTS_OF(AssetPacks);
    mAssetPacks.reserve(mAssetPackCount);
    mRequestingMobileDownload = false;

    for (int i = 0; i < mAssetPackCount; ++i) {
        LOGD("GameAssetManager: Setting up asset pack %s", AssetPacks[i].mPackName);
        AssetPackInfo *assetPackInfo = new AssetPackInfo(&AssetPacks[i]);
        mAssetPacks.push_back(assetPackInfo);
        SetAssetPackInitialStatus(*assetPackInfo);
    }
}

GameAssetManagerInternals::~GameAssetManagerInternals() {
    // Delete our allocated asset pack info structures
    for (std::vector<AssetPackInfo *>::iterator iter = mAssetPacks.begin();
         iter != mAssetPacks.end(); ++iter) {
        delete *iter;
    }
    mAssetPacks.clear();
}

uint64_t GameAssetManagerInternals::GetExternalGameAssetSize(const char *assetName,
                                                             AssetPackInfo *packInfo) {
    uint64_t assetSize = 0;
    if (packInfo->mAssetPackBasePath == NULL) {
        // If a parent directory base path was not set, assume this is actually an internal
        // asset
        return GetInternalGameAssetSize(assetName);
    }

    char fullAssetFilePath[MAX_ASSET_PATH_LENGTH];
    if (GenerateFullAssetPath(assetName, packInfo, fullAssetFilePath, MAX_ASSET_PATH_LENGTH)) {
        FILE *fp = fopen(fullAssetFilePath, "rb");
        if (fp != NULL) {
            struct stat fileStats;
            int statResult = fstat(fileno(fp), &fileStats);
            if (statResult == 0) {
                assetSize = fileStats.st_size;
            }
            fclose(fp);
        }
    }
    return assetSize;
}

uint64_t GameAssetManagerInternals::GetInternalGameAssetSize(const char *assetName) {
    size_t assetSize = 0;
    AAsset *asset = AAssetManager_open(mAssetManager, assetName, AASSET_MODE_STREAMING);
    if (asset != NULL) {
        assetSize = AAsset_getLength(asset);
        AAsset_close(asset);
    }
    return assetSize;
}

bool
GameAssetManagerInternals::LoadExternalGameAsset(const char *assetName, const uint64_t bufferSize,
                                                 void *loadBuffer, AssetPackInfo *packInfo) {
    bool loadSuccessful = false;
    if (packInfo->mAssetPackBasePath == NULL) {
        // If a parent directory base path was not set, assume this is actually an internal
        // asset
        return LoadInternalGameAsset(assetName, bufferSize, loadBuffer);
    }

    char fullAssetFilePath[MAX_ASSET_PATH_LENGTH];
    if (GenerateFullAssetPath(assetName, packInfo, fullAssetFilePath, MAX_ASSET_PATH_LENGTH)) {
        FILE *fp = fopen(fullAssetFilePath, "rb");
        if (fp != NULL) {
            struct stat fileStats;
            uint64_t assetSize = 0;
            int statResult = fstat(fileno(fp), &fileStats);
            if (statResult == 0) {
                assetSize = fileStats.st_size;
                if (assetSize <= bufferSize) {
                    fread(loadBuffer, assetSize, 1, fp);
                    loadSuccessful = true;
                }
            }
            fclose(fp);
        }
    }

    return loadSuccessful;
}

bool
GameAssetManagerInternals::LoadInternalGameAsset(const char *assetName, const uint64_t bufferSize,
                                                 void *loadBuffer) {
    bool loadSuccess = false;

    AAsset *asset = AAssetManager_open(mAssetManager, assetName, AASSET_MODE_STREAMING);
    if (asset != NULL) {
        size_t assetSize = AAsset_getLength(asset);
        if (assetSize <= bufferSize) {
            AAsset_read(asset, loadBuffer, assetSize);
            loadSuccess = true;
        }
        AAsset_close(asset);
    }

    return loadSuccess;
}

AssetPackInfo *GameAssetManagerInternals::GetAssetPackByName(const char *assetPackName) {
    AssetPackInfo *packInfo = NULL;

    for (int i = 0; i < mAssetPackCount; ++i) {
        if (strcmp(assetPackName, mAssetPacks[i]->mDefinition->mPackName) == 0) {
            packInfo = mAssetPacks[i];
            break;
        }
    }
    return packInfo;
}

AssetPackInfo *GameAssetManagerInternals::GetAssetPackForAssetName(const char *assetName) {
    AssetPackInfo *packInfo = NULL;
    int i = 0;

    while (i < mAssetPackCount) {
        for (size_t j = 0; j < mAssetPacks[i]->mDefinition->mPackFileCount; ++j) {
            if (strcmp(assetName, mAssetPacks[i]->mDefinition->mPackFiles[j]) == 0) {
                packInfo = mAssetPacks[i];
                i = mAssetPackCount;
                break;
            }
        }
        ++i;
    }

    return packInfo;
}

bool GameAssetManagerInternals::GenerateFullAssetPath(const char *assetName,
                                                      const AssetPackInfo *packInfo,
                                                      char *pathBuffer, const size_t bufferSize) {
    bool generatedPath = false;
    const size_t requiredSize = strlen(assetName) + strlen(packInfo->mAssetPackBasePath) + 1;
    if (requiredSize < MAX_ASSET_PATH_LENGTH) {
        generatedPath = true;
        strncpy(pathBuffer, packInfo->mAssetPackBasePath, MAX_ASSET_PATH_LENGTH);
        strncat(pathBuffer, assetName, MAX_ASSET_PATH_LENGTH);
    }
    LOGD("GameAssetManager: full external asset path for %s : %s", assetName, pathBuffer);
    return generatedPath;
}

void GameAssetManagerInternals::SetAssetPackInitialStatus(AssetPackInfo &info) {
    // Assume we are present on device and ready to be used
    info.mAssetPackStatus = GameAssetManager::GAMEASSET_READY;
    info.mAssetPackCompletion = 1.0f;
}

// Main GameAssetManager class
GameAssetManager::GameAssetManager(AAssetManager *assetManager, JavaVM *jvm,
                                   jobject android_context) {
    mInternals = new GameAssetManagerInternals(assetManager, jvm, android_context);
}

GameAssetManager::~GameAssetManager() {
    delete mInternals;
    mInternals = NULL;
}

void GameAssetManager::OnPause() {

}

void GameAssetManager::OnResume() {

}

const char *GameAssetManager::GetGameAssetErrorMessage() {
    return "GENERIC ASSET ERROR MESSAGE";
}

void GameAssetManager::UpdateGameAssetManager() {

}

uint64_t GameAssetManager::GetGameAssetSize(const char *assetName) {
    uint64_t assetSize = 0;

    if (assetName != NULL) {
        AssetPackInfo *packInfo = mInternals->GetAssetPackForAssetName(assetName);
        if (packInfo != NULL) {
            if (packInfo->mAssetPackStatus == GameAssetManager::GAMEASSET_READY) {
                switch (packInfo->mDefinition->mPackType) {
                    case GAMEASSET_PACKTYPE_INTERNAL:
                        assetSize = mInternals->GetInternalGameAssetSize(assetName);
                        break;
                    case GAMEASSET_PACKTYPE_FASTFOLLOW:
                    case GAMEASSET_PACKTYPE_ONDEMAND:
                        assetSize = mInternals->GetExternalGameAssetSize(assetName, packInfo);
                        break;
                }
            }
        }
    }

    return assetSize;
}

bool
GameAssetManager::LoadGameAsset(const char *assetName, const size_t bufferSize, void *loadBuffer) {
    bool loadSuccess = false;

    if (assetName != NULL) {
        AssetPackInfo *packInfo = mInternals->GetAssetPackForAssetName(assetName);
        if (packInfo != NULL) {
            if (packInfo->mAssetPackStatus == GameAssetManager::GAMEASSET_READY) {
                switch (packInfo->mDefinition->mPackType) {
                    case GAMEASSET_PACKTYPE_INTERNAL:
                        loadSuccess = mInternals->LoadInternalGameAsset(assetName, bufferSize,
                                                                        loadBuffer);
                        break;
                    case GAMEASSET_PACKTYPE_FASTFOLLOW:
                    case GAMEASSET_PACKTYPE_ONDEMAND:
                        loadSuccess = mInternals->LoadExternalGameAsset(assetName, bufferSize,
                                                                        loadBuffer, packInfo);
                        break;
                }
            }
        }
    }

    return loadSuccess;
}

const char *GameAssetManager::GetGameAssetParentPackName(const char *assetName) {
    const char *assetPackName = NULL;

    if (assetName != NULL) {
        AssetPackInfo *packInfo = mInternals->GetAssetPackForAssetName(assetName);
        if (packInfo != NULL) {
            assetPackName = packInfo->mDefinition->mPackName;
        }
    }

    return assetPackName;
}

const char **
GameAssetManager::GetGameAssetPackFileList(const char *assetPackName, int *fileListSize) {
    AssetPackInfo *packInfo = mInternals->GetAssetPackByName(assetPackName);
    if (packInfo != NULL) {
        if (fileListSize != NULL) {
            *fileListSize = static_cast<int>(packInfo->mDefinition->mPackFileCount);
        }
        return packInfo->mDefinition->mPackFiles;
    }

    // Couldn't find pack, return zero files
    if (fileListSize != NULL) {
        *fileListSize = 0;
    }
    return NULL;
}

GameAssetManager::GameAssetStatus
GameAssetManager::GetGameAssetPackStatus(const char *assetPackName) {
    GameAssetManager::GameAssetStatus assetStatus = GAMEASSET_NOT_FOUND;

    if (assetPackName != NULL) {
        AssetPackInfo *packInfo = mInternals->GetAssetPackByName(assetPackName);
        if (packInfo != NULL) {
            assetStatus = packInfo->mAssetPackStatus;
        }
    }

    return assetStatus;
}

GameAssetManager::GameAssetPackType GameAssetManager::GetGameAssetPackType(
        const char *assetPackName) {
    GameAssetManager::GameAssetPackType assetPackType = GAMEASSET_PACKTYPE_INTERNAL;

    if (assetPackName != NULL) {
        AssetPackInfo *packInfo = mInternals->GetAssetPackByName(assetPackName);
        if (packInfo != NULL) {
            assetPackType = packInfo->mDefinition->mPackType;
        }
    }

    return assetPackType;
}

void GameAssetManager::RequestMobileDataDownloads() {

}

bool GameAssetManager::RequestDownload(const char *assetPackName) {
    return false;
}

void GameAssetManager::RequestDownloadCancellation(const char *assetPackName) {

}

bool GameAssetManager::RequestRemoval(const char *assetPackName) {
    return false;
}

GameAssetManager::GameAssetStatus GameAssetManager::GetDownloadStatus(
        const char *assetPackName, float *completionProgress, uint64_t *totalPackDownloadSize) {
    GameAssetManager::GameAssetStatus assetStatus = GAMEASSET_NOT_FOUND;

    if (assetPackName != NULL) {
        AssetPackInfo *packInfo = mInternals->GetAssetPackByName(assetPackName);
        if (packInfo != NULL) {
            assetStatus = packInfo->mAssetPackStatus;

            if (completionProgress != NULL) {
                *completionProgress = packInfo->mAssetPackCompletion;
            }

            if (totalPackDownloadSize != NULL) {
                *totalPackDownloadSize = packInfo->mAssetPackDownloadSize;
            }
        }
    }

    return assetStatus;
}
