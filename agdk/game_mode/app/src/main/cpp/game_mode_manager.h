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

#ifndef GAME_MODE_MANAGER_H_
#define GAME_MODE_MANAGER_H_

#include <android/api-level.h>
#include <android/log.h>
#include <android/thermal.h>

#include <memory>

#include "common.h"
#include "native_engine.h"
#include "util.h"

enum GAME_MODE_DEFINITION {
    GAME_MODE_UNSUPPORTED = 0,
    GAME_MODE_STANDARD = 1,
    GAME_MODE_PERFORMANCE = 2,
    GAME_MODE_BATTERY = 3,
};

enum GAME_STATE_DEFINITION {
    GAME_STATE_UNKNOWN = 0,
    GAME_STATE_NONE = 1,
    GAME_STATE_GAMEPLAY_INTERRUPTIBLE = 2,
    GAME_STATE_GAMEPLAY_UNINTERRUPTIBLE = 3,
    GAME_STATE_CONTENT = 4,
};

/*
 * GameModeManager class manages the GameMode APIs.
 */
class GameModeManager {
 public:
  // Singleton function.
  static GameModeManager& getInstance() {
    static GameModeManager instance;
    return instance;
  }
  // Dtor. Remove global reference (if any).
  ~GameModeManager() {}

  // Delete copy constructor since the class is used as a singleton.
  GameModeManager(GameModeManager const&) = delete;
  void operator=(GameModeManager const&) = delete;

  // Invoke the API first to set the android_app instance.
  void SetApplication(android_app* app);
  void Initialize();
  void Uninitialize();

  void SetGameMode(int game_mode);
  int GetGameMode();

  void SetGameState(bool is_loading, GAME_STATE_DEFINITION game_state);

  const char* GetGameModeString();
  const char* GetFPSString(int32_t swappy_swap_interval);

 private:
  // Ctor. It's private since the class is designed as a singleton.
  GameModeManager() {}

  int game_mode_ = 0;

  std::shared_ptr<android_app> app_;

  jobject obj_gamemanager_;
  jmethodID gamemgr_setgamestate_;
};

#endif  // GAME_MODE_MANAGER_H_
