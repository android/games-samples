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

﻿using Debug = UnityEngine.Debug;

namespace com.google.unity.anr
{
    public static class Logger
    {
        public static bool logEnabled;


        public static void Log(string message)
        {
            if (!logEnabled)
                return;

            Debug.Log(message);
        }

        public static void LogWarning(string message)
        {
            if (!logEnabled)
                return;

            Debug.LogWarning(message);
        }

        public static void LogError(string message)
        {
            if (!logEnabled)
                return;

            Debug.LogError(message);
        }
    }
}
