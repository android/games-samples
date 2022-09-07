/*
 * Copyright 2022 The Android Open Source Project
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

#include "game_mode_manager.h"

#include "scene_manager.h"

extern "C" {

void Java_com_android_example_games_GameModeManager_retrieveGameMode(
    JNIEnv* env, jobject obj, jint game_mode) {
  GameModeManager& gmm = GameModeManager::getInstance();
  int old_game_mode = gmm.GetGameMode();
  ALOGI("GameMode updated from %d to:%d", old_game_mode, game_mode);
  GameModeManager::getInstance().SetGameMode(game_mode);
}

void GameModeManager::SetGameMode(int game_mode) {
  ALOGI("GameMode SET: %d => %d", game_mode_, game_mode);
  game_mode_ = game_mode;
}

int GameModeManager::GetGameMode() { return game_mode_; }

const char* GameModeManager::GetGameModeString() {
  switch (game_mode_) {
    case 0:
      return "UNSUPPORTED";
    case 1:
      return "STANDARD";
    case 2:
      return "PERFORMANCE";
    case 3:
      return "BATTERY";
  }
  return "UNKNOWN";
}

const char* GameModeManager::GetFPSString(int32_t swappy_swap_interval) {
  if (swappy_swap_interval <= SWAPPY_SWAP_60FPS) {
    return "60 FPS";
  } else if (swappy_swap_interval <= SWAPPY_SWAP_30FPS) {
    return "30 FPS";
  } else {
    return "20 FPS";  // The last one would be SWAPPY_SWAP_20FPS
  }
}
}
