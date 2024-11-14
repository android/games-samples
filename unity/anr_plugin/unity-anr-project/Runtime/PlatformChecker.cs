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
using UnityEngine;

namespace com.google.unity.anr
{
    public class PlatformChecker
    {
        private static bool? _isChromebook = null;
        private static bool? _isPlayGamesPC = null;

        public static bool IsChromebook
        {
            get
            {
                Logger.Log($"#PC# Checking platform");
                if (_isChromebook.HasValue)
                {
                    Logger.Log($"#PC# Already checked {_isChromebook.Value}");
                    return _isChromebook.Value;
                }
#if UNITY_ANDROID && !UNITY_EDITOR

                try
                {
                    Logger.Log($"#PC# Checking native code");
                    _isChromebook = PluginBinder.MainJavaObject.Call<bool>("isChromebook");
                }
                catch (System.Exception e)
                {
                    Logger.LogError(e.ToString());
                    _isChromebook = false;
                }
#else
                    _isChromebook = false;
#endif

                return _isChromebook.Value;
            }
        }


        public static bool IsPlayGamesPC
        {
            get
            {
                Logger.Log($"#PC# Checking platform");
                if (_isPlayGamesPC.HasValue)
                {
                    Logger.Log($"#PC# Already checked {_isPlayGamesPC.Value}");
                    return _isPlayGamesPC.Value;
                }
#if UNITY_ANDROID && !UNITY_EDITOR
                try
                {
                    Logger.Log($"#PC# Checking native code");
                    _isPlayGamesPC = PluginBinder.MainJavaObject.Call<bool>("isPlayGamesPC");
                }
                catch (System.Exception e)
                {
                    Logger.LogError(e.ToString());
                    _isPlayGamesPC = false;
                }
#else
                _isPlayGamesPC = false;
#endif

                return _isPlayGamesPC.Value;
            }
        }
    }
}
