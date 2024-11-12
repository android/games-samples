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
﻿
using System;

namespace com.google.unity.anr
{
    public abstract class PluginEvents
    {
        public static event EventHandler<string> OnOperationCompleted;

        public static event EventHandler<AppExitInfoData[]> OnApplicationExitInfoFetched;

        public static event EventHandler<MemoryData> OnMemoryStatsUpdated;

        public static event EventHandler<TrimMemoryData> OnLowMemoryReceived;

        internal static void ProcessCompleted(object sender, string time)
        {
            OnOperationCompleted?.Invoke(sender, time);
        }

        internal static void ApplicationExitInfoFetched(object sender, AppExitInfoData[] appExitInfo)
        {
            OnApplicationExitInfoFetched?.Invoke(sender, appExitInfo);
        }

        internal static void MemoryInfoUpdated(object sender, MemoryData data)
        {
            OnMemoryStatsUpdated?.Invoke(sender, data);
        }

        internal static void LowMemoryReceived(object sender, TrimMemoryData trimMemoryData)
        {
            OnLowMemoryReceived?.Invoke(sender, trimMemoryData);
        }
    }
}
