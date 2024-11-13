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
 */﻿

using System;
using UnityEngine;

namespace com.google.unity.anr
{
    public static class PluginBinder
    {
#if UNITY_ANDROID && !UNITY_EDITOR
        private static AndroidJavaClass _androidUnityActivity;
        public static AndroidJavaObject Context;
        public static AndroidJavaObject MainJavaObject;
        public const string PackageName = "com.google.unity.plugin";

        public static bool IsInitialized { get; private set; }

        public static void InitPlugin(string gameObjectName)
        {
            Context ??= GetUnityActivity().Call<AndroidJavaObject>("getApplicationContext");
            MainJavaObject ??= new AndroidJavaObject($"{PackageName}.UnityDebugHelper");
            MainJavaObject.Call("init", Context, gameObjectName);
            IsInitialized = true;
        }

        public static void ForceThreadSleep(bool isUnityThread, float milliseconds)
        {
            MainJavaObject.Call(isUnityThread ? "threadSleep" : "threadSleepMain", milliseconds);
        }

        public static void ForceCrash()
        {
            MainJavaObject.Call("forceCrash");
        }

        public static void ForceNativeCrash()
        {
            MainJavaObject.Call("forceNativeCrash");
        }

        public static void ForceApplicationNotResponding()
        {
            MainJavaObject.Call("forceANR");
        }

        public static void CheckAppExitInfo()
        {
            MainJavaObject.Call("checkForExitInfo");
        }

        public static void GetMemoryInfo(bool forced)
        {
            MainJavaObject.Call("getMemoryInfo", forced);
        }

        public static void StartMemoryStats(int memoryStatsInterval)
        {
            MainJavaObject.Call("startMemoryStats", memoryStatsInterval);
        }

        public static string GetWebViewInfo()
        {
            return MainJavaObject.Call<string>("getWebViewVersion");
        }

        public static void EnableStrictMode()
        {
            MainJavaObject.Call("enableStrictMode");
        }

        public static void DisableStrictMode()
        {
            MainJavaObject.Call("disableStrictMode");
        }

        public static void EnableLogging(bool enable)
        {
            MainJavaObject.Call("enableLogging", enable);
        }

        private static AndroidJavaObject GetUnityActivity()
        {
            _androidUnityActivity ??= new AndroidJavaClass("com.unity3d.player.UnityPlayer");
            return _androidUnityActivity.GetStatic<AndroidJavaObject>("currentActivity");
        }

        private static AndroidJavaObject _androidUnityObj;
        public static void Sleep()
        {
            _androidUnityObj ??= new AndroidJavaObject("com.unity3d.player.UnityPlayer");
            _androidUnityObj.Call("Sleep", 4000);
        }
#else
        /// <summary>
        /// Initializes the Plugin with the GameObject name that will receive the messages from the Java
        /// </summary>
        /// <param name="name"></param>
        public static void InitPlugin(string name)
        {
        }

        /// <summary>
        /// Forces the MainThread or the Unity Thread to sleep
        /// </summary>
        /// <param name="isUnityThread">Block the Unity Thread</param>
        /// <param name="milliseconds">Sleep duration</param>
        public static void ForceThreadSleep(bool isUnityThread, float milliseconds)
        {
        }

        /// <summary>
        /// Force a crash in the Java code
        /// </summary>
        public static void ForceCrash()
        {
        }

        /// <summary>
        /// Force a crash in the Java code on the MainThread
        /// </summary>
        public static void ForceNativeCrash()
        {
        }

        /// <summary>
        /// Calls the ActivityManager.appNotResponding(REASON)
        /// </summary>
        public static void ForceApplicationNotResponding()
        {
        }

        /// <summary>
        /// Starts the processing for fetching ApplicationExitInfo
        /// </summary>
        public static void CheckAppExitInfo()
        {
        }

        /// <summary>
        /// Retrieve latest MemoryInfo from the Plugin
        /// </summary>
        public static void GetMemoryInfo(bool forced)
        {
        }

        /// <summary>
        /// Retrieve WebView apk verison
        /// </summary>
        /// <returns></returns>
        public static String GetWebViewInfo()
        {
            return "0";
        }

        /// <summary>
        /// Start the process of checking memory on an interval
        /// </summary>
        /// <param name="memoryStatsInterval"></param>
        public static void StartMemoryStats(int memoryStatsInterval)
        {
        }

        /// <summary>
        /// Calls the StrictMode.setThreadPolicy
        /// </summary>
        public static void EnableStrictMode()
        {
        }

        /// <summary>
        /// Disable all StrictMode.setThreadPolicy
        /// </summary>
        public static void DisableStrictMode()
        {
        }

        
        public static void EnableLogging(bool enable)
        {

        }

        public static void Sleep()
        {
        }
#endif
    }
}
