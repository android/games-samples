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

using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using UnityEngine;

/// <summary>
/// A MonoBehaviour that monitors memory and process state for the application.
/// It uses a background thread for high-frequency polling of lightweight /proc stats,
/// and throttles heavier JNI calls on the main thread to avoid frame drops.
/// </summary>
public class GameMemoryMonitor : MonoBehaviour
{
    public bool showDebugOverlay = true;
    private string cachedOverlayText = "";
    private float nextJniUpdateTime = 0f;
    private float nextLogTime = 0f;

    private List<AndroidProcessStats.AppProcessInfo> knownProcesses = new List<AndroidProcessStats.AppProcessInfo>();
    private readonly object processLock = new object();
    
    private Dictionary<int, PidStats> pidStatsMap = new Dictionary<int, PidStats>();
    private readonly object statsLock = new object();

    private Thread pollingThread;
    private volatile bool isRunning = true;

    private string[] pidColorHexes = new string[] {
        "#FF5555", "#55FF55", "#FFFF55", "#5555FF",
        "#FF55FF", "#55FFFF", "#FFFFFF", "#AAAAAA",
        "#FF9999", "#9999FF"
    };

    /// <summary>
    /// Container for tracking memory trends specific to a Process ID.
    /// </summary>
    public class PidStats
    {
        public string processName;
        public string colorHex;
        
        // Tracks the highest memory seen during each OOM adj state
        public Dictionary<int, long> maxMemPerOom = new Dictionary<int, long>();
        // Tracks time spent at various memory levels during each OOM adj state
        public Dictionary<int, Dictionary<long, float>> memHistograms = new Dictionary<int, Dictionary<long, float>>();
        
        // Latest snapshot data for GUI presentation
        public int lastOomAdj;
        public int lastImportance;
        public long lastAnonRssKb;
        public long lastSwapKb;
        public string lastReasonComponent;
    }

    private void Start()
    {
        // PERFORMANCE: Start a background thread to read /proc files. 
        // File I/O (even virtual files like /proc) can occasionally block.
        // Doing this off the main thread ensures we never stutter the game's framerate.
        pollingThread = new Thread(BackgroundPollingLoop);
        pollingThread.Start();
    }

    private void OnDestroy()
    {
        isRunning = false;
        if (pollingThread != null)
        {
            if (!pollingThread.Join(500))
            {
                pollingThread.Abort();
            }
        }
    }

    private void OnApplicationPause(bool isPaused)
    {
        if (isPaused)
        {
            Debug.Log("[MemoryMonitor] Game minimized. Querying entry stats...");
            LogDiagnostics("BACKGROUND_TRANSITION_START");
        }
        else
        {
            Debug.Log("[MemoryMonitor] Game resumed. Querying active stats...");
            LogDiagnostics("FOREGROUND_RESUME");
        }
    }

    /// <summary>
    /// The background loop that continuously reads memory data.
    /// Runs on a separate thread.
    /// </summary>
    private void BackgroundPollingLoop()
    {
        float pollInterval = 0.1f; // Poll at 10Hz to catch quick transitions
        
        while (isRunning)
        {
            List<AndroidProcessStats.AppProcessInfo> currentProcesses;
            
            // Thread Safety: Lock when reading the shared list of known processes
            // which is updated by the main thread.
            lock (processLock)
            {
                currentProcesses = new List<AndroidProcessStats.AppProcessInfo>(knownProcesses);
            }

            lock (statsLock)
            {
                foreach (var proc in currentProcesses)
                {
                    if (!pidStatsMap.ContainsKey(proc.pid))
                    {
                        pidStatsMap[proc.pid] = new PidStats {
                            processName = proc.processName,
                            colorHex = pidColorHexes[pidStatsMap.Count % pidColorHexes.Length]
                        };
                    }

                    var stats = pidStatsMap[proc.pid];
                    UnityMemorySnapshot mem = AndroidNativeDiagnostics.CaptureMemorySnapshot(proc.pid);
                    
                    if (mem.oomScoreAdj != -9999)
                    {
                        long currentTotal = mem.anonRssPlusSwap;

                        // Update Max
                        if (!stats.maxMemPerOom.ContainsKey(mem.oomScoreAdj))
                        {
                            stats.maxMemPerOom[mem.oomScoreAdj] = 0;
                        }
                        if (currentTotal > stats.maxMemPerOom[mem.oomScoreAdj])
                        {
                            stats.maxMemPerOom[mem.oomScoreAdj] = currentTotal;
                        }

                        // Update Histogram using absolute MB to prevent corruption when max changes
                        long bucketMb = currentTotal / 1024; 
                        
                        if (!stats.memHistograms.ContainsKey(mem.oomScoreAdj))
                        {
                            stats.memHistograms[mem.oomScoreAdj] = new Dictionary<long, float>();
                        }
                        if (!stats.memHistograms[mem.oomScoreAdj].ContainsKey(bucketMb))
                        {
                            stats.memHistograms[mem.oomScoreAdj][bucketMb] = 0f;
                        }
                        stats.memHistograms[mem.oomScoreAdj][bucketMb] += pollInterval;

                        // Update latest snapshot
                        stats.lastOomAdj = mem.oomScoreAdj;
                        stats.lastImportance = proc.importance;
                        stats.lastAnonRssKb = mem.anonRssKb;
                        stats.lastSwapKb = mem.swapKb;
                        stats.lastReasonComponent = proc.reasonComponent;
                    }
                }
            }
            
            Thread.Sleep(100);
        }
    }

    private void Update()
    {
        if (!showDebugOverlay)
        {
            return;
        }

        // PERFORMANCE: Throttle the heavy JNI queries to once every 2 seconds.
        // ActivityManager.getRunningAppProcesses() does IPC (Inter-Process Communication) 
        // to the system server, which is slow and creates garbage. We do NOT want to call this every frame.
        if (Time.time > nextJniUpdateTime)
        {
            nextJniUpdateTime = Time.time + 2.0f;
            var newProcs = AndroidProcessStats.GetAllRunningAppProcesses();
            
            // Thread Safety: Lock when writing the shared list of known processes.
            lock (processLock)
            {
                knownProcesses = newProcs;
            }
            RefreshOverlayText();
        }

        // Print histograms every 60 seconds
        if (Time.time > nextLogTime && Time.time > 10.0f)
        {
            nextLogTime = Time.time + 60.0f;
            LogHistogramSummary();
        }
    }

    private void LogHistogramSummary()
    {
        StringBuilder sb = new StringBuilder();
        sb.AppendLine("[MemoryMonitor] --- 60s Periodic Histogram Summary ---");
        
        lock (statsLock)
        {
            foreach (var kvp in pidStatsMap)
            {
                int pid = kvp.Key;
                var stats = kvp.Value;
                sb.AppendLine($"PID: {pid} ({stats.processName})");
                
                foreach (var oomKvp in stats.maxMemPerOom)
                {
                    int oom = oomKvp.Key;
                    long maxMem = oomKvp.Value;
                    
                    float timeAbove95 = 0f;
                    long thresholdMb = (long)(maxMem * 0.95f) / 1024;

                    if (stats.memHistograms.ContainsKey(oom))
                    {
                        foreach (var bucketKvp in stats.memHistograms[oom])
                        {
                            if (bucketKvp.Key >= thresholdMb)
                            {
                                timeAbove95 += bucketKvp.Value;
                            }
                        }
                    }
                    
                    sb.AppendLine($"  OOM {oom}: Max Mem = {(maxMem / 1024f):F2} MB, Time >95% Peak = {timeAbove95:F1}s");
                }
            }
        }
        sb.AppendLine("---------------------------------------------------");
        Debug.Log(sb.ToString());
    }

    private void RefreshOverlayText()
    {
        var sb = new StringBuilder();

        lock (statsLock)
        {
            foreach (var kvp in pidStatsMap)
            {
                int pid = kvp.Key;
                var stats = kvp.Value;
                
                string calculatedProcGroup = MapToProcGroup(stats.lastOomAdj);
                
                sb.AppendLine($"<color={stats.colorHex}>PID: {pid} ({stats.processName})</color>");
                sb.AppendLine($"OOM Adj: {stats.lastOomAdj} | Imp: {stats.lastImportance}");
                sb.AppendLine($"State: {calculatedProcGroup}");
                
                if (Application.platform == RuntimePlatform.Android)
                {
                    sb.AppendLine($"Current Anon+Swap: {((stats.lastAnonRssKb + stats.lastSwapKb) / 1024f):F2} MB");
                    
                    if (stats.maxMemPerOom.ContainsKey(stats.lastOomAdj))
                    {
                        sb.AppendLine($"<color=#ffddaa>Max at OOM {stats.lastOomAdj}: {(stats.maxMemPerOom[stats.lastOomAdj] / 1024f):F2} MB</color>");
                    }

                    if (!string.IsNullOrEmpty(stats.lastReasonComponent))
                    {
                        sb.AppendLine($"Reason: {stats.lastReasonComponent}");
                    }
                }
                sb.AppendLine("-----------------------------");
            }
        }
        cachedOverlayText = sb.ToString();
        needsFontRecalc = true;
    }

    private int cachedFontSize = 14;
    private bool needsFontRecalc = true;
    private int lastScreenWidth = 0;

    private void OnGUI()
    {
        if (!showDebugOverlay || string.IsNullOrEmpty(cachedOverlayText))
        {
            return;
        }

        float rectWidth = Screen.width;
        float rectHeight = Screen.height * 0.5f;
        Rect overlayRect = new Rect(0, 0, rectWidth, rectHeight);

        // PERFORMANCE: Recalculate font size only when text changes or screen resizes.
        // GUIStyle.CalcHeight is an expensive operation that computes layout.
        // Doing this every frame would cause significant CPU overhead.
        if (needsFontRecalc || lastScreenWidth != Screen.width)
        {
            lastScreenWidth = Screen.width;
            needsFontRecalc = false;
            
            // Allow Rich Text in CalcHeight
            GUIStyle tempStyle = new GUIStyle(GUI.skin.label) { wordWrap = true, padding = new RectOffset(10, 10, 10, 10), richText = true };
            int bestFont = 10;
            for (int i = 70; i >= 10; i--)
            {
                tempStyle.fontSize = i;
                if (tempStyle.CalcHeight(new GUIContent(cachedOverlayText), rectWidth) <= rectHeight)
                {
                    bestFont = i;
                    break;
                }
            }
            cachedFontSize = bestFont;
        }

        // Draw a translucent black background that doesn't block raycasts
        Color oldColor = GUI.color;
        GUI.color = new Color(0, 0, 0, 0.7f);
        GUI.DrawTexture(overlayRect, Texture2D.whiteTexture);
        GUI.color = oldColor;

        // Draw the text
        GUIStyle style = new GUIStyle(GUI.skin.label)
        {
            fontSize = cachedFontSize,
            normal = { textColor = Color.green },
            wordWrap = true,
            padding = new RectOffset(10, 10, 10, 10),
            richText = true
        };

        GUI.Label(overlayRect, cachedOverlayText, style);
    }

    private void LogDiagnostics(string contextTag)
    {
        var processes = AndroidProcessStats.GetAllRunningAppProcesses();

        foreach (var proc in processes)
        {
            UnityMemorySnapshot mem = AndroidNativeDiagnostics.CaptureMemorySnapshot(proc.pid);
            string calculatedProcGroup = MapToProcGroup(mem.oomScoreAdj);
            
            string componentInfo = string.IsNullOrEmpty(proc.reasonComponent) ? "" : $" | Reason: {proc.reasonComponent}";

            Debug.Log($"[{contextTag}] [PID: {proc.pid} ({proc.processName})] " +
                    $"OOM Score: {mem.oomScoreAdj} | " +
                    $"OS Importance: {proc.importance} | " +
                    $"Engine Proc State: {calculatedProcGroup} | " +
                    $"Anon RSS + Swap: {(mem.anonRssPlusSwap / 1024f):F2} MB{componentInfo}");
        }
    }

    /// <summary>
    /// Maps the raw OOM score adjustment to a human-readable Android process state.
    /// </summary>
    /// <param name="adj">The OOM score adjustment value.</param>
    /// <returns>A string describing the state.</returns>
    private string MapToProcGroup(int adj)
    {
        if (adj == 0)
        {
            return "0 (FOREGROUND_APP_ADJ): Foreground (focused/visible)";
        }
        if (adj == 50)
        {
            return "50 (RECENT_FOREGROUND): Briefly bumped to avoid disruption";
        }
        if (adj == 100)
        {
            return "100 (VISIBLE_APP_ADJ): Visible but not focused";
        }
        if (adj == 200)
        {
            return "200 (PERCEPTIBLE_APP_ADJ): Background with visible UI (e.g., PiP)";
        }
        if (adj >= 201 && adj <= 224)
        {
            return $"{adj} (PERCEPTIBLE_LESS_IMPORTANT): Less important perceptible";
        }
        if (adj == 225)
        {
            return "225 (PERCEPTIBLE_MEDIUM_APP_ADJ): Device state dependent (e.g. mic active)";
        }
        if (adj >= 226 && adj <= 249)
        {
            return $"{adj} (PERCEPTIBLE_LESS_IMPORTANT): Less important perceptible";
        }
        if (adj == 250)
        {
            return "250 (PERCEPTIBLE_LOW_APP_ADJ): Low perceptible (e.g. location)";
        }
        if (adj >= 300 && adj < 400)
        {
            return $"{adj} (BACKUP_APP_ADJ): Performing backup";
        }
        if (adj >= 400 && adj < 500)
        {
            return $"{adj} (HEAVY_WEIGHT): Heavy weight app";
        }
        if (adj >= 500 && adj < 600)
        {
            return $"{adj} (SERVICE_ADJ): Background service";
        }
        if (adj >= 600 && adj < 700)
        {
            return $"{adj} (HOME_APP_ADJ): Home/Launcher";
        }
        if (adj >= 700 && adj < 800)
        {
            return $"{adj} (PREVIOUS_APP_ADJ): Previous app (user navigated away)";
        }
        if (adj >= 800 && adj < 900)
        {
            return $"{adj} (SERVICE_B_ADJ): Less important service";
        }
        if (adj >= 900)
        {
            return $"{adj} (CACHED_APP_ADJ): Cached/Idle (expendable)";
        }
        return $"{adj} (UNKNOWN): Unmapped score";
    }
}