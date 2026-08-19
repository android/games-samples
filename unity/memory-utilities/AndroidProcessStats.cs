/*
 * Copyright (C) 2026 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

using System;
using System.IO;
using UnityEngine;

/// <summary>
/// A utility class for fetching Android process information using Android's ActivityManager via JNI.
/// </summary>
/// <remarks>
/// Performance Warning: Querying the ActivityManager via JNI is relatively slow and creates garbage 
/// on both the C# and Java sides. This should be done sparingly (e.g., once every few seconds), 
/// rather than every frame.
/// </remarks>
public static class AndroidProcessStats
{
    // Importance constants defined by android.app.ActivityManager$RunningAppProcessInfo

    public const int IMPORTANCE_FOREGROUND = 100;
    public const int IMPORTANCE_FOREGROUND_SERVICE = 125;
    public const int IMPORTANCE_PERCEPTIBLE = 230;
    public const int IMPORTANCE_CANT_SAVE_STATE = 170;
    public const int IMPORTANCE_SERVICE = 300;
    public const int IMPORTANCE_CACHED = 400;

    /// <summary>
    /// Fetches process information for all processes running under this app's UID.
    /// This includes the main game process, any background services, or isolated WebViews.
    /// </summary>
    /// <returns>A list of processes and their OS-level importance.</returns>
    public static System.Collections.Generic.List<AppProcessInfo> GetAllRunningAppProcesses()
    {
        var processes = new System.Collections.Generic.List<AppProcessInfo>();
        if (Application.platform != RuntimePlatform.Android) 
        {
            // Fallback for Unity Editor so testing can continue without crashing
            processes.Add(new AppProcessInfo { 
                pid = System.Diagnostics.Process.GetCurrentProcess().Id, 
                processName = "Unity Editor (Fallback)", 
                importance = -1 
            });
            return processes;
        }

        try
        {
            // Note on JNI (Java Native Interface):
            // We use AndroidJavaClass and AndroidJavaObject to call Java methods from C#.
            // The 'using' blocks ensure that the JNI local references are disposed of immediately,
            // preventing memory leaks in the local reference table.
            using (var activity = new AndroidJavaClass("com.unity3d.player.UnityPlayer").GetStatic<AndroidJavaObject>("currentActivity"))
            using (var context = activity.Call<AndroidJavaObject>("getApplicationContext"))
            using (var activityManager = context.Call<AndroidJavaObject>("getSystemService", "activity"))
            {
                // Call ActivityManager.getRunningAppProcesses()
                var runningProcesses = activityManager.Call<AndroidJavaObject>("getRunningAppProcesses");
                if (runningProcesses != null)
                {
                    int count = runningProcesses.Call<int>("size");
                    for (int i = 0; i < count; i++)
                    {
                        using (var processInfo = runningProcesses.Call<AndroidJavaObject>("get", i))
                        {
                            var info = new AppProcessInfo();
                            info.pid = processInfo.Get<int>("pid");
                            info.processName = processInfo.Get<string>("processName");
                            info.importance = processInfo.Get<int>("importance");
                            
                            // Get the component causing this importance (if any).
                            // This helps identify *why* a process is kept alive (e.g., a specific background service).
                            using (var component = processInfo.Get<AndroidJavaObject>("importanceReasonComponent"))
                            {
                                if (component != null)
                                {
                                    info.reasonComponent = component.Call<string>("flattenToShortString");
                                }
                            }
                            processes.Add(info);
                        }
                    }
                }
            }
        }
        catch (System.Exception e)
        {
            Debug.LogError($"[AndroidProcessStats] JNI Query Failed: {e.Message}");
        }
        return processes;
    }

    /// <summary>
    /// Struct containing basic OS information about an app process.
    /// </summary>
    public struct AppProcessInfo
    {
        public int pid;
        public string processName;
        public int importance;
        public string reasonComponent;
    }
}

/// <summary>
/// Holds a snapshot of memory usage metrics read directly from the Linux procfs.
/// </summary>
public struct UnityMemorySnapshot
{
    public int oomScoreAdj;      // Process priority adjustment score (-1000 to +1000)
    public long totalRssKb;      // Total Resident Set Size in memory
    public long anonRssKb;       // Anonymous RSS (Java heap + unmanaged Native engine heap)
    public long swapKb;          // Compressed memory swapped into zRAM
    
    /// <summary>
    /// The sum of anonymous RSS and swap. This is often considered the "gold standard" 
    /// for measuring the background footprint of a process, as it represents memory 
    /// that cannot be discarded by the OS (unlike clean file-backed pages).
    /// </summary>
    public long anonRssPlusSwap => anonRssKb + swapKb;
}

/// <summary>
/// A utility class for directly reading Linux process virtual files (/proc).
/// </summary>
/// <remarks>
/// Reading from /proc is extremely fast and avoids the heavy overhead of JNI or 
/// allocating large Java objects. This makes it suitable for higher-frequency polling
/// on a background thread.
/// </remarks>
public static class AndroidNativeDiagnostics
{
    /// <summary>
    /// Reads the OOM (Out Of Memory) score adjustment directly from virtual Linux storage.
    /// The higher the score, the more likely the process is to be killed by the low memory killer (LMK).
    /// </summary>
    /// <param name="pid">The process ID.</param>
    /// <returns>The OOM score adjustment, or -9999 if it cannot be read.</returns>
    public static int GetOomScoreAdj(int pid)
    {
        // /proc/[pid]/oom_score_adj contains the adjustment score set by Android's ActivityManager.
        string path = $"/proc/{pid}/oom_score_adj";
        if (!File.Exists(path))
        {
            return -9999;
        }

        try
        {
            string contents = File.ReadAllText(path).Trim();
            if (int.TryParse(contents, out int score))
            {
                return score;
            }
        }
        catch
        {
        }
        return -9999;
    }

    /// <summary>
    /// Parses /proc/{pid}/status in a single pass to fetch raw memory pages.
    /// </summary>
    /// <param name="pid">The process ID.</param>
    /// <returns>A UnityMemorySnapshot containing RSS and Swap values.</returns>
    public static UnityMemorySnapshot CaptureMemorySnapshot(int pid)
    {
        var snapshot = new UnityMemorySnapshot();
        snapshot.oomScoreAdj = GetOomScoreAdj(pid);

        // /proc/[pid]/status contains human-readable status information about the process,
        // including detailed memory consumption breakdowns.
        string path = $"/proc/{pid}/status";
        if (!File.Exists(path))
        {
            return snapshot;
        }

        try
        {
            string[] lines = File.ReadAllLines(path);
            foreach (string line in lines)
            {
                if (line.StartsWith("VmRSS:", StringComparison.OrdinalIgnoreCase))
                {
                    snapshot.totalRssKb = ParseKbValue(line);
                }
                else if (line.StartsWith("RssAnon:", StringComparison.OrdinalIgnoreCase))
                {
                    snapshot.anonRssKb = ParseKbValue(line);
                }
                else if (line.StartsWith("VmSwap:", StringComparison.OrdinalIgnoreCase))
                {
                    snapshot.swapKb = ParseKbValue(line);
                }
            }
        }
        catch (Exception e)
        {
            Debug.LogError($"[NativeDiagnostics] Failed parsing /proc/self/status: {e.Message}");
        }

        return snapshot;
    }

    private static long ParseKbValue(string line)
    {
        if (string.IsNullOrEmpty(line))
        {
            return 0;
        }

        try
        {
            // 1. Split by the colon first (e.g., "RssAnon:      154388 kB")
            string[] colonParts = line.Split(':');
            if (colonParts.Length < 2)
            {
                return 0; // Guard rail: Skip if no colon found
            }

            // 2. Extract the value portion string at index 1 ("     154388 kB")
            string valueSection = colonParts[1];
            string[] spaceParts = valueSection.Split(new[] { ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries);
            
            // 3. Extract the first string element at index 0 ("154388") and parse it
            if (spaceParts.Length > 0 && long.TryParse(spaceParts[0], out long value))
            {
                return value;
            }
        }
        catch (System.Exception e)
        {
            // Prevent a single malformed line from interrupting the telemetry update loop
            Debug.LogWarning($"[NativeDiagnostics] Skipping malformed line '{line}': {e.Message}");
        }
        
        return 0;
    }
}