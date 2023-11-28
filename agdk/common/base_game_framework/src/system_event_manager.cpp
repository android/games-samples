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

#include "system_event_manager.h"

namespace base_game_framework {

std::unique_ptr<SystemEventManager> SystemEventManager::instance_ = nullptr;

SystemEventManager &SystemEventManager::GetInstance() {
  if (!instance_) {
    instance_ = std::unique_ptr<SystemEventManager>(new SystemEventManager());
  }
  return *instance_;
}

void SystemEventManager::ShutdownInstance() {
  SystemEventManager::instance_.reset();
}

SystemEventManager::SystemEventManager() {
}

void SystemEventManager::OnFocusEvent(const FocusEvent focus_event) {
  if (focus_event_callback_) {
    focus_event_callback_(focus_event, focus_event_user_data_);
  }
}

void SystemEventManager::OnLifecycleEvent(const LifecycleEvent lifecycle_event) {
  if (lifecycle_event_callback_) {
    lifecycle_event_callback_(lifecycle_event, lifecycle_event_user_data_);
  }
}

void SystemEventManager::OnMemoryEvent(const MemoryWarningEvent memory_warning_event) {
  if (memory_warning_event_callback_) {
    memory_warning_event_callback_(memory_warning_event, memory_warning_event_user_data_);
  }
}

void SystemEventManager::OnReadSaveStateEvent(const SaveState &save_state) {
  if (read_save_state_callback_) {
    read_save_state_callback_(save_state, read_save_state_user_data_);
  }
}

void SystemEventManager::SetFocusEventCallback(FocusEventCallback callback, void *user_data) {
  focus_event_callback_ = callback;
  focus_event_user_data_ = user_data;
}

void SystemEventManager::SetLifecycleEventCallback(LifecycleEventCallback callback,
                                                   void *user_data) {
  lifecycle_event_callback_ = callback;
  lifecycle_event_user_data_ = user_data;
}

void SystemEventManager::SetMemoryWarningEventCallback(MemoryWarningEventCallback callback,
                                                       void *user_data) {
  memory_warning_event_callback_ = callback;
  lifecycle_event_user_data_ = user_data;
}

void SystemEventManager::SetReadSaveStateCallback(ReadSaveStateCallback callback, void *user_data) {
  read_save_state_callback_ = callback;
  read_save_state_user_data_ = user_data;
}

} // namespace base_game_framework