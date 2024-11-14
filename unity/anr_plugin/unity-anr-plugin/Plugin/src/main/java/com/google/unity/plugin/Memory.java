/*
 * Copyright 2024 The Android Open Source Project
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

package com.google.unity.plugin;

import static android.content.ComponentCallbacks2.TRIM_MEMORY_BACKGROUND;
import static android.content.ComponentCallbacks2.TRIM_MEMORY_COMPLETE;
import static android.content.ComponentCallbacks2.TRIM_MEMORY_MODERATE;
import static android.content.ComponentCallbacks2.TRIM_MEMORY_RUNNING_CRITICAL;
import static android.content.ComponentCallbacks2.TRIM_MEMORY_RUNNING_LOW;
import static android.content.ComponentCallbacks2.TRIM_MEMORY_RUNNING_MODERATE;
import static android.content.ComponentCallbacks2.TRIM_MEMORY_UI_HIDDEN;

import android.app.ActivityManager;
import android.content.Context;
import android.util.Log;

import androidx.core.util.Consumer;

import com.unity3d.player.UnityPlayer;

import org.json.JSONException;
import org.json.JSONObject;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

public class Memory {

    private final ActivityManager activityManager;
    private final ActivityManager.MemoryInfo memoryInfo;
    private final Runtime runtime;
    private final boolean sendUnityMsg;
    private String totalMemory;
    private final SimpleDateFormat dateFormat;
    private String jsonData;
    private final JSONObject jsonObject;


    public Memory(Context context, boolean sendUnityMsg) {
        this.sendUnityMsg = sendUnityMsg;
        memoryInfo = new ActivityManager.MemoryInfo();
        activityManager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        runtime = Runtime.getRuntime();

        dateFormat = new SimpleDateFormat("HH:mm:ss:ms", Locale.ENGLISH);

        jsonObject = new JSONObject();
    }

    public void startMemoryStats(int interval) {
        ScheduledThreadPoolExecutor executor = new ScheduledThreadPoolExecutor(1);
        executor.scheduleWithFixedDelay(this::updateMemoryInfo, 0, interval, TimeUnit.SECONDS);
    }

    private void updateMemoryInfo() {
        long time = System.nanoTime();

        activityManager.getMemoryInfo(memoryInfo);

        long availableMegs = memoryInfo.availMem / 0x100000L;
        double percentAvail = memoryInfo.availMem / (double) memoryInfo.totalMem * 100.0;

        long usedHeap = (runtime.totalMemory() - runtime.freeMemory()) / 0x100000L;
        long maxHeap = runtime.maxMemory() / 0x100000L;
        long availableHeap = maxHeap - usedHeap;

        try {
            //Unlikely to change everytime
            if (totalMemory == null) {
                totalMemory = JSONObject.numberToString(memoryInfo.totalMem / 0x100000L);
                jsonObject.put("TotalMem", totalMemory);

                long threshold = memoryInfo.threshold / 0x100000L;
                jsonObject.put("Threshold", JSONObject.numberToString(threshold));

                jsonObject.put("MaxHeap", JSONObject.numberToString(maxHeap));
            }


            jsonObject.put("AvailableMem", JSONObject.numberToString(availableMegs));
            jsonObject.put("PercentAvailable", String.format(Locale.ENGLISH, "%.1f", percentAvail));

            jsonObject.put("UsedHeap", JSONObject.numberToString(usedHeap));
            jsonObject.put("AvailableHeap", JSONObject.numberToString(availableHeap));

            String ts = dateFormat.format(new Date());
            jsonObject.put("Timestamp", ts);

            jsonData = jsonObject.toString();
            if (UnityDebugHelper.logEnabled)
                Log.d(UnityDebugHelper.Tag, jsonData);

        } catch (JSONException e) {
            throw new RuntimeException(e);
        }

        if (UnityDebugHelper.logEnabled)
            Util.logTime(time, "updateMemoryInfo");
    }

    public void getMemoryInfo(boolean forceUpdate) {
        long time = System.nanoTime();

        if (UnityDebugHelper.logEnabled)
            Log.d(UnityDebugHelper.Tag, "MemoryInfo requested to Unity. Is forced: " + forceUpdate);

        if (forceUpdate)
            updateMemoryInfo();

        if (sendUnityMsg && jsonData != null) {
            UnityPlayer.UnitySendMessage(UnityDebugHelper.UnityGameObject, "OnMemoryInfoUpdated", jsonData);
        }
        if (UnityDebugHelper.logEnabled)
            Util.logTime(time, "getMemoryInfo");
    }

    public void handleLowMemoryEvent(int level) {
        JSONObject jsonObject = new JSONObject();
        String desc = Memory.convertMemoryLevel(level);
        try {
            jsonObject.put("Level", level);
            jsonObject.put("Message", desc);
            String ts = dateFormat.format(new Date());
            jsonObject.put("Timestamp", ts);

            String jsonData = jsonObject.toString();
            if (UnityDebugHelper.logEnabled)
                Log.e(UnityDebugHelper.Tag, jsonData);
            UnityPlayer.UnitySendMessage(UnityDebugHelper.UnityGameObject, "OnTrimMemory", jsonData);
        } catch (JSONException e) {
            Log.e(UnityDebugHelper.Tag, e.toString());
        }
    }

    public static String convertMemoryLevel(int level) {

        String description = String.valueOf(level);
        switch (level) {
            case TRIM_MEMORY_BACKGROUND:
                description = "TRIM_MEMORY_BACKGROUND | Process at beginning of the LRU list";
                break;
            case TRIM_MEMORY_MODERATE:
                description = "TRIM_MEMORY_MODERATE | Process at middle of the LRU list ";
                break;
            case TRIM_MEMORY_COMPLETE:
                description = "TRIM_MEMORY_COMPLETE | Process is one the first to be killed ";
                break;

            case TRIM_MEMORY_RUNNING_MODERATE:
                description = "TRIM_MEMORY_RUNNING_MODERATE | Moderate - App is running and not killable";
                break;
            case TRIM_MEMORY_RUNNING_LOW:
                description = "TRIM_MEMORY_RUNNING_LOW | Low - App is running and not killable";
                break;
            case TRIM_MEMORY_RUNNING_CRITICAL:
                description = "TRIM_MEMORY_RUNNING_CRITICAL | Critical - App likely to be killed";
                break;

            case TRIM_MEMORY_UI_HIDDEN:
                description = "TRIM_MEMORY_UI_HIDDEN | Release large resources";
                break;

        }
        return description;
    }
}
