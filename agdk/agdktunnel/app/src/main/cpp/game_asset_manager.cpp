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

#include <android/asset_manager.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <vector>
#include "common.hpp"
#include "game_asset_manager.hpp"
#include "game_asset_manifest.hpp"

#if !defined(NO_ASSET_PACKS)
#include "play/asset_pack.h"
#endif

#define ELEMENTS_OF(x) (sizeof(x) / sizeof(x[0]))
#define MAX_ASSET_PATH_LENGTH 512

using namespace GameAssetManifest;

// Internal implementation utility class
class GameAssetManagerInternals {
public:
    GameAssetManagerInternals(AAssetManager *assetManager, JavaVM *jvm, jobject nativeActivity);

    ~GameAssetManagerInternals();

    uint64_t GetExternalGameAssetSize(const char *assetName, AssetPackInfo *packInfo);

    uint64_t GetInternalGameAssetSize(const char *assetName);

    void LoadGameAssetAsync(const char *assetName, const uint64_t bufferSize,
                            void *loadBuffer, LoadingCompleteCallback callback,
                            AssetPackInfo *packInfo, bool isInternal, void* userData);

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

    // Asset Pack Manager support functions below
    bool GetAssetPackManagerInitialized() const { return mAssetPackManagerInitialized; }

#if !defined(NO_ASSET_PACKS)
    // Requests
    bool RequestAssetPackDownload(const char *assetPackName);

    void RequestAssetPackCancelDownload(const char *assetPackName);

    bool RequestAssetPackRemoval(const char *assetPackName);

    void RequestMobileDataDownloads();

    // Update processing
    void UpdateAssetPackBecameAvailable(AssetPackInfo *assetPackInfo);

    void UpdateAssetPackFromDownloadState(AssetPackInfo *assetPackInfo,
                                          AssetPackDownloadState *downloadState);

    void UpdateMobileDataRequestStatus();

    // Error reporting utility
    void SetAssetPackErrorStatus(const AssetPackErrorCode assetPackErrorCode,
                                 AssetPackInfo *assetPackInfo, const char *message);
#endif // !NO_ASSET_PACKS

private:
    LoadingThread *mLoadingThread;
    AAssetManager *mAssetManager;
    std::vector<AssetPackInfo *> mAssetPacks;
    const char *mAssetPackErrorMessage;
    jobject mNativeActivity;
    int mAssetPackCount;
    bool mRequestingMobileDownload;
    bool mAssetPackManagerInitialized;
};

GameAssetManagerInternals::GameAssetManagerInternals(AAssetManager *assetManager, JavaVM *jvm,
                                                     jobject nativeActivity) {
#if defined(NO_ASSET_PACKS)
    mAssetPackErrorMessage = "No Error";
    mAssetPackManagerInitialized = true;
#else // !NO_ASSET_PACKS
    // Initialize the asset pack manager
    AssetPackErrorCode assetPackErrorCode = AssetPackManager_init(jvm, nativeActivity);
    if (assetPackErrorCode == ASSET_PACK_NO_ERROR) {
        ALOGI("GameAssetManager: Initialized Asset Pack Manager");
        mAssetPackErrorMessage = "No Error";
        mAssetPackManagerInitialized = true;
    } else {
        mAssetPackManagerInitialized = false;
        SetAssetPackErrorStatus(assetPackErrorCode, NULL,
                                "GameAssetManager: Asset Pack Manager initialization");
    }
#endif // NO_ASSET_PACKS

    mAssetManager = assetManager;
    mNativeActivity = nativeActivity;
    mAssetPackCount = AssetManifest_GetAssetPackCount();
    const AssetPackDefinition *AssetPacks = AssetManifest_GetAssetPackDefinitions();
    mAssetPacks.reserve(mAssetPackCount);
    mRequestingMobileDownload = false;

    for (int i = 0; i < mAssetPackCount; ++i) {
        ALOGI("GameAssetManager: Setting up asset pack %s", AssetPacks[i].mPackName);
        AssetPackInfo *assetPackInfo = new AssetPackInfo(&AssetPacks[i]);
        mAssetPacks.push_back(assetPackInfo);
        SetAssetPackInitialStatus(*assetPackInfo);
    }

#if !defined NO_ASSET_PACKS
    if (mAssetPackManagerInitialized) {
        // Start asynchronous requests to get information about our asset packs
        for (int i = 0; i < mAssetPackCount; ++i) {
            const char *packName = AssetPacks[i].mPackName;
            assetPackErrorCode = AssetPackManager_requestInfo(&packName, 1);
            if (assetPackErrorCode == ASSET_PACK_NO_ERROR) {
                ALOGI("GameAssetManager: Requested asset pack info for %s", packName);
            } else {
                mAssetPackManagerInitialized = false;
                SetAssetPackErrorStatus(assetPackErrorCode, mAssetPacks[i],
                                        "GameAssetManager: requestInfo");
                break;
            }
        }
    }
#endif

    mLoadingThread = new LoadingThread(mAssetManager);
}

GameAssetManagerInternals::~GameAssetManagerInternals() {
    delete mLoadingThread;

    // Delete our allocated asset pack info structures
    for (std::vector<AssetPackInfo *>::iterator iter = mAssetPacks.begin();
         iter != mAssetPacks.end(); ++iter) {
        delete *iter;
    }
    mAssetPacks.clear();

#if !defined(NO_ASSET_PACKS)
    // Shut down the asset pack manager
    AssetPackManager_destroy();
#endif
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
    } else {
        ALOGI("GameAssetManager: asset %s found to be NULL", assetName);
    }
    return assetSize;
}

void
GameAssetManagerInternals::LoadGameAssetAsync(const char *assetName, const uint64_t bufferSize,
                                              void *loadBuffer, LoadingCompleteCallback callback,
                                              AssetPackInfo *packInfo,
                                              bool isInternal,
                                              void* userData) {

    char *assetPath = NULL;
    if (packInfo->mAssetPackBasePath == NULL) {
        // If a parent directory base path was not set, assume this is actually an internal
        // asset
        isInternal = true;
    }

    if (!isInternal) {
        assetPath = new char[MAX_ASSET_PATH_LENGTH];
        GenerateFullAssetPath(assetName, packInfo, assetPath, MAX_ASSET_PATH_LENGTH);
    }
    mLoadingThread->StartAssetLoad(assetName, assetPath, bufferSize, loadBuffer, callback,
                                   isInternal, userData);
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
    ALOGI("GameAssetManager: full external asset path for %s : %s", assetName, pathBuffer);
    return generatedPath;
}

void GameAssetManagerInternals::SetAssetPackInitialStatus(AssetPackInfo &info) {
#if defined NO_ASSET_PACKS
    info.mAssetPackStatus = GameAssetManager::GAMEASSET_READY;
    info.mAssetPackCompletion = 1.0f;
#else
    if (info.mDefinition->mPackType == GameAssetManager::GAMEASSET_PACKTYPE_INTERNAL) {
        // if internal assume we are present on device and ready to be used
        info.mAssetPackStatus = GameAssetManager::GAMEASSET_READY;
        info.mAssetPackCompletion = 1.0f;
    } else {
        // mark as waiting for status since the asset pack status query is
        // an async operation
        info.mAssetPackStatus = GameAssetManager::GAMEASSET_WAITING_FOR_STATUS;
    }
#endif
}

// New asset pack support functions
#if !defined(NO_ASSET_PACKS)
bool GameAssetManagerInternals::RequestAssetPackDownload(const char *assetPackName) {
    ALOGI("GameAssetManager: RequestAssetPackDownload %s", assetPackName);
    AssetPackErrorCode assetPackErrorCode = AssetPackManager_requestDownload(&assetPackName, 1);
    bool success = (assetPackErrorCode == ASSET_PACK_NO_ERROR);

    if (success) {
        ChangeAssetPackStatus(GetAssetPackByName(assetPackName),
                GameAssetManager::GAMEASSET_DOWNLOADING);
    } else {
        SetAssetPackErrorStatus(assetPackErrorCode, GetAssetPackByName(assetPackName),
                "GameAssetManager: requestDownload");
    }
    return success;
}

void GameAssetManagerInternals::RequestAssetPackCancelDownload(const char *assetPackName) {
    ALOGI("GameAssetManager: RequestAssetPackCancelDownload %s", assetPackName);
    // Request cancellation of the download, this is a request, it is not guaranteed
    // that the download will be canceled.
    AssetPackManager_cancelDownload(&assetPackName, 1);
}

bool GameAssetManagerInternals::RequestAssetPackRemoval(const char *assetPackName) {
    ALOGI("GameAssetManager: RequestAssetPackRemoval %s", assetPackName);
    AssetPackErrorCode assetPackErrorCode = AssetPackManager_requestRemoval(assetPackName);
    bool success = (assetPackErrorCode == ASSET_PACK_NO_ERROR);

    if (success) {
        ChangeAssetPackStatus(GetAssetPackByName(assetPackName),
                              GameAssetManager::GAMEASSET_PENDING_ACTION);

    } else {
        SetAssetPackErrorStatus(assetPackErrorCode, GetAssetPackByName(assetPackName),
                                "GameAssetManager: requestDelete");
    }
    return success;
}

void GameAssetManagerInternals::RequestMobileDataDownloads() {
    ALOGI("GameAssetManager: RequestMobileDataDownloads");
    AssetPackErrorCode assetPackErrorCode = AssetPackManager_showCellularDataConfirmation(
            mNativeActivity);
    SetAssetPackErrorStatus(assetPackErrorCode, NULL,
                            "GameAssetManager: RequestCellularDownload");
    if (assetPackErrorCode == ASSET_PACK_NO_ERROR) {
        mRequestingMobileDownload = true;
    }
}

void GameAssetManagerInternals::UpdateAssetPackBecameAvailable(AssetPackInfo *assetPackInfo) {
    ALOGI("GameAssetManager: ProcessAssetPackBecameAvailable : %s",
         assetPackInfo->mDefinition->mPackName);
    if (assetPackInfo->mAssetPackStatus != GameAssetManager::GAMEASSET_READY) {
        assetPackInfo->mAssetPackStatus = GameAssetManager::GAMEASSET_READY;
        assetPackInfo->mAssetPackCompletion = 1.0f;

        // Get the path of the directory containing the asset files for
        // this asset pack
        AssetPackLocation *assetPackLocation = NULL;
        AssetPackErrorCode assetPackErrorCode = AssetPackManager_getAssetPackLocation(
                assetPackInfo->mDefinition->mPackName, &assetPackLocation);

        if (assetPackErrorCode == ASSET_PACK_NO_ERROR) {
            AssetPackStorageMethod storageMethod = AssetPackLocation_getStorageMethod(
                    assetPackLocation);
            if (storageMethod == ASSET_PACK_STORAGE_FILES) {
                const char *assetPackPath = AssetPackLocation_getAssetsPath(assetPackLocation);
                if (assetPackPath != NULL) {
                    // Make a copy of the path, and add a path delimiter to the end
                    // if it isn't already present
                    size_t pathLength = strlen(assetPackPath);
                    bool needPathDelimiter = (assetPackPath[pathLength] != '/');
                    if (needPathDelimiter) {
                        ++pathLength;
                    }
                    char *pathCopy = new char[pathLength + 1];
                    pathCopy[pathLength] = '\0';
                    strncpy(pathCopy, assetPackPath, pathLength);
                    if (needPathDelimiter) {
                        pathCopy[pathLength - 1] = '/';
                    }
                    assetPackInfo->mAssetPackBasePath = pathCopy;
                }
            }
            AssetPackLocation_destroy(assetPackLocation);
        } else {
            SetAssetPackErrorStatus(assetPackErrorCode, assetPackInfo,
                                    "GameAssetManager: getAssetPackLocation");
        }
    }
}

void GameAssetManagerInternals::UpdateAssetPackFromDownloadState(AssetPackInfo *assetPackInfo,
        AssetPackDownloadState *downloadState) {
    AssetPackDownloadStatus downloadStatus = AssetPackDownloadState_getStatus(
            downloadState);

    switch (downloadStatus) {
        case ASSET_PACK_UNKNOWN:
            break;
        case ASSET_PACK_DOWNLOAD_PENDING:
            ChangeAssetPackStatus(assetPackInfo, GameAssetManager::GAMEASSET_DOWNLOADING);
            assetPackInfo->mAssetPackCompletion = 0.0f;
            break;
        case ASSET_PACK_DOWNLOADING: {
            ChangeAssetPackStatus(assetPackInfo, GameAssetManager::GAMEASSET_DOWNLOADING);
            uint64_t dlBytes = AssetPackDownloadState_getBytesDownloaded(downloadState);
            uint64_t totalBytes = AssetPackDownloadState_getTotalBytesToDownload(downloadState);
            double dlPercent = ((double) dlBytes) / ((double) totalBytes);
            assetPackInfo->mAssetPackCompletion = (float) dlPercent;
        }
            break;
        case ASSET_PACK_TRANSFERRING:
            break;
        case ASSET_PACK_DOWNLOAD_COMPLETED:
            UpdateAssetPackBecameAvailable(assetPackInfo);
            break;
        case ASSET_PACK_DOWNLOAD_FAILED:
            ChangeAssetPackStatus(assetPackInfo, GameAssetManager::GAMEASSET_ERROR);
            break;
        case ASSET_PACK_DOWNLOAD_CANCELED:
            ChangeAssetPackStatus(assetPackInfo, GameAssetManager::GAMEASSET_NEEDS_DOWNLOAD);
            assetPackInfo->mAssetPackCompletion = 0.0f;
            break;
        case ASSET_PACK_WAITING_FOR_WIFI:
            ChangeAssetPackStatus(assetPackInfo, GameAssetManager::GAMEASSET_NEEDS_MOBILE_AUTH);
            break;
        case ASSET_PACK_NOT_INSTALLED: {
            ChangeAssetPackStatus(assetPackInfo, GameAssetManager::GAMEASSET_NEEDS_DOWNLOAD);
            uint64_t totalBytes = AssetPackDownloadState_getTotalBytesToDownload(downloadState);
            if (totalBytes > 0) {
                assetPackInfo->mAssetPackDownloadSize = totalBytes;
            }
        }
            break;
        case ASSET_PACK_INFO_PENDING:
            break;
        case ASSET_PACK_INFO_FAILED:
            ChangeAssetPackStatus(assetPackInfo, GameAssetManager::GAMEASSET_ERROR);
            break;
        case ASSET_PACK_REMOVAL_PENDING:
            ChangeAssetPackStatus(assetPackInfo, GameAssetManager::GAMEASSET_PENDING_ACTION);
            break;
        case ASSET_PACK_REMOVAL_FAILED:
            ChangeAssetPackStatus(assetPackInfo, GameAssetManager::GAMEASSET_READY);
            assetPackInfo->mAssetPackCompletion = 1.0f;
            break;
        default:
            break;
    }
}

void GameAssetManagerInternals::SetAssetPackErrorStatus(
        const AssetPackErrorCode assetPackErrorCode,
        AssetPackInfo *assetPackInfo,
        const char *message) {
    switch (assetPackErrorCode) {
        case ASSET_PACK_NO_ERROR:
            // No error, so return immediately.
            return;
        case ASSET_PACK_APP_UNAVAILABLE:
            mAssetPackErrorMessage = "ASSET_PACK_APP_UNAVAILABLE";
            break;
        case ASSET_PACK_UNAVAILABLE:
            mAssetPackErrorMessage = "ASSET_PACK_UNAVAILABLE";
            break;
        case ASSET_PACK_INVALID_REQUEST:
            mAssetPackErrorMessage = "ASSET_PACK_INVALID_REQUEST";
            break;
        case ASSET_PACK_DOWNLOAD_NOT_FOUND:
            mAssetPackErrorMessage = "ASSET_PACK_DOWNLOAD_NOT_FOUND";
            break;
        case ASSET_PACK_API_NOT_AVAILABLE:
            mAssetPackErrorMessage = "ASSET_PACK_API_NOT_AVAILABLE";
            break;
        case ASSET_PACK_NETWORK_ERROR:
            mAssetPackErrorMessage = "ASSET_PACK_NETWORK_ERROR";
            break;
        case ASSET_PACK_ACCESS_DENIED:
            mAssetPackErrorMessage = "ASSET_PACK_ACCESS_DENIED";
            break;
        case ASSET_PACK_INSUFFICIENT_STORAGE:
            mAssetPackErrorMessage = "ASSET_PACK_INSUFFICIENT_STORAGE";
            break;
        case ASSET_PACK_PLAY_STORE_NOT_FOUND:
            mAssetPackErrorMessage = "ASSET_PACK_PLAY_STORE_NOT_FOUND";
            break;
        case ASSET_PACK_NETWORK_UNRESTRICTED:
            mAssetPackErrorMessage = "ASSET_PACK_NETWORK_UNRESTRICTED";
            break;
        case ASSET_PACK_INTERNAL_ERROR:
            mAssetPackErrorMessage = "ASSET_PACK_INTERNAL_ERROR";
            break;
        case ASSET_PACK_INITIALIZATION_NEEDED:
            mAssetPackErrorMessage = "ASSET_PACK_INITIALIZATION_NEEDED";
            break;
        case ASSET_PACK_INITIALIZATION_FAILED:
            mAssetPackErrorMessage = "ASSET_PACK_INITIALIZATION_FAILED";
            break;

        default:
            mAssetPackErrorMessage = "Unknown error code";
            break;
    }

    if (assetPackInfo == NULL) {
        ALOGE("%s failed with error code %d : %s", message,
             static_cast<int>(assetPackErrorCode), mAssetPackErrorMessage);
    } else {
        assetPackInfo->mAssetPackStatus = GameAssetManager::GAMEASSET_ERROR;
        ALOGE("%s failed on asset pack %s with error code %d : %s",
             message, assetPackInfo->mDefinition->mPackName,
             static_cast<int>(assetPackErrorCode),
             mAssetPackErrorMessage);
    }
}

void GameAssetManagerInternals::UpdateMobileDataRequestStatus() {
    if (mRequestingMobileDownload) {
        ShowCellularDataConfirmationStatus cellularStatus;
        AssetPackErrorCode assetPackErrorCode =
                AssetPackManager_getShowCellularDataConfirmationStatus(&cellularStatus);
        SetAssetPackErrorStatus(assetPackErrorCode, NULL,
                                "GameAssetManager: UpdateCellularRequestStatus");
        if (assetPackErrorCode == ASSET_PACK_NO_ERROR) {
            if (cellularStatus == ASSET_PACK_CONFIRM_USER_APPROVED) {
                mRequestingMobileDownload = false;
                ALOGI("GameAssetManager: User approved mobile data download");
            } else if (cellularStatus == ASSET_PACK_CONFIRM_USER_CANCELED) {
                mRequestingMobileDownload = false;
                ALOGI("GameAssetManager: User declined mobile data download");
            }
        }
    }
}
#endif // !NO_ASSET_PACKS

GameAssetManager::GameAssetManager(AAssetManager *assetManager, JavaVM *jvm,
                                   jobject android_context) {
    mInternals = new GameAssetManagerInternals(assetManager, jvm, android_context);
}

GameAssetManager::~GameAssetManager() {
    delete mInternals;
    mInternals = NULL;
}

void GameAssetManager::OnPause() {
    ALOGI("GameAssetManager onPause");
#if !defined(NO_ASSET_PACKS)
    AssetPackManager_onPause();
#endif
}

void GameAssetManager::OnResume() {
    ALOGI("GameAssetManager onResume");
#if !defined(NO_ASSET_PACKS)
    AssetPackManager_onResume();
#endif
}

const char *GameAssetManager::GetGameAssetErrorMessage() {
    return mInternals->GetAssetPackErrorMessage();
}

void GameAssetManager::UpdateGameAssetManager() {
#if !defined(NO_ASSET_PACKS)
    // Update the status outcome of any mobile data requests
    mInternals->UpdateMobileDataRequestStatus();

    // Update status of asset packs if necessary
    for (int i = 0; i < mInternals->GetAssetPackCount(); ++i) {
        AssetPackInfo *assetPackInfo = mInternals->GetAssetPack(i);
        if (assetPackInfo != NULL) {
            // If we are in an internal status where we want to query the asset pack
            // download state and update status accordingly, do so
            if (assetPackInfo->mAssetPackStatus ==
                GameAssetManager::GAMEASSET_WAITING_FOR_STATUS ||
                assetPackInfo->mAssetPackStatus == GameAssetManager::GAMEASSET_DOWNLOADING ||
                assetPackInfo->mAssetPackStatus == GameAssetManager::GAMEASSET_PENDING_ACTION ||
                assetPackInfo->mAssetPackStatus == GameAssetManager::GAMEASSET_NEEDS_MOBILE_AUTH) {
                const char *assetPackName = assetPackInfo->mDefinition->mPackName;
                AssetPackDownloadState *downloadState = NULL;
                AssetPackErrorCode assetPackErrorCode = AssetPackManager_getDownloadState(
                        assetPackName, &downloadState);

                if (assetPackErrorCode == ASSET_PACK_NO_ERROR) {
                    // Use the returned download state to update our asset pack info
                    mInternals->UpdateAssetPackFromDownloadState(assetPackInfo, downloadState);
                } else {
                    // If an error is reported, mark the asset pack as being in error and
                    // bail on the update process
                    mInternals->SetAssetPackErrorStatus(assetPackErrorCode, assetPackInfo,
                                                        "GameAssetManager: getDownloadState");
                    return;
                }

                AssetPackDownloadState_destroy(downloadState);

            }
        }
    }
#endif // !NO_ASSET_PACKS
}

uint64_t GameAssetManager::GetGameAssetSize(const char *assetName) {
    uint64_t assetSize = 0;

    if (assetName != NULL) {
#if defined NO_ASSET_PACKS
        assetSize = mInternals->GetInternalGameAssetSize(assetName);
#else
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
#endif
    }

    return assetSize;
}

bool
GameAssetManager::LoadGameAsset(const char *assetName, const size_t bufferSize, void *loadBuffer) {
    bool loadSuccess = false;

    if (assetName != NULL) {
#if defined NO_ASSET_PACKS
        loadSuccess = mInternals->LoadInternalGameAsset(assetName, bufferSize,
                                                        loadBuffer);
#else
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
#endif
    }

    return loadSuccess;
}

bool
GameAssetManager::LoadGameAssetAsync(const char *assetName, const size_t bufferSize,
                                     void *loadBuffer,
                                     LoadingCompleteCallback callback,
                                     void* userData) {
    bool startSuccess = false;

    if (assetName != NULL) {
        AssetPackInfo *packInfo = mInternals->GetAssetPackForAssetName(assetName);
        if (packInfo != NULL) {
            if (packInfo->mAssetPackStatus == GameAssetManager::GAMEASSET_READY) {
#if defined NO_ASSET_PACKS
                bool isInternal = true;
#else
                bool isInternal = false;
                switch (packInfo->mDefinition->mPackType) {
                    case GAMEASSET_PACKTYPE_INTERNAL:
                        isInternal = true;
                        break;
                    case GAMEASSET_PACKTYPE_FASTFOLLOW:
                    case GAMEASSET_PACKTYPE_ONDEMAND:
                        break;
                }
#endif
                mInternals->LoadGameAssetAsync(assetName, bufferSize, loadBuffer, callback,
                                               packInfo, isInternal, userData);
                startSuccess = true;
            }
        }
    }

    return startSuccess;
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

#if !defined NO_ASSET_PACKS
    if (assetPackName != NULL) {
        AssetPackInfo *packInfo = mInternals->GetAssetPackByName(assetPackName);
        if (packInfo != NULL) {
            assetPackType = packInfo->mDefinition->mPackType;
        }
    }
#endif

    return assetPackType;
}

void GameAssetManager::RequestMobileDataDownloads() {
#if !defined(NO_ASSET_PACKS)
    mInternals->RequestMobileDataDownloads();
#endif
}

bool GameAssetManager::RequestDownload(const char *assetPackName) {
    bool downloadStarted = false;
    ALOGI("GameAssetManager :: UI called RequestDownload %s", assetPackName);
#if !defined(NO_ASSET_PACKS)
    if (mInternals->GetAssetPackManagerInitialized()) {
        downloadStarted = mInternals->RequestAssetPackDownload(assetPackName);
    }
#endif
    return downloadStarted;
}

void GameAssetManager::RequestDownloadCancellation(const char *assetPackName) {
#if !defined(NO_ASSET_PACKS)
    mInternals->RequestAssetPackCancelDownload(assetPackName);
#endif
}

bool GameAssetManager::RequestRemoval(const char *assetPackName) {
#if !defined(NO_ASSET_PACKS)
    return mInternals->RequestAssetPackRemoval(assetPackName);
#else
    return false;
#endif
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
