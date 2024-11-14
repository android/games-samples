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
import android.content.pm.PackageManager;

public class PlatformChecker {

    private final boolean isChrome;
    private final boolean isPlayGamesPC;

    public PlatformChecker(Context context) {
        PackageManager packageManager = context.getPackageManager();
        String CHROMIUM = "org.chromium.arc.device_management";
        isChrome = packageManager.hasSystemFeature(CHROMIUM);

        String HPE_EXPERIENCE = "com.google.android.play.feature.HPE_EXPERIENCE";
        isPlayGamesPC = packageManager.hasSystemFeature(HPE_EXPERIENCE);
    }

    public boolean isChromebook() {
        return isChrome;
    }

    public boolean isPlayGamesPC() {

        return isPlayGamesPC;
    }


}
