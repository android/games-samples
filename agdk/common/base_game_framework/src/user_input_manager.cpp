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

#include "user_input_manager.h"

namespace base_game_framework {

std::unique_ptr<UserInputManager> UserInputManager::instance_ = nullptr;

UserInputManager &UserInputManager::GetInstance() {
  if (!instance_) {
    instance_ = std::unique_ptr<UserInputManager>(new UserInputManager());
  }
  return *instance_;
}

void UserInputManager::ShutdownInstance() {
  UserInputManager::instance_.reset();
}

UserInputManager::UserInputManager()
    : key_event_callback_(nullptr),
      key_event_user_data_(nullptr),
      touch_event_callback_(nullptr),
      touch_event_user_data_(nullptr) {
  game_controller_manager_ = std::unique_ptr<GameControllerManager>(new GameControllerManager());
}

bool UserInputManager::OnKeyEvent(const KeyEvent &key_event) {
  if (key_event_callback_) {
    return key_event_callback_(key_event, key_event_user_data_);
  }
  return false;
}

bool UserInputManager::OnTouchEvent(const TouchEvent &touch_event) {
  if (touch_event_callback_) {
    return touch_event_callback_(touch_event, touch_event_user_data_);
  }
  return false;
}

void UserInputManager::SetKeyEventCallback(KeyEventCallback callback, void *user_data) {
  key_event_callback_ = callback;
  key_event_user_data_ = user_data;
}

void UserInputManager::SetTouchEventCallback(TouchEventCallback callback, void *user_data) {
  touch_event_callback_ = callback;
  touch_event_user_data_ = user_data;
}

}