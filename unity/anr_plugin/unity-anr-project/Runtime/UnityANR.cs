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

﻿using System.Threading;
using System.Threading.Tasks;
using UnityEngine;

namespace com.google.unity.anr
{
    public class UnityANR : MonoBehaviour
    {
        [SerializeField] private int memoryStatsInterval;
        [SerializeField] private bool enableStrictMode;
        [SerializeField] private bool shouldSetThreadName;
        [SerializeField] private bool logEnabled;

        public const string UnityGameThread = "UnityGame";
        private CancellationTokenSource _tokenSource;
        private Task _memoryTask;

        private void Awake()
        {
            Logger.logEnabled = logEnabled;
            DontDestroyOnLoad(gameObject);
            if (shouldSetThreadName && string.IsNullOrEmpty(Thread.CurrentThread.Name))
            {
                Thread.CurrentThread.Name = UnityGameThread;
            }

            PluginBinder.InitPlugin(gameObject.name);
            PluginBinder.EnableLogging(logEnabled);
            if (enableStrictMode)
                PluginBinder.EnableStrictMode();
        }

        private void Start()
        {
            PluginBinder.StartMemoryStats(memoryStatsInterval);
            _tokenSource = new CancellationTokenSource();
            _memoryTask = new Task(GetLastMemoryInfo, _tokenSource.Token);
            _memoryTask.Start();
        }

        private async void GetLastMemoryInfo()
        {
            if (AndroidJNI.AttachCurrentThread() != 0)
            {
                Logger.LogError("Unable to attach the current thread to a Java (Dalvik) VM");
                return;
            }

            while (!_tokenSource.IsCancellationRequested)
            {
                Logger.Log("GetLastMemoryInfo");
                PluginBinder.GetMemoryInfo();

                Logger.Log("Wait to GetLastMemoryInfo");
                await Task.Delay(memoryStatsInterval * 1000, _tokenSource.Token);
            }

            AndroidJNI.DetachCurrentThread();
        }

        private void OnDestroy()
        {
            if (_tokenSource.IsCancellationRequested) return;
            _tokenSource.Cancel();
            _memoryTask?.Dispose();
            _memoryTask = null;
        }

        #region Called by the Java Plugin

        public void OnAppExitInfoChecked(string exitInfo)
        {
            if (string.IsNullOrEmpty(exitInfo))
            {
                Logger.Log($"No abnormal App Exit Reason found");
                return;
            }

            // Create a task to avoid running parsing on UnityMain Thread
            Task.Run(() =>
            {
                Logger.Log($"OnAppExitInfoChecked:\n{exitInfo}");

                var appExitInfo = JsonUtility.FromJson<AppExitInfoList>(exitInfo);
                PluginEvents.ApplicationExitInfoFetched(this, appExitInfo.entries);
            });
        }

        private void OnMemoryInfoUpdated(string memoryData)
        {
            // Create a task to avoid running parsing on UnityMain Thread
            Task.Run(() =>
            {
                var memData = JsonUtility.FromJson<MemoryData>(memoryData);
                Logger.Log($"Memory Available: {memData.PercentAvailable}%");
                PluginEvents.MemoryInfoUpdated(this, memData);
            });
        }

        public void OnTrimMemory(string trimMemoryData)
        {
            // Create a task to avoid running parsing on UnityMain Thread
            Task.Run(() =>
            {
                var memData = JsonUtility.FromJson<TrimMemoryData>(trimMemoryData);
                memData.Code = (TrimMemoryData.LevelCode)memData.Level;

                Logger.Log($"onTrimMemory - Level {memData.Level} - {memData.Message}");

                //Send event on Thread Pool worker
                PluginEvents.LowMemoryReceived(this, memData);
            });
        }

        public void OnProcessingCompleted(string totalTime)
        {
            Logger.Log($"OnProcessingCompleted: {totalTime}");
            PluginEvents.ProcessCompleted(this, totalTime);
        }

        #endregion
    }
}
