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
import android.app.ApplicationExitInfo;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;

import com.unity3d.player.UnityPlayer;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Objects;
import java.util.UUID;

public class AppExitInfoHandler {

    private final Context _context;
    private final boolean _sendUnityMsg;

    private String appDirectory;
    private String appName;

    private boolean isProcessing;

    private final JSONArray jsonArray;
    private final String Folder = "ANR_Traces";
    private Thread _thread;


    public AppExitInfoHandler(Context context, boolean sendUnityMsg) {
        _context = context;
        _sendUnityMsg = sendUnityMsg;
        jsonArray = new JSONArray();
        appName = _context.getApplicationInfo().packageName;
    }

    public void checkForExitInfo() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            if (_sendUnityMsg) {
                Log.w(UnityDebugHelper.Tag, "Unable to check - Android 11 required");
                sendUnityEvent("OnAppExitInfoChecked", "");
            }
            return;
        }

        if (_thread != null && _thread.isAlive()) {
            Log.w(UnityDebugHelper.Tag, "Processing AppExitInfo is already in progress");
            return;
        }


        Runnable runnable = this::processPendingAppExitInfo;
        _thread = new Thread(runnable);
        _thread.setName("AppExitInfo");
        _thread.start();
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    private void processPendingAppExitInfo() {

        List<ApplicationExitInfo> exitInfoList = getExitInfoList();
        if (exitInfoList == null || exitInfoList.isEmpty()) {
            sendUnityEvent("OnAppExitInfoChecked", "");
            return;
        }

        prepareDirectory();

        try {
            saveExitInfo(exitInfoList);

            JSONObject wrapper = new JSONObject();
            wrapper.put("entries", jsonArray);

            if (UnityDebugHelper.logEnabled)
                Log.i(UnityDebugHelper.Tag, wrapper.toString());

            sendUnityEvent("OnAppExitInfoChecked", wrapper.toString());

        } catch (JSONException e) {
            Log.e(UnityDebugHelper.Tag, e.toString());
        }


    }

    private void prepareDirectory() {
        if (appDirectory != null) return;

        String extFilesDir;
        if (FileHandler.isExternalStorageAvailable())
            extFilesDir = Objects.requireNonNull(_context.getExternalFilesDir(""), "").getAbsolutePath();
        else
            extFilesDir = _context.getFilesDir().getAbsolutePath();

        appDirectory = extFilesDir + "/" + Folder + "/";
        File directory = new File(appDirectory);
        if (!directory.exists())
            directory.mkdir();

    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    private List<ApplicationExitInfo> getExitInfoList() {
        ActivityManager am = (ActivityManager) _context.getSystemService(Context.ACTIVITY_SERVICE);
        List<ApplicationExitInfo> appExitInfoList = am.getHistoricalProcessExitReasons(null, 0, 0);

        if (appExitInfoList.size() == 0)
            return null;

        SharedPreferences sharedPreferences = _context.getSharedPreferences("appExitHelper", Context.MODE_PRIVATE);

        String PREV_DETECTED_TIME = "PREV_DETECTED_TIME";
        long lastExitTimestamp = sharedPreferences.getLong(PREV_DETECTED_TIME, 0);

        List<ApplicationExitInfo> unprocessedExitInfoList = new ArrayList<>();
        for (ApplicationExitInfo exitInfo : appExitInfoList) {
            if (exitInfo.getTimestamp() <= lastExitTimestamp)
                break;

            if (shouldProcess(exitInfo.getReason()))
                unprocessedExitInfoList.add(exitInfo);

        }

        if (unprocessedExitInfoList.size() > 0) {
            long timestamp = unprocessedExitInfoList.get(0).getTimestamp();
            sharedPreferences.edit().putLong(PREV_DETECTED_TIME, timestamp).apply();
        }

        return unprocessedExitInfoList;
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    private void saveExitInfo(List<ApplicationExitInfo> unprocessedExitInfoList) throws JSONException {

        String description, reason;

        for (ApplicationExitInfo exitInfo : unprocessedExitInfoList) {

            int reasonCode = exitInfo.getReason();

            reason = convertReason(reasonCode);
            description = checkDescription(exitInfo);

            JSONObject appExitInfoObj = new JSONObject();
            String keyDescription = "Description";
            String keyReason = "Reason";
            String keyReasonCode = "ReasonCode";
            String keyFilePath = "FilePath";
            String keyId = "ID";

            appExitInfoObj.put(keyDescription, description);
            appExitInfoObj.put(keyReason, reason);
            appExitInfoObj.put(keyReasonCode, reasonCode);


            UUID uuid = UUID.randomUUID();
            String fileName = saveLog(exitInfo, uuid.toString());
            if (fileName != null) {
                appExitInfoObj.put(keyFilePath, String.format("%s/%s", Folder, fileName));
            }
            appExitInfoObj.put(keyId, uuid.toString());

            jsonArray.put(appExitInfoObj);
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    @NonNull
    private String checkDescription(ApplicationExitInfo exitInfo) {
        String description = exitInfo.getDescription();
        if (description == null || description.isEmpty()) {
            description = getString(R.string.REASON_UNKNOWN);
        } else {
            description = description.replace(appName, "App");
        }
        return description;
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    private String saveLog(ApplicationExitInfo exitInfo, String uuid) {
        String msg;
        String fileName = "AppExitInfo_" + uuid + ".trace";
        File outFile = new File(appDirectory + "/" + fileName);

        boolean result = false;
        try (InputStream inputStream = exitInfo.getTraceInputStream()) {

            if (inputStream != null) {
                msg = String.format(Locale.ENGLISH, "Saving ANR trace file at %s", outFile.getAbsolutePath());
                if (UnityDebugHelper.logEnabled)
                    Log.i(UnityDebugHelper.Tag, msg);
                result = FileHandler.copyStreamToFile(inputStream, outFile);
            }
        } catch (IOException e) {
            Log.e(UnityDebugHelper.Tag, "copyStreamToFile: ", e);
        }

        if (!result) {
            if (UnityDebugHelper.logEnabled) {
                Log.d(UnityDebugHelper.Tag, getString(R.string.NO_TRACE));
            }
            return null;
        }
        return fileName;
    }

    private String convertReason(int reason) {

        String description = getString(R.string.REASON_UNKNOWN);
        switch (reason) {
            case ApplicationExitInfo.REASON_ANR:
                description = getString(R.string.REASON_ANR);
                break;
            case ApplicationExitInfo.REASON_CRASH:
                description = getString(R.string.REASON_CRASH);
                break;
            case ApplicationExitInfo.REASON_CRASH_NATIVE:
                description = getString(R.string.REASON_CRASH_NATIVE);
                break;
            case ApplicationExitInfo.REASON_LOW_MEMORY:
                description = getString(R.string.REASON_LOW_MEMORY);
                break;
            case ApplicationExitInfo.REASON_EXIT_SELF:
                description = getString(R.string.REASON_EXIT_SELF);
                break;
            case ApplicationExitInfo.REASON_USER_REQUESTED:
                description = getString(R.string.REASON_USER_REQUESTED);
                break;
            case ApplicationExitInfo.REASON_USER_STOPPED:
                description = getString(R.string.REASON_USER_STOPPED);
                break;

        }
        return description;
    }

    private void sendUnityEvent(String method, String data) {
        if (!_sendUnityMsg)
            return;
        UnityPlayer.currentActivity.getMainExecutor().execute(() ->
                UnityPlayer.UnitySendMessage(UnityDebugHelper.UnityGameObject, method, data));
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    private boolean shouldProcess(int reasonCode) {
        return reasonCode == ApplicationExitInfo.REASON_ANR ||
                reasonCode == ApplicationExitInfo.REASON_CRASH ||
                reasonCode == ApplicationExitInfo.REASON_LOW_MEMORY ||
                reasonCode == ApplicationExitInfo.REASON_CRASH_NATIVE ||
                reasonCode == ApplicationExitInfo.REASON_EXCESSIVE_RESOURCE_USAGE;
    }

    public String getString(int resource) {
        return _context.getResources().getString(resource);
    }
}
