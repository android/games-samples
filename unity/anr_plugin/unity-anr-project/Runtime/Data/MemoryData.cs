/*
 * Copyright 2024 Google LLC
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

﻿// ReSharper disable UnassignedField.Global

namespace com.google.unity.anr
{
    public struct MemoryData
    {
        /// <summary>
        /// The total memory accessible by the kernel in megabytes. This is basically the RAM size of the device, not including below-kernel fixed allocations like DMA buffers, RAM for the baseband CPU, etc.
        /// </summary>
        public string TotalMem;

        /// <summary>
        /// The available memory on the system in megabytes. This number should not be considered absolute: due to the nature of the kernel, a significant portion of this memory is actually in use and needed for the overall system to run well.
        /// </summary>
        public string AvailableMem;

        /// <summary>
        /// The threshold of availMem at which we consider memory to be low and start killing background services and other non-extraneous processes.
        /// </summary>
        public string Threshold;

        /// <summary>
        /// The <see cref="AvailableMem"/> divided by <see cref="TotalMem"/>.zzzz
        /// </summary>
        public string PercentAvailable;

        /// <summary>
        /// The maximum amount of memory that the virtual machine will attempt to use, measured in megabytes.
        /// </summary>
        public string MaxHeap;

        /// <summary>
        /// Returns the total amount of memory minus the amount of free memory in the Java virtual machine.
        /// </summary>
        public string UsedHeap;

        /// <summary>
        /// The <see cref="MaxHeap"/> minus <see cref="UsedHeap"/>.
        /// </summary>
        public string AvailableHeap;

        /// <summary>
        /// This represents the exact time when the memory snapshot was captured.
        /// </summary>
        public string Timestamp;
    }


    /// <summary>
    /// Wrapper class to store onTrimMemory information. More info <a href="https://developer.android.com/reference/android/content/ComponentCallbacks2#onTrimMemory(int)">ComponentCallbacks2</a>.
    /// </summary>
    public struct TrimMemoryData
    {
        /// <summary>
        /// The context of the trim, giving a hint of the amount of trimming the application may like to perform.
        /// </summary>
        public int Level;

        public LevelCode Code;

        /// <summary>
        /// Message generated from the Android documentation.
        /// </summary>
        public string Message;

        /// <summary>
        /// This represents the exact time when the memory event was triggered.
        /// </summary>
        public string Timestamp;

        public enum LevelCode
        {
            TRIM_MEMORY_COMPLETE = 80,
            TRIM_MEMORY_MODERATE = 60,
            TRIM_MEMORY_BACKGROUND = 40,
            TRIM_MEMORY_UI_HIDDEN = 20,
            TRIM_MEMORY_RUNNING_CRITICAL = 15,
            TRIM_MEMORY_RUNNING_LOW = 10,
            TRIM_MEMORY_RUNNING_MODERATE = 5
        }
    }
}
