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

#ifndef agdktunnel_loading_thread_hpp
#define agdktunnel_loading_thread_hpp

#include <condition_variable>
#include <mutex>
#include <queue>
#include <stddef.h>
#include <thread>

struct AAssetManager;

// Enable thread safety attributes only with clang.
// The attributes can be safely erased when compiling with other compilers.
#if defined(__clang__) && (!defined(SWIG))
#define THREAD_ANNOTATION_ATTRIBUTE__(x)   __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE__(x)   // no-op
#endif

#define GUARDED_BY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))

#define REQUIRES(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(requires_capability(__VA_ARGS__))

struct LoadingCompleteMessage {
    const char *assetName;
    size_t bytesRead;
    void *loadBuffer;
    bool loadSuccessful;
    void* userData; // Opaque pointer to data owned by the load requester.
};

typedef void (*LoadingCompleteCallback)(const LoadingCompleteMessage *message);

class LoadingThread {
public:
    LoadingThread(AAssetManager *assetManager);

    ~LoadingThread();

    // userData is passed without modification to the callback.
    void StartAssetLoad(const char *assetName, const char *assetPath, const size_t bufferSize,
                        void *loadBuffer, LoadingCompleteCallback callback, bool useAssetManager,
                        void* userData);

private:
    void LaunchThread();

    void TerminateThread() REQUIRES(mThreadMutex);

    void ThreadMain();

    AAssetManager *mAssetManager;

    struct LoadingJob {
        const char *assetName;
        const char *assetPath;
        size_t bufferSize;
        void *loadBuffer;
        LoadingCompleteCallback callback;
        bool useAssetManager;
        void* userData; // Opaque pointer to data owned by the load requester.
    };

    std::mutex mThreadMutex;
    std::thread mThread GUARDED_BY(mThreadMutex);

    std::mutex mWorkMutex;
    bool mIsActive GUARDED_BY(mWorkMutex) = true;
    std::queue<LoadingJob *> mWorkQueue GUARDED_BY(mWorkMutex);
    std::condition_variable_any mWorkCondition;
};

#endif
