/*
 * Copyright 2022 The Android Open Source Project
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

#include "common.hpp"
#include "game_consts.hpp"
#include "memory_consumer.hpp"
#include "memory_advice/memory_advice.h"
#include "text_renderer.hpp"
#include <mutex>
#include <vector>

// Delay between periodic memory watcher status updates (milliseconds)
static const uint64_t MEMORY_CALLBACK_PERIOD_MS = (5*1000);

// Text color constants
static const float RED_TEXT_COLOR[3] = {1.0f, 0.0f, 0.0f};
static const float YELLOW_TEXT_COLOR[3] = {1.0f, 1.0f, 0.0f};
static const float GREEN_TEXT_COLOR[3] = {0.0f, 1.0f, 0.0f};

// Strings and colors mapping to the MemoryAdvice_MemoryState enum
static const char *MEMORYSTATE_STRINGS[4] = {
    "UNK ", // MEMORYADVICE_STATE_UNKNOWN
    " OK ", // MEMORYADVICE_STATE_OK
    "WARN", // MEMORYADVICE_STATE_APPROACHING_LIMIT
    "CRIT"  // MEMORYADVICE_STATE_CRITICAL
};

static const float *MEMORYSTATE_COLORS[4] = {
    YELLOW_TEXT_COLOR,  // MEMORYADVICE_STATE_UNKNOWN
    GREEN_TEXT_COLOR,   // MEMORYADVICE_STATE_OK
    YELLOW_TEXT_COLOR,  // MEMORYADVICE_STATE_APPROACHING_LIMIT
    RED_TEXT_COLOR      // MEMORYADVICE_STATE_CRITICAL
};

// Define some buckets of constants to randomly pick from
// for allocation sizes and delays between allocs

// This constant is assumed to be a power of two during random number selection
static const size_t BUCKET_COUNT = 8;

static const size_t ALLOC_SIZES[BUCKET_COUNT] = {
    (64*1024),
    (128*1024),
    (256*1024),
    (384*1024),
    (512*1024),
    (768*1024),
    (1024*1024),
    (2048*1024)
};

// Milliseconds to delay before next alloc
static const uint64_t ALLOC_DELTA_TIME[BUCKET_COUNT] = {
    10,
    20,
    30,
    40,
    50,
    75,
    100,
    200
};

// Generate a current timestamp, milliseconds since epoch
static uint64_t GetAllocationTimestampMS() {
    const auto timestamp =
            std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count();
    uint64_t timestampMS = (static_cast<uint64_t>(timestamp)) / 1000;
    return timestampMS;
}

// Very basic structure and class to track pools of memory allocations, only aggregate
// allocation count and size, and complete drain of all pool allocations are supported
struct MemoryPoolAllocation {
    MemoryPoolAllocation(void *ptr, const size_t size) {
        mPtr = ptr;
        mSize = size;
    }

    void *mPtr;
    size_t mSize;
};

class MemoryConsumerPool {
public:
    MemoryConsumerPool() {
        mAllocationCount = 0;
        mLastAllocationTimestamp = 0;
        mTotalBytesAllocated = 0;
        mAllocations.reserve(4096);
    }

    ~MemoryConsumerPool() {
        Purge();
    }

    void Purge() {
        std::lock_guard<std::mutex> poolLock(mPoolMutex);
        for (auto iter = mAllocations.begin(); iter != mAllocations.end(); ++iter) {
            free(iter->mPtr);
        }
        mAllocations.clear();
        mTotalBytesAllocated = 0;
        mAllocationCount = 0;
    }

    uint64_t GetTotalBytesAllocated() const {
        return mTotalBytesAllocated;
    }

    uint64_t GetTotalAllocationCount() const {
        return mAllocationCount;
    }

    uint64_t GetLastAllocationTimestamp() const {
        return mLastAllocationTimestamp;
    }

    // Ownership of the pointer passed in ptr will
    // belong to the pool
    void AddAllocation(void *ptr, const size_t size) {
        std::lock_guard<std::mutex> poolLock(mPoolMutex);
        mAllocations.push_back({ptr, size});
        mTotalBytesAllocated += size;
        ++mAllocationCount;
        mLastAllocationTimestamp = GetAllocationTimestampMS();
    }

private:
    uint64_t mAllocationCount;
    uint64_t mLastAllocationTimestamp;
    size_t mTotalBytesAllocated;
    std::vector<MemoryPoolAllocation> mAllocations;
    std::mutex mPoolMutex;
};

// Callback for Memory Advice
static void MemoryWatcherCallback(MemoryAdvice_MemoryState state, void *user_data) {
    MemoryConsumer *consumer = reinterpret_cast<MemoryConsumer *>(user_data);
    consumer->SetMemoryState(static_cast<int32_t>(state));
}

MemoryConsumer::MemoryConsumer(bool startAsActive) {
    mActive = startAsActive;
    mCriticalMemoryWarningCount = 0;
    mLastReportedMemoryPercentage = MemoryAdvice_getPercentageAvailableMemory();
    mLastReportedMemoryState = MemoryAdvice_getMemoryState();
    mPercentageAvailableCheckTimestamp = GetAllocationTimestampMS();
    for (int i = 0; i < MEMORY_POOL_COUNT; ++i) {
        mNextAllocationDeltaTime[i] = 0;
        mPools[i] = new MemoryConsumerPool();
    }
    MemoryAdvice_ErrorCode error = MemoryAdvice_registerWatcher(MEMORY_CALLBACK_PERIOD_MS,
                                                                MemoryWatcherCallback, this);
    if (error != MEMORYADVICE_ERROR_OK) {
        ALOGE("Error registering memory watcher: %d", (int)error);
    }
}

MemoryConsumer::~MemoryConsumer() {
    MemoryAdvice_ErrorCode error = MemoryAdvice_unregisterWatcher(MemoryWatcherCallback);
    if (error != MEMORYADVICE_ERROR_OK) {
        ALOGE("Error unregistering memory watcher: %d", (int)error);
    }
    for (int i = 0; i < MEMORY_POOL_COUNT; ++i) {
        delete mPools[i];
    }
}

void MemoryConsumer::Update() {
    if (mActive) {
        const uint64_t currentTimestamp = GetAllocationTimestampMS();
        // Only alloc into the general pool until we get a critical memory warning
        if (mCriticalMemoryWarningCount == 0) {
            UpdatePoolAllocations(MEMORY_POOL_GENERAL, currentTimestamp);
        }
        // Always update the cache pool
        UpdatePoolAllocations(MEMORY_POOL_CACHE, currentTimestamp);

        // Check if it is time to update the percentage available estimate
        if (currentTimestamp - mPercentageAvailableCheckTimestamp > MEMORY_CALLBACK_PERIOD_MS) {
            mPercentageAvailableCheckTimestamp = currentTimestamp;
            mLastReportedMemoryPercentage = MemoryAdvice_getPercentageAvailableMemory();
            // Refresh our memory state when we call the percentage check, in case
            // our memory state was upgraded after a purge
            SetMemoryState(MemoryAdvice_getMemoryState());
        }
    }
}

// Check if a memory pool is due for another allocation, attempt one if it
// is and reset its delay timer
void MemoryConsumer::UpdatePoolAllocations(const MemoryPools memoryPool,
                                           const uint64_t currentTimestamp) {
    const uint64_t deltaTime = currentTimestamp -
                               mPools[memoryPool]->GetLastAllocationTimestamp();
    if (deltaTime > mNextAllocationDeltaTime[memoryPool]) {
        // Pick a size using a random index into a predefined bucket
        // of allocation sizes
        const int sizeBucketIndex = rand() & (BUCKET_COUNT - 1);
        const size_t allocSize = ALLOC_SIZES[sizeBucketIndex];
        void *allocPtr = malloc(allocSize);
        memset(allocPtr, 0xBA, allocSize);
        if (allocPtr == NULL) {
            ALOGE("Failed to alloc %zu bytes for pool %d", allocSize,
                  static_cast<int>(memoryPool));
        } else {
            mPools[memoryPool]->AddAllocation(allocPtr, allocSize);
        }
        // Pick a delay until the next alloc using a random index into
        // a predefined bucket of delay intervals
        const int delayBucketIndex = rand() & (BUCKET_COUNT - 1);
        mNextAllocationDeltaTime[memoryPool] = ALLOC_DELTA_TIME[delayBucketIndex];
    }
}

size_t MemoryConsumer::GetTotalPoolAllocatedBytes(const MemoryPools memoryPool) const {
    return mPools[memoryPool]->GetTotalBytesAllocated();
}

void MemoryConsumer::SetMemoryState(const int32_t memoryState) {
    if (memoryState >= 0 && memoryState <= MEMORYADVICE_STATE_CRITICAL) {
        if (memoryState != mLastReportedMemoryState) {
            if (memoryState == MEMORYADVICE_STATE_CRITICAL) {
                ++mCriticalMemoryWarningCount;
                // Purge the cache pool on a critical memory warning
                mPools[MEMORY_POOL_CACHE]->Purge();
            }
            mLastReportedMemoryState = memoryState;
        }
    }
}

// Render memory statistics to the screen, using the provided TextRenderer
void MemoryConsumer::RenderMemoryStatistics(TextRenderer *textRenderer) const {
    if (mActive) {
        const size_t generalPoolMB = GetTotalPoolAllocatedBytes(MEMORY_POOL_GENERAL) /
                                     (1024 * 1024);
        const size_t cachePoolMB = GetTotalPoolAllocatedBytes(MEMORY_POOL_CACHE) /
                                   (1024 * 1024);
        const int percentAvailable = static_cast<int>(mLastReportedMemoryPercentage);
        const char *statusString = MEMORYSTATE_STRINGS[mLastReportedMemoryState];
        const float *statusColor = MEMORYSTATE_COLORS[mLastReportedMemoryState];
        char memoryString[64];
        snprintf(memoryString, 63, "%s (%d) %d%% G: %zu MB C: %zu MB",
                 statusString, mCriticalMemoryWarningCount, percentAvailable,
                 generalPoolMB, cachePoolMB);

        textRenderer->SetColor(statusColor);
        textRenderer->SetFontScale(MEMORY_FONT_SCALE);
        textRenderer->RenderText(memoryString, MEMORY_POS_X, MEMORY_POS_Y);
        textRenderer->ResetColor();
    }
}
