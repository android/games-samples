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

#ifndef agdktunnel_gameassetmanager_hpp
#define agdktunnel_gameassetmanager_hpp

#include <jni.h>
#include <stddef.h>
#include "loading_thread.hpp"
#include "util.hpp"

class GameAssetManagerInternals;

struct AAssetManager;

static const char *INSTALL_ASSETPACK_NAME = "InstallPack";
static const char *FASTFOLLOW_ASSETPACK_NAME = "FastFollowPack";
static const char *ONDEMAND_ASSETPACK_NAME = "OnDemandPack";

class GameAssetManager {
public:

    enum GameAssetPackType {
        // This asset pack type is always available and included in the application package
        GAMEASSET_PACKTYPE_INTERNAL = 0,
        // This asset pack type is downloaded separately but is an automatic download after
        // app installation
        GAMEASSET_PACKTYPE_FASTFOLLOW,
        // This asset pack type is only downloaded when specifically requested
        GAMEASSET_PACKTYPE_ONDEMAND
    };

    enum GameAssetStatus {
        // The named asset pack was not recognized as a valid asset pack name
        GAMEASSET_NOT_FOUND = 0,

        // The asset pack is waiting for information about its status to become available
        GAMEASSET_WAITING_FOR_STATUS,

        // The asset pack needs to be downloaded to the device
        GAMEASSET_NEEDS_DOWNLOAD,

        // The asset pack is large enough to require explicit authorization to download
        // over a mobile data connection as wi-fi is currently unavailable
        GAMEASSET_NEEDS_MOBILE_AUTH,

        // The asset pack is in the process of downloading to the device
        GAMEASSET_DOWNLOADING,

        // The asset pack is ready to be used
        GAMEASSET_READY,

        // The asset pack is pending the results of a request for download cancellation,
        // deletion or cellular download authorization
        GAMEASSET_PENDING_ACTION,

        // The asset pack is in an error state and cannot be used or downloaded
        GAMEASSET_ERROR
    };

    GameAssetManager(AAssetManager *assetManager, JavaVM *jvm, jobject android_context);

    ~GameAssetManager();

    // Event handlers for pause and resume events
    void OnPause();

    void OnResume();

    // Call once every game frame to update internal asset states
    void UpdateGameAssetManager();

    // If an asset was set to the GAMEASSET_ERROR status, this will return
    // an error message describing the reason for the error
    const char *GetGameAssetErrorMessage();

    // Return the name of the asset pack that contains the specified asset file,
    // returns NULL if no parent pack could be found.
    const char *GetGameAssetParentPackName(const char *assetName);

    // If the status of the asset is GAMEASSET_READY, return the size in bytes
    // If the status anything else, 0 will be returned
    uint64_t GetGameAssetSize(const char *assetName);

    // If the status of the asset is GAMEASSET_READY, load file data into the specified buffer,
    // returns true if successful
    bool LoadGameAsset(const char *assetName, const size_t bufferSize, void *loadBuffer);

    // If the status of the asset is GAMEASSET_READY, start asynchronously loading
    // file data into the specified buffer. Callback will be called when load completes.
    // returns true if async load began successfully.
    // userData is passed without modification to the callback.
    bool LoadGameAssetAsync(const char *assetName, const size_t bufferSize, void *loadBuffer,
                            LoadingCompleteCallback callback, void* userData);

    // Returns an array of filenames of files present in the specified asset pack,
    // returns NULL if the asset pack name was not found
    const char **GetGameAssetPackFileList(const char *assetPackName, int *fileListSize);

    // Returns the status of an asset pack
    GameAssetStatus GetGameAssetPackStatus(const char *assetPackName);

    // Returns the type of an asset pack
    GameAssetPackType GetGameAssetPackType(const char *assetPackName);

    // Request permission to download large asset packs over a mobile data connection
    void RequestMobileDataDownloads();

    // Starts the download process for the specified asset pack, only
    // fast-follow and on-demand packs can be downloaded,
    // returns true if the download begins successfully
    bool RequestDownload(const char *assetPackName);

    // Requests a cancellation of the download process for the specified asset pack,
    // is only a request, not a guarantee, will have no impact on an asset pack with
    // a status other than GAMEASSET_DOWNLOADING
    void RequestDownloadCancellation(const char *assetPackName);

    // Requests removal  specified asset pack, only fast-follow and on-demand packs
    // can be removed, removal is a request and not guaranteed. Returns false if an
    // error was reported
    bool RequestRemoval(const char *assetPackName);

    // Returns the status of an asset pack file, including progress
    // information if the asset pack file is in the process of being downloaded
    GameAssetStatus GetDownloadStatus(const char *assetPackName,
                                      float *completionProgress, uint64_t *totalPackSize);

private:
    GameAssetManagerInternals *mInternals;
};

#endif
