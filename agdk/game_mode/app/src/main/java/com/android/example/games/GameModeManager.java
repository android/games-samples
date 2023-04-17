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

import android.app.GameManager;
import android.app.GameState;
import android.content.Context;
import android.os.Build;
import android.util.Log;

// A manager class that managers ADPF APIs in Java code.
// The class managers thermal throttle status listener and other ADPF related tasks.
public class GameModeManager {

    private Context context;
    private int gameMode;

    public void initialize(Context context) {
        this.context = context;
        this.gameMode = GameManager.GAME_MODE_UNSUPPORTED;
        if ( context != null ) {
            if ( Build.VERSION.SDK_INT >= Build.VERSION_CODES.S ) {
                // Get GameManager from SystemService
                GameManager gameManager = context.getSystemService(GameManager.class);

                // Returns the selected GameMode
                gameMode = gameManager.getGameMode();
                Log.d("GameMode", "GameMode: " + gameMode); // 0
            }
        }
        this.retrieveGameMode(this.gameMode);
    }

    public void uninitialize(Context context) {
        uninitializeGameModeManager();
    }

    public int getGameMode() {
        return this.gameMode;
    }

    protected native void retrieveGameMode(int gameMode);

    protected native void uninitializeGameModeManager();

}
