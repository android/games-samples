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

#ifndef BASEGAMEFRAMEWORK_USERINPUTMANAGER_H_
#define BASEGAMEFRAMEWORK_USERINPUTMANAGER_H_

#include <cstdint>
#include <functional>
#include <memory>
#include "game_controller_manager.h"
#include "user_input_events.h"

namespace base_game_framework {

/**
 * @brief The base class definition for the `UserInputManager` class of BaseGameFramework.
 * This class is used to register callbacks to receive user input events
 */
 class UserInputManager {
  // Internal platform specific implementation that dispatches input events
  // to the UserInputManager
  friend class PlatformUserInputDispatch;

 public:

  /** @brief Definition of the KeyEventCallback to be used with ::SetKeyEventCallback */
  typedef std::function<bool(const KeyEvent &key_event, void *user_data)>
      KeyEventCallback;

  /** @brief Definition of the TouchEventCallback to be used with ::SetTouchEventCallback */
  typedef std::function<bool(const TouchEvent &touch_event, void *user_data)>
      TouchEventCallback;

/**
 * @brief Retrieve an instance of the `UserInputManager`. The first time this is called
 * it will construct and initialize the manager.
 * @return Reference to the `UserInputManager` class.
 */
  static UserInputManager &GetInstance();

/**
 * @brief Shuts down the `DisplayManager`.
 */
  static void ShutdownInstance();

/**
 * @brief Set a callback to be called when a key event occurs.
 * @param callback A function object to use as the callback. Passing nullptr will clear
 * any currently registered callback.
 * @param user_data A pointer to user data to be passed to the callback
 */
  void SetKeyEventCallback(KeyEventCallback callback, void *user_data);

/**
 * @brief Set a callback to be called when a touch event occurs.
 * @param callback A function object to use as the callback. Passing nullptr will clear
 * any currently registered callback.
 * @param user_data A pointer to user data to be passed to the callback
 */
  void SetTouchEventCallback(TouchEventCallback callback, void *user_data);

/**
 * @brief Retrieves a reference to the GameControllerManager object
 * @return A `GameControllerManager` reference
 */
  GameControllerManager &GetGameControllerManager() { return *game_controller_manager_.get(); }

/**
 * @brief Class destructor, do not call directly, use ::ShutdownInstance.
 */
  ~UserInputManager() = default;

  UserInputManager(const UserInputManager &) = delete;
  UserInputManager &operator=(const UserInputManager &) = delete;

 private:
  UserInputManager();

  bool OnKeyEvent(const KeyEvent &focus_event);
  bool OnTouchEvent(const TouchEvent &lifecycle_event);

  KeyEventCallback key_event_callback_;
  void *key_event_user_data_;

  TouchEventCallback touch_event_callback_;
  void *touch_event_user_data_;

  std::unique_ptr<GameControllerManager> game_controller_manager_;
  static std::unique_ptr<UserInputManager> instance_;
};

} // namespace base_game_framework

#endif //BASEGAMEFRAMEWORK_USERINPUTMANAGER_H_
