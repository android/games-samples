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

void Java_com_android_example_games_GameModeManager_uninitializeGameModeManager(
    JNIEnv* env, jobject obj) {
  GameModeManager& gmm = GameModeManager::getInstance();
  gmm.Uninitialize();
}

void GameModeManager::SetGameState(bool is_loading,
                                   GAME_STATE_DEFINITION game_state) {
  if (android_get_device_api_level() >= 33) {
    ALOGI("GameModeManager::SetGameState: %d => %d", is_loading, game_state);

    JNIEnv* env = NativeEngine::GetInstance()->GetJniEnv();

    jclass cls_gamestate = env->FindClass("android/app/GameState");

    jmethodID ctor_gamestate =
        env->GetMethodID(cls_gamestate, "<init>", "(ZI)V");
    jobject obj_gamestate = env->NewObject(
        cls_gamestate, ctor_gamestate, (jboolean)is_loading, (jint)game_state);

    env->CallVoidMethod(obj_gamemanager_, gamemgr_setgamestate_, obj_gamestate);

    env->DeleteLocalRef(obj_gamestate);
    env->DeleteLocalRef(cls_gamestate);
  }
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

// Invoke the API first to set the android_app instance.
void GameModeManager::SetApplication(android_app* app) {
  app_.reset(app);

  // Initialize JNI reference.
  Initialize();
}

void GameModeManager::Initialize() {
  JNIEnv* env = NativeEngine::GetInstance()->GetJniEnv();

  jclass context = env->FindClass("android/content/Context");
  jclass gamemgr = env->FindClass("android/app/GameManager");

  jmethodID mid_getss = env->GetMethodID(
      context, "getSystemService", "(Ljava/lang/Class;)Ljava/lang/Object;");
  jobject obj_gamemanager = env->CallObjectMethod(
      app_->activity->javaGameActivity, mid_getss, gamemgr);

  gamemgr_setgamestate_ =
      env->GetMethodID(gamemgr, "setGameState", "(Landroid/app/GameState;)V");

  obj_gamemanager_ = env->NewGlobalRef(obj_gamemanager);

  env->DeleteLocalRef(obj_gamemanager);
  env->DeleteLocalRef(gamemgr);
  env->DeleteLocalRef(context);
}

void GameModeManager::Uninitialize() {
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
