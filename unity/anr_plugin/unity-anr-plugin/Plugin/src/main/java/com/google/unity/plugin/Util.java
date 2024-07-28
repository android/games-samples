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

import android.content.Context;
import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;

import com.unity3d.player.UnityPlayer;

import java.time.DateTimeException;
import java.util.Locale;
import java.util.concurrent.TimeUnit;


public class Util {

    public static final int DefaultWaitTime = 15500;
    public static void threadSleep() {
        long time = System.nanoTime();
        Log.d(UnityDebugHelper.Tag, "Forcing Thread Sleep. Running Thread: " + Thread.currentThread().getName());
        try {
            Thread.sleep(DefaultWaitTime);
            time = logTime(time, "threadSleep");
        } catch (InterruptedException e) {
            Log.e(UnityDebugHelper.Tag, "My custom ANR. Exception: ", e);
        }

        UnityPlayer.UnitySendMessage(UnityDebugHelper.UnityGameObject, "OnProcessingCompleted",
                String.format(Locale.ENGLISH, "%dms", time));

    }

    public static void threadSleepMain(Context context) {

        context.getMainExecutor().execute(() -> {
            long time = System.nanoTime();
            Log.d(UnityDebugHelper.Tag, "Forcing Thread Sleep. Running Thread: " + Thread.currentThread().getName());
            try {
                Thread.sleep(DefaultWaitTime);
                time = logTime(time, "threadSleepMain");
                UnityPlayer.UnitySendMessage(UnityDebugHelper.UnityGameObject, "OnProcessingCompleted",
                        String.format(Locale.ENGLISH, "%dms", time));
            } catch (InterruptedException e) {
                Log.e(UnityDebugHelper.Tag, "My custom ANR. Exception: ", e);
            }
        });
    }

    public static void threadOverheadMain(Context context) {

        context.getMainExecutor().execute(() -> {
            long time = System.nanoTime();
            Log.d(UnityDebugHelper.Tag, "Forcing Thread Sleep. Running Thread: " + Thread.currentThread().getName());

            int a = 1;
            int b = 2;
            while (System.currentTimeMillis() < time + 5100) ;
            {
                a = a + b;
                if (a > 10000000)
                    a = 0;
            }
            time = logTime(time, "threadOverheadMain");
            UnityPlayer.UnitySendMessage(UnityDebugHelper.UnityGameObject, "OnProcessingCompleted",
                    String.format(Locale.ENGLISH, "%dms", time));
        });
    }

    public static long logTime(long time, String operation) {
        time = System.nanoTime() - time;
        time = TimeUnit.NANOSECONDS.toMillis(time);
        Log.d(UnityDebugHelper.Tag, String.format("Operation %s took: " + time + "ms", operation));
        return time;
    }

}
