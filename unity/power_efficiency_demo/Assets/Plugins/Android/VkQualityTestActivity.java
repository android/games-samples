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
package com.google.android.games.VkQualityTest;

import com.unity3d.player.R;
import com.unity3d.player.UnityPlayerActivity;
import android.os.Bundle;
import android.os.Build;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceView;
import com.google.android.games.vkquality.VKQuality;

public class VkQualityTestActivity extends UnityPlayerActivity {

    private static final int OVERRIDE_NONE = 0;
    private static final int OVERRIDE_GLES = 1;
    private static final int OVERRIDE_VULKAN = 2;

    private int apiOverride = OVERRIDE_NONE;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        CheckVkQuality();
        super.onCreate(savedInstanceState);
    }

    @Override protected String updateUnityCommandLineArguments(String cmdLine)
    {
        if (apiOverride == OVERRIDE_VULKAN) {
            Log.i("VKQUALITY", "Passing -force-vulkan");
            return appendCommandLineArgument(cmdLine, "-force-vulkan");
        }
        else if (apiOverride == OVERRIDE_GLES) {
            Log.i("VKQUALITY", "Passing -force-gles");
            return appendCommandLineArgument(cmdLine, "-force-gles");
        }
        Log.i("VKQUALITY", "No override passed");
        // let Unity pick the Graphics API based on PlayerSettings
        return cmdLine;
    }    

    private void CheckVkQuality() {
        VKQuality vkQuality = new VKQuality(this);
        int startResult = vkQuality.StartVkQuality("");
        if (startResult == VKQuality.INIT_SUCCESS) {
            // In the current release, we can assume GetVkQuality is
            // ready as soon as StartVkQuality has returned success
            int getResult = vkQuality.GetVkQuality();
            LogVkQualityResult(getResult);

            switch (getResult) {
                case VKQuality.RECOMMENDATION_VULKAN_BECAUSE_DEVICE_MATCH:
                case VKQuality.RECOMMENDATION_VULKAN_BECAUSE_PREDICTION_MATCH:
                case VKQuality.RECOMMENDATION_VULKAN_BECAUSE_FUTURE_ANDROID:
                    apiOverride = OVERRIDE_VULKAN;
                    break;
                case VKQuality.RECOMMENDATION_GLES_BECAUSE_OLD_DEVICE:
                case VKQuality.RECOMMENDATION_GLES_BECAUSE_OLD_DRIVER:
                case VKQuality.RECOMMENDATION_GLES_BECAUSE_NO_DEVICE_MATCH:
                case VKQuality.RECOMMENDATION_GLES_BECAUSE_PREDICTION_MATCH:                
                default:
                    apiOverride = OVERRIDE_GLES;
                    break;
            }
            vkQuality.StopVkQuality();
        } else {
            LogVkQualityError(startResult);
        }
    }

    private String LogVkQualityResult(int getResult) {
        String returnString = "";
        switch (getResult) {
            case VKQuality.RECOMMENDATION_NOT_READY:
                returnString = "RECOMMENDATION_NOT_READY";
                break;
            case VKQuality.RECOMMENDATION_ERROR_NOT_INITIALIZED:
                returnString = "RECOMMENDATION_ERROR_NOT_INITIALIZED";
                break;
            case VKQuality.RECOMMENDATION_VULKAN_BECAUSE_DEVICE_MATCH:
                returnString = "RECOMMENDATION_VULKAN_BECAUSE_DEVICE_MATCH";
                break;
            case VKQuality.RECOMMENDATION_VULKAN_BECAUSE_PREDICTION_MATCH:
                returnString = "RECOMMENDATION_VULKAN_BECAUSE_PREDICTION_MATCH";
                break;
            case VKQuality.RECOMMENDATION_VULKAN_BECAUSE_FUTURE_ANDROID:
                returnString = "RECOMMENDATION_VULKAN_BECAUSE_FUTURE_ANDROID";
                break;
            case VKQuality.RECOMMENDATION_GLES_BECAUSE_OLD_DEVICE:
                returnString = "RECOMMENDATION_GLES_BECAUSE_OLD_DEVICE";
                break;
            case VKQuality.RECOMMENDATION_GLES_BECAUSE_OLD_DRIVER:
                returnString = "RECOMMENDATION_GLES_BECAUSE_OLD_DRIVER";
                break;
            case VKQuality.RECOMMENDATION_GLES_BECAUSE_NO_DEVICE_MATCH:
                returnString = "RECOMMENDATION_GLES_BECAUSE_NO_DEVICE_MATCH";
                break;
            case VKQuality.RECOMMENDATION_GLES_BECAUSE_PREDICTION_MATCH:
                returnString = "RECOMMENDATION_GLES_BECAUSE_PREDICTION_MATCH";
                break;
            default:
                returnString = "Unknown getVkQuality result: " + getResult;
                break;
        }
        Log.i("VKQUALITY", returnString);
        return returnString;
    }

    private String LogVkQualityError(int startResult) {
        String returnString = "";
        switch (startResult) {
            case VKQuality.ERROR_INITIALIZATION_FAILURE:
                returnString = "ERROR_INITIALIZATION_FAILURE";
                break;
            case VKQuality.ERROR_NO_VULKAN:
                returnString = "ERROR_NO_VULKAN";
                break;
            case VKQuality.ERROR_INVALID_DATA_VERSION:
                returnString = "ERROR_INVALID_DATA_VERSION";
                break;
            case VKQuality.ERROR_INVALID_DATA_FILE:
                returnString = "ERROR_INVALID_DATA_FILE";
                break;
            case VKQuality.ERROR_MISSING_DATA_FILE:
                returnString = "ERROR_MISSING_DATA_FILE";
                break;
            default:
                returnString = "Unknown startVkQuality result: " + startResult;
                break;
        }
        Log.e("VKQUALITY", returnString);
        return returnString;
    }

    private String appendCommandLineArgument(String cmdLine, String arg) {
        if (arg == null || arg.isEmpty())
            return cmdLine;
        else if (cmdLine == null || cmdLine.isEmpty())
            return arg;
        else
            return cmdLine + " " + arg;
    }

    public void SetDisplayRefreshRate(int refreshRate) 
    {
        SurfaceView surfaceView = findViewById(R.id.unitySurfaceView);        
        Surface targetSurface = surfaceView.getHolder().getSurface();
        if (VERSION.SDK_INT >= VERSION_CODES.R) {
            targetSurface.setFrameRate(refreshRate, Surface.FRAME_RATE_COMPATIBILITY_DEFAULT);
        }
    }
}