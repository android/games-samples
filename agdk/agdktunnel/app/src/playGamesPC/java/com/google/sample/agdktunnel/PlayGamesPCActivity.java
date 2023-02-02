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
package com.google.sample.agdktunnel;

import android.content.pm.PackageManager;
import android.os.Bundle;

import com.google.android.libraries.play.games.inputmapping.InputMappingClient;
import com.google.android.libraries.play.games.inputmapping.InputMappingProvider;
import com.google.android.libraries.play.games.inputmapping.Input;

// If you are building for Google Play Games for Windows, some APIs modules are not supported.
// For example, references to the following modules will fail silently at runtime and may cause
// unexpected behavior.
// import com.google.android.gms.analytics; // NOT SUPPORTED
// import com.google.android.gms.location.places; // NOT SUPPORTED

public class PlayGamesPCActivity extends AGDKTunnelActivity {

    private final String mPlayGamesPCSystemFeature =
            "com.google.android.play.feature.HPE_EXPERIENCE";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (isGooglePlayGames()) {
            InputMappingProvider inputMappingProvider = new InputSDKProvider();
            InputMappingClient inputMappingClient = Input.getInputMappingClient(this);
            inputMappingClient.setInputMappingProvider(inputMappingProvider);
        }
    }

    @Override
    protected void onDestroy() {
        if (isGooglePlayGames()) {
            InputMappingClient inputMappingClient = Input.getInputMappingClient(this);
            inputMappingClient.clearInputMappingProvider();
        }

        super.onDestroy();
    }

    public boolean isGooglePlayGames() {
        PackageManager pm = getPackageManager();
        return pm.hasSystemFeature(mPlayGamesPCSystemFeature);
    }
}