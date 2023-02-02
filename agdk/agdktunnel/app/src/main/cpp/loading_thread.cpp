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
#include <sys/stat.h>

#include "common.hpp"
#include "loading_thread.hpp"

namespace {

}

LoadingThread::LoadingThread(AAssetManager *assetManager) {
    mAssetManager = assetManager;
    LaunchThread();
}

LoadingThread::~LoadingThread() {
    std::lock_guard<std::mutex> threadLock(mThreadMutex);
    TerminateThread();
}

void
LoadingThread::StartAssetLoad(const char *assetName, const char *assetPath,
                              const size_t bufferSize, void *loadBuffer,
                              LoadingCompleteCallback callback, bool useAssetManager,
                              void* userData) {
    std::lock_guard<std::mutex> workLock(mWorkMutex);
    LoadingJob *loadingJob = new LoadingJob();
    loadingJob->assetName = assetName;
    loadingJob->assetPath = assetPath;
    loadingJob->bufferSize = bufferSize;
    loadingJob->loadBuffer = loadBuffer;
    loadingJob->callback = callback;
    loadingJob->useAssetManager = useAssetManager;
    loadingJob->userData = userData;
    mWorkQueue.emplace(loadingJob);
    mWorkCondition.notify_all();
}

void LoadingThread::LaunchThread() {
    std::lock_guard<std::mutex> threadLock(mThreadMutex);
    if (mThread.joinable()) {
        TerminateThread();
    }
    mThread = std::thread([this]() { ThreadMain(); });
}

void LoadingThread::TerminateThread() REQUIRES(mThreadMutex) {
    {
        std::lock_guard<std::mutex> workLock(mWorkMutex);
        mIsActive = false;
        mWorkCondition.notify_all();
    }
    mThread.join();
}

void LoadingThread::ThreadMain() {
    pthread_setname_np(pthread_self(), "LoadingThread");

    std::lock_guard<std::mutex> lock(mWorkMutex);
    while (mIsActive) {
        mWorkCondition.wait(
                mWorkMutex,
                [this]() REQUIRES(mWorkMutex) {
                    return !mWorkQueue.empty() || !mIsActive;
                });
        if (!mWorkQueue.empty()) {
            LoadingJob *loadingJob = mWorkQueue.front();
            mWorkQueue.pop();

            // Drop the mutex while we execute
            mWorkMutex.unlock();

            LoadingCompleteMessage loadingCompleteMessage;
            loadingCompleteMessage.assetName = loadingJob->assetName;
            loadingCompleteMessage.bytesRead = 0;
            loadingCompleteMessage.loadBuffer = loadingJob->loadBuffer;
            loadingCompleteMessage.loadSuccessful = false;
            loadingCompleteMessage.userData = loadingJob->userData;

            if (loadingJob->useAssetManager) {
                AAsset *asset = AAssetManager_open(mAssetManager, loadingJob->assetName,
                                                   AASSET_MODE_STREAMING);
                if (asset != NULL) {
                    size_t assetSize = AAsset_getLength(asset);
                    if (assetSize <= loadingJob->bufferSize) {
                        AAsset_read(asset, loadingJob->loadBuffer, assetSize);
                        loadingCompleteMessage.bytesRead = assetSize;
                        loadingCompleteMessage.loadSuccessful = true;
                    }
                    AAsset_close(asset);
                }
            } else {
                FILE *fp = fopen(loadingJob->assetPath, "rb");
                if (fp != NULL) {
                    struct stat fileStats;
                    size_t assetSize = 0;
                    int statResult = fstat(fileno(fp), &fileStats);
                    if (statResult == 0) {
                        assetSize = fileStats.st_size;
                        if (assetSize <= loadingJob->bufferSize) {
                            fread(loadingJob->loadBuffer, assetSize, 1, fp);
                            loadingCompleteMessage.bytesRead = assetSize;
                            loadingCompleteMessage.loadSuccessful = true;
                        }
                    }
                    fclose(fp);
                }
                delete[] loadingJob->assetPath;
            }

            loadingJob->callback(&loadingCompleteMessage);
            delete loadingJob;

            mWorkMutex.lock();
        }
    }
}
