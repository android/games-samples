/*
 * Copyright 2023 Google LLC
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

#ifndef BASEGAMEFRAMEWORK_GAME_CONTROLLER_MANAGER_H_
#define BASEGAMEFRAMEWORK_GAME_CONTROLLER_MANAGER_H_

#include <cstdint>
#include <functional>
#include "platform_defines.h"
#include "game_controller.h"

namespace base_game_framework {

/**
 * @brief The base class definition for the `GameControllerManager` class of BaseGameFramework.
 * This class is used to get status about and read data from connected game controllers
 */
class GameControllerManager {
  friend class UserInputManager;
 public:
  /** @brief Constant specifying the maximum number of active controllers */
  static constexpr int32_t kMaxControllers = 8;

  /**
   * @brief Definition of the GameControllerStatusCallback to be used
   * with ::SetGameControllerStatusCallback
   */
  typedef std::function<void(const int32_t controller_index,
                             const GameController::GameControllerStatus status, void *user_data)>
      GameControllerStatusCallback;

  /**
   * @brief Definition of the GameControllerMotionDataCallback to be used
   * with ::SetGameControllerMotionDataCallback
   */
  typedef std::function<void(const GameController &controller,
                             const GameController::GameControllerMotionData &motion_data,
                             void *user_data)> GameControllerMotionDataCallback;

  /**
   * @brief Definition of the IntegratedMotionDataCallback to be used
   * with ::SetIntegratedMotionDataCallback
   */
  typedef std::function<void(const GameController::GameControllerMotionData &motion_data,
                             void *user_data)> IntegratedMotionDataCallback;

/**
 * @brief Class destructor, do not call directly
 */
  ~GameControllerManager();

/**
 * @brief Retrieve whether this device supports game controllers
 * @return true if game controllers are supported, false if not
 */
  bool GetControllerSupport() const { return initialized_; }

/**
 * @brief Retrieve whether this device supports reporting integrated device
 * (i.e. handset, not controller) motion  data
 * @return true if the device can report motion data, false if not
 */
  bool GetIntegratedMotionDataSupport() { return integrated_motion_data_; }

/**
 * @brief Retrieves the specified `GameController` object
 * @param controller_index Index of the controller, must be a value between 0 and
 * `kMaxControllers`. An index outside this value will be treated as index 0.
 * @return A `GameController` object
 */
  GameController &GetGameController(const int32_t controller_index);

/**
 * @brief Set a callback to be used to report motion data from integrated
 * sensors on the device (not on game controllers).
 * @param callback A function object to use as the callback. Passing nullptr will clear
 * any currently registered callback.
 * @param user_data A pointer to user data to be passed to the callback
 */
  void SetIntegratedMotionDataCallback(
      IntegratedMotionDataCallback callback, void *user_data);

/**
 * @brief Set a callback to be used to report motion data from connected
 * game controllers.
 * @param callback A function object to use as the callback. Passing nullptr will clear
 * any currently registered callback.
 * @param user_data A pointer to user data to be passed to the callback
 */
  void SetGameControllerMotionDataCallback(
      GameControllerMotionDataCallback callback, void *user_data);

/**
 * @brief Set a callback to be used to report game controller connection and
 * disconnection status events
 * @param callback A function object to use as the callback. Passing nullptr will clear
 * any currently registered callback.
 * @param user_data A pointer to user data to be passed to the callback
 */
  void SetGameControllerStatusCallback(GameControllerStatusCallback callback, void *user_data);

/**
 * @brief Set whether the manager consumes 'back' button events from game controller devices
 * The default at initialization is true.
 * This can be set to false to allow exiting the game from a back button press
 * when the application is in an appropriate state (i.e. the title screen).
 * @param consume_back_button If true, 'back' button events are consumed,
 * if false it will pass them through.
 */
  void SetConsumeBackButton(bool consume_back_button);

  /** @brief Internal function, do not call directly */
  void MotionDataEvent(const int32_t controller_index,
                       const GameController::GameControllerMotionData &motion_data);

  /** @brief Internal function, do not call directly */
  void OnStart();

  /** @brief Internal function, do not call directly */
  void OnStop();

  /** @brief Internal function, do not call directly */
  void RefreshControllerData();

  /** @brief Internal function, do not call directly */
  void SetControllerStatus(const int32_t controller_index,
                           const GameController::GameControllerStatus status);

  /** @brief Internal function, do not call directly */
  void Update();

 private:
  GameControllerManager();

  void UpdateControllerData(const int32_t controller_index);
  void UpdateControllerInfo(const int32_t controller_index);

  GameController game_controllers_[kMaxControllers];
  GameControllerMotionDataCallback game_controller_motion_data_callback_ = nullptr;
  GameControllerStatusCallback status_callback_ = nullptr;
  IntegratedMotionDataCallback integrated_motion_data_callback_ = nullptr;
  void *callback_user_data_ = nullptr;
  void *integrated_user_data_ = nullptr;
  void *motion_user_data_ = nullptr;
  bool initialized_ = false;
  bool integrated_motion_data_ = false;

  static constexpr const char *BGM_CLASS_TAG = "BGF::GameControllerManager";
};

}

#endif // BASEGAMEFRAMEWORK_GAME_CONTROLLER_MANAGER_H_
