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

#ifndef agdktunnel_memory_consumer_hpp
#define agdktunnel_memory_consumer_hpp

#include <cstdint>

class MemoryConsumerPool;
class TextRenderer;

class MemoryConsumer
{
public:
    enum MemoryPools
    {
        // The general memory pool occasionally allocs until the first
        // critical memory warning is received
        MEMORY_POOL_GENERAL = 0,
        // The cache memory pool occasionally allocs, purging if a critical
        // memory warning is received
        MEMORY_POOL_CACHE,
        MEMORY_POOL_COUNT
    };

    MemoryConsumer(bool startAsActive);

    ~MemoryConsumer();

    // Set whether continual memory consumption is active or not
    void SetActive(bool active) { mActive = active; }

    // Called from game frame, periodically makes new allocations if active
    void Update();

    // Return the number of times memory advice has reported a critical memory warning status
    uint32_t GetCriticalMemoryWarningCount() const { return mCriticalMemoryWarningCount; }

    // Return the memory state most recently reported by the memory advice library,
    // this value can be cast to MemoryAdvice_MemoryState
    int32_t GetLastReportedMemoryState() const { return mLastReportedMemoryState; }

    // Returns total number of bytes allocated in a specified pool
    size_t GetTotalPoolAllocatedBytes(const MemoryPools memoryPool) const;

    // Update the current memory state (casts to MemoryAdvice_MemoryState)
    void SetMemoryState(const int32_t memoryState);

    // Render memory statistics to the screen, using the provided TextRenderer
    void RenderMemoryStatistics(TextRenderer *textRenderer) const;

private:
    // Check if a pool is due to for a new memory allocation, and make
    // an allocation if it is.
    void UpdatePoolAllocations(const MemoryPools memoryPool, const uint64_t currentTimestamp);

    // Whether memory consumption/statistics display is active
    bool mActive;

    // Number of critical memory warnings received
    uint32_t mCriticalMemoryWarningCount;

    // Last reported memory state (casts to MemoryAdvice_MemoryState)
    int32_t mLastReportedMemoryState;

    // Last reported percentage memory available returned by Memory Advice
    float mLastReportedMemoryPercentage;

    // Previous timestamp, in milliseconds, of the most recent call
    // of MemoryAdvice_getPercentageAvailableMemory
    uint64_t mPercentageAvailableCheckTimestamp;

    // Elapsed time in milliseconds between allocations for each memory pool
    uint64_t mNextAllocationDeltaTime[MemoryConsumer::MEMORY_POOL_COUNT];

    // Memory allocation pools
    MemoryConsumerPool *mPools[MemoryConsumer::MEMORY_POOL_COUNT];
};

#endif
