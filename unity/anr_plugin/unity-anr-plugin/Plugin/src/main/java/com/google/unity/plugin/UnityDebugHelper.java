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

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.os.Build;
import android.util.Log;
import android.webkit.WebView;

public class UnityDebugHelper {
    public static final String Tag = "Unity-Debug";
    public static String UnityGameObject = "UnityDebugHelper";

    public static UnityDebugHelper instance;
    private Context _context;
    private AppExitInfoHandler appExitInfoHandler;
    private Memory memory;
    private PlatformChecker platformChecker;

    private StrictModeHandler strictModeHandler;

    public static boolean logEnabled;


    public void init(Context context, String unityGameObject) {
        instance = this;
        _context = context;
        UnityGameObject = unityGameObject;
        memory = new Memory(_context, true);
        strictModeHandler = new StrictModeHandler();

        if (logEnabled)
            Log.w(UnityDebugHelper.Tag, "Unity ANR Plugin Initialized");
    }

    public void enableLogging(boolean enabled) {
        logEnabled = enabled;
    }


    public void checkForExitInfo() {
        if (appExitInfoHandler == null)
            appExitInfoHandler = new AppExitInfoHandler(_context, UnityGameObject != null);
        appExitInfoHandler.checkForExitInfo();
    }

    public void getMemoryInfo(boolean forceUpdate) {
        memory.getMemoryInfo(forceUpdate);
    }

    public void startMemoryStats(int interval) {
        memory.startMemoryStats(interval);
    }

    public void sendLowMemoryEvent(int level) {

        getMemoryInfo(true);
        memory.handleLowMemoryEvent(level);
    }

    public void enableStrictMode() {
        strictModeHandler.enableStrictMode();
    }

    public void disableStrictMode() {
        strictModeHandler.disableStrictMode();
    }

    public boolean isChromebook() {
        if (platformChecker == null)
            platformChecker = new PlatformChecker(_context);
        return platformChecker.isChromebook();
    }

    public boolean isPlayGamesPC() {
        if (platformChecker == null)
            platformChecker = new PlatformChecker(_context);
        return platformChecker.isPlayGamesPC();
    }

    public void threadSleep() {
        Util.threadSleep();
    }

    public void threadSleepMain() {
        Util.threadSleepMain(_context);
    }

    public void threadOverheadMain() {
        Util.threadOverheadMain(_context);
    }

    public void forceCrash() {
        Crasher.forceCrash();
    }

    public void forceNativeCrash() {
        Crasher.forceNativeCrash(_context);
    }

    public String getWebViewVersion() {
        String version = "-1";
        try {
            PackageInfo info = null;
            info = WebView.getCurrentWebViewPackage();
            if (info != null) {
                version = info.versionName;
            }
        } catch (Exception exception) {
            Log.e(UnityDebugHelper.Tag, exception.toString());
        }

        return version;
    }

    public void forceANR() {
        ActivityManager am = (ActivityManager) _context.getSystemService(Context.ACTIVITY_SERVICE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            am.appNotResponding("Forced ANR");
        }
    }
}
