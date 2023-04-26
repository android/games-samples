/*
 * Copyright 2021 The Android Open Source Project
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
package com.android.example.games;

import android.content.Context;
import android.os.Build;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.os.Bundle;
import android.os.PowerManager;
import android.view.View;
import android.view.WindowManager.LayoutParams;

import android.os.PerformanceHintManager;
import android.os.PerformanceHintManager.Session;

import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

import com.google.androidgamesdk.GameActivity;

// A manager class that managers ADPF APIs in Java code.
// The class managers thermal throttle status listener and other ADPF related tasks.
public class ADPFManager implements PowerManager.OnThermalStatusChangedListener {
    // Native methods to receive thermal status.
    @SuppressWarnings("JniMissingFunction")
    protected native void nativeThermalStatusChanged(int i);
    protected native void nativeRegisterThermalStatusListener();
    protected native void nativeUnregisterThermalStatusListener();

    //
    // Thermal status change listener.
    // Invokes native implementation.
    public void onThermalStatusChanged(int i) {
        // Pass the device's thermal status to native code.
        nativeThermalStatusChanged(i);
    }

    public boolean registerListener(Context context) {
        // Retrieve power manager and register thermal state change callback.
        if (Build.VERSION.SDK_INT >= VERSION_CODES.R) {
            // Use NDK Thermal API.
            nativeRegisterThermalStatusListener();
            return true;
        } else if (Build.VERSION.SDK_INT >= VERSION_CODES.Q) {
            PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
            if (pm != null) {
                pm.addThermalStatusListener(this);
            }
            return true;
        } else {
            return false;
        }
    }

    public boolean unregisterListener(Context context) {
        // Remove the thermal state change listener on pause.
        if (Build.VERSION.SDK_INT >= VERSION_CODES.R) {
            // Use NDK Thermal API.
            nativeUnregisterThermalStatusListener();
            return true;
        } else if (Build.VERSION.SDK_INT >= VERSION_CODES.Q) {
            PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
            if (pm != null) {
                pm.removeThermalStatusListener(this);
            }
            return true;
        } else {
            return false;
        }
    }
}