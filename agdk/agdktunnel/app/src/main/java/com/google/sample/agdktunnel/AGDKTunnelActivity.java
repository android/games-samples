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

import static android.view.inputmethod.EditorInfo.IME_ACTION_NONE;
import static android.view.inputmethod.EditorInfo.IME_FLAG_NO_FULLSCREEN;

import android.content.pm.PackageManager;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.os.Bundle;

import android.text.InputType;
import android.util.Log;
import android.view.View;
import android.view.WindowManager.LayoutParams;

import androidx.annotation.Keep;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

import com.google.android.games.basegameframework.BaseGameFrameworkUtils;
import com.google.androidgamesdk.GameActivity;
import com.google.android.libraries.play.games.inputmapping.InputMappingClient;
import com.google.android.libraries.play.games.inputmapping.InputMappingProvider;
import com.google.android.libraries.play.games.inputmapping.Input;

@Keep
public class AGDKTunnelActivity extends GameActivity {

    private PGSManager mPGSManager;
    private final String mPlayGamesPCSystemFeature =
            "com.google.android.play.feature.HPE_EXPERIENCE";
    private static final String TAG = "AGDKTunnelActivity";

    // Some code to load our native library:
    static {
        // Load the STL first to workaround issues on old Android versions:
        // "if your app targets a version of Android earlier than Android 4.3
        // (Android API level 18),
        // and you use libc++_shared.so, you must load the shared library before any other
        // library that depends on it."
        // See https://developer.android.com/ndk/guides/cpp-support#shared_runtimes
        System.loadLibrary("c++_shared");

        // Optional: reload the native library.
        // However this is necessary when any of the following happens:
        //     - agdktunnel library is not configured to the following line in the manifest:
        //        <meta-data android:name="android.app.lib_name" android:value="agdktunnel" />
        //     - GameActivity derived class calls to the native code before calling
        //       the super.onCreate() function.
        System.loadLibrary("agdktunnel");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        baseGameFrameworkUtils = new BaseGameFrameworkUtils(this);
        baseGameFrameworkUtils.Initialize();

        // When true, the app will fit inside any system UI windows.
        // When false, we render behind any system UI windows.
        WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
        hideSystemUI();
        // You can set IME fields here or in native code using GameActivity_setImeEditorInfoFields.
        // We set the fields in native_engine.cpp.
        // super.setImeEditorInfoFields(InputType.TYPE_CLASS_TEXT,
        //     IME_ACTION_NONE, IME_FLAG_NO_FULLSCREEN );
        super.onCreate(savedInstanceState);

        if (isPlayGamesServicesLinked()) {
            // Initialize Play Games Services
            mPGSManager = new PGSManager(this);
        }

        if (isGooglePlayGames()) {
            InputMappingProvider inputMappingProvider = new InputSDKProvider();
            InputMappingClient inputMappingClient = Input.getInputMappingClient(this);
            inputMappingClient.registerRemappingListener(new InputSDKRemappingListener());
            inputMappingClient.setInputMappingProvider(inputMappingProvider);
        }
    }

    @Override
    protected void onDestroy() {
        if (isGooglePlayGames()) {
            InputMappingClient inputMappingClient = Input.getInputMappingClient(this);
            inputMappingClient.clearInputMappingProvider();
            inputMappingClient.clearRemappingListener();
        }

        super.onDestroy();
    }

    @Override
    protected void onResume() {
        super.onResume();

        // To learn best practices to handle lifecycle events visit
        // https://developer.android.com/topic/libraries/architecture/lifecycle
        if (isPlayGamesServicesLinked()) {
            mPGSManager.onResume();
        }
    }

    private void hideSystemUI() {
        // This will put the game behind any cutouts and waterfalls on devices which have
        // them, so the corresponding insets will be non-zero.
        if (VERSION.SDK_INT >= VERSION_CODES.P) {
            getWindow().getAttributes().layoutInDisplayCutoutMode
                = LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
        }
        // From API 30 onwards, this is the recommended way to hide the system UI, rather than
        // using View.setSystemUiVisibility.
        View decorView = getWindow().getDecorView();
        WindowInsetsControllerCompat controller = new WindowInsetsControllerCompat(getWindow(),
            decorView);
        controller.hide(WindowInsetsCompat.Type.systemBars());
        controller.hide(WindowInsetsCompat.Type.displayCutout());
        controller.setSystemBarsBehavior(
            WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
    }

    private boolean isPlayGamesServicesLinked() {
        String playGamesServicesPlaceholder = "0000000000";
        return !getString(R.string.game_services_project_id).equals(playGamesServicesPlaceholder);
    }

    private void loadCloudCheckpoint() {
        if (isPlayGamesServicesLinked()) {
            mPGSManager.loadCheckpoint();
        }
    }

    private void saveCloudCheckpoint(int level) {
        if (isPlayGamesServicesLinked()) {
            mPGSManager.saveCheckpoint(level);
        }
    }

    private String getInternalStoragePath() {
        return getFilesDir().getAbsolutePath();
    }

    private void setInputContext(int contextIndex) {
        for(InputSDKProvider.InputContextIds context : InputSDKProvider.InputContextIds.values()) {
            if (context.value() == contextIndex) {
                setInputContext(context);
                return;
            }
        }
        Log.e(TAG, String.format(
                "can't find InputContext with id %d on attempt to change of context",
                contextIndex));
    }

    private void setInputContext(InputSDKProvider.InputContextIds context) {
        InputMappingClient inputMappingClient = Input.getInputMappingClient(this);
        switch(context) {
            case INPUT_CONTEXT_PLAY_SCENE:
                inputMappingClient.setInputContext(InputSDKProvider.sPlaySceneInputContext);
                break;
            case INPUT_CONTEXT_UI_SCENE:
                inputMappingClient.setInputContext(InputSDKProvider.sUiSceneInputContext);
                break;
            case INPUT_CONTEXT_PAUSE_MENU:
                inputMappingClient.setInputContext(InputSDKProvider.sPauseMenuInputContext);
                break;
            default:
                Log.e(TAG,
                        "can't match the requested InputContext on attempt to change of context");
        }
    }

    private boolean isGooglePlayGames() {
        PackageManager pm = getPackageManager();
        return pm.hasSystemFeature(mPlayGamesPCSystemFeature);
    }

    private BaseGameFrameworkUtils baseGameFrameworkUtils;
}
