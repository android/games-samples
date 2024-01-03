/*
 * Copyright 2023 The Android Open Source Project
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

#ifndef agdktunnel_tunnel_engine_hpp
#define agdktunnel_tunnel_engine_hpp

#include "native_engine.hpp"

#include "gfx_manager.hpp"
#include "game_asset_manager.hpp"
#include "tuning_manager.hpp"
#include "vibration_helper.hpp"
#include "data_loader_machine.hpp"
#include "java/lang/string.h"
#include "texture_manager.hpp"

#define INPUT_ACTION_COUNT 6

class TunnelEngine : public NativeEngine {
 public:
  // returns the (singleton) instance
  static TunnelEngine *GetInstance();

  TunnelEngine(struct android_app *app);

  virtual ~TunnelEngine();

  virtual void GameLoop();

  virtual bool CheckRenderPrerequisites();

  virtual void DoFirstFrameSetup();

  virtual void ScreenSizeChanged();

  // returns the asset manager instance
  GameAssetManager *GetGameAssetManager() { return mGameAssetManager; }

  // returns the graphics resource manager instance
  GfxManager *GetGfxManager() { return mGfxManager; }

  // returns the texture manager instance
  TextureManager *GetTextureManager() { return mTextureManager; }

  // returns the tuning manager instance
  TuningManager *GetTuningManager() { return mTuningManager; }

  // returns the vibration helper instance
  VibrationHelper *GetVibrationHelper() { return mVibrationHelper; }

  // Load data from cloud if it is enabled, or from local data otherwise
  DataLoaderStateMachine *BeginSavedGameLoad();

  // Return the data loader state machine for cloud save
  DataLoaderStateMachine *GetDataStateMachine() { return mDataStateMachine; }

  // Returns if cloud save is enabled
  bool IsCloudSaveEnabled() { return mCloudSaveEnabled; }

  // Saves data to local storage and to cloud if it is enabled
  bool SaveProgress(int level, bool forceSave = false);

  void SetInputSdkContext(int context);

 private:
  void PollGameController();

  void InitializeGfxManager();

  // Save the checkpoint level in the cloud
  void SaveGameToCloud(int level);

  // returns whether or not this level is a "checkpoint level" (that is,
  // where progress should be saved)
  bool IsCheckpointLevel(int level) {
    return 0 == level % LEVELS_PER_CHECKPOINT;
  }

  // Game asset manager instance
  GameAssetManager *mGameAssetManager;

  // Texture manager instance
  TextureManager *mTextureManager;

  // Tuning manager instance
  TuningManager *mTuningManager;

  // Gfx resource manager instance
  GfxManager *mGfxManager;

  // Vibration helper instance
  VibrationHelper *mVibrationHelper;

  // state machine instance to query the status of the current load of data
  DataLoaderStateMachine *mDataStateMachine;

  // is cloud save enabled
  bool mCloudSaveEnabled;

  // Util functions, action mapping and state data for game controller inputs
  bool mJoyKeyState[OURKEY_COUNT];
  uint32_t mPrevButtonsDown = 0;

  struct InputAction {
    uint32_t buttonMask;
    int32_t actionCode;
  };

  static constexpr InputAction INPUT_ACTIONS[INPUT_ACTION_COUNT] = {
      {GameController::kButton_A,          OURKEY_ENTER},
      {GameController::kButton_B,          OURKEY_ESCAPE},
      {GameController::kButton_Dpad_Up,    OURKEY_UP},
      {GameController::kButton_Dpad_Left,  OURKEY_LEFT},
      {GameController::kButton_Dpad_Down,  OURKEY_DOWN},
      {GameController::kButton_Dpad_Right, OURKEY_RIGHT}
  };

  void ReportJoyKeyState(int keyCode, bool state);
  void ReportJoyKeyStatesFromAxis(float x, float y);
  void CheckControllerButton(const uint32_t buttonsDown, const InputAction &inputAction);
};

#endif // agdktunnel_tunnel_engine_hpp
