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

#ifndef BASEGAMEFRAMEWORK_SYSTEMEVENTMANAGER_H_
#define BASEGAMEFRAMEWORK_SYSTEMEVENTMANAGER_H_

#include <cstdint>
#include <functional>
#include <memory>

namespace base_game_framework {

/**
 * @brief The base class definition for the `SystemEventManager` class of BaseGameFramework.
 * This class is used to register callbacks to receive system events
 */
class SystemEventManager {
  // Internal platform specific implementation that dispatches system events
  // to the SystemEventManager
  friend class PlatformSystemEventDispatch;
 public:

  /** @brief Enum of system focus events */
  enum FocusEvent : int32_t {
    kMadeForeground = 0, ///< Game brought to foreground
    kSentToBackground ///< Game sent to background
  };

  /** @brief Enum of system lifecycle events */
  enum LifecycleEvent : int32_t {
    kLifecycleStart = 0, ///< Game received system start event
    kLifecycleResume, ///< Game received system resume event
    kLifecyclePause, ///< Game received system pause event
    kLifecycleStop, ///< Game received system stop event
    kLifecycleQuit, ///< Game received system quit event
    kLifecycleSaveState ///< Game received system saved game state event
  };

  /** @brief Enum of system memory warming events */
  enum MemoryWarningEvent : int32_t {
    kMemoryWarningLow = 0, ///< Game received system low memory warning event
    kMemoryWarningCritical ///< Game received system critically low memory warning event
  };

  /** @brief Enum of system saved game state events types */
  enum SaveStateEvent : int32_t {
    kSaveStateRead = 0, ///< Game received read saved game state event
    kSaveStateWrite ///< Game received write saved game state event
  };

/**
 * @brief Structure defining the elements of SaveState event data
 * The lifetime of the state_data pointer is not guaranteed to persist past the
 * exit from the SaveState callback or beyond return from the ::WriteSaveState function
 */
  struct SaveState {
    void *state_data; ///< Pointer to the saved state data
    size_t state_size; ///< Size of the saved state data in bytes
  };

/**
  * @brief Set a callback to be called when a focus change event occurs.
  * @param callback A function object to use as the callback. Passing nullptr will clear
  * any currently registered callback.
  * @param user_data A pointer to user data to be passed to the callback
  */
  typedef std::function<void(const FocusEvent focus_event, void *user_data)>
      FocusEventCallback;

/**
  * @brief Set a callback to be called when a lifecycle event occurs.
  * @param callback A function object to use as the callback. Passing nullptr will clear
  * any currently registered callback.
  * @param user_data A pointer to user data to be passed to the callback
  */
  typedef std::function<void(const LifecycleEvent lifecycle_event, void *user_data)>
      LifecycleEventCallback;

/**
  * @brief Set a callback to be called when a memory warning event occurs.
  * @param callback A function object to use as the callback. Passing nullptr will clear
  * any currently registered callback.
  * @param user_data A pointer to user data to be passed to the callback
  */
  typedef std::function<void(const MemoryWarningEvent memory_event, void *user_data)>
      MemoryWarningEventCallback;

/**
  * @brief Set a callback to be called when a read save state event occurs.
  * @param callback A function object to use as the callback. Passing nullptr will clear
  * any currently registered callback.
  * @param user_data A pointer to user data to be passed to the callback
  */
  typedef std::function<void(const SaveState &save_state, void *user_data)> ReadSaveStateCallback;

  static SystemEventManager &GetInstance();

  static void ShutdownInstance();

/**
  * @brief Set a callback to be called when a focus change event occurs.
  * @param callback A function object to use as the callback. Passing nullptr will clear
  * any currently registered callback.
  * @param user_data A pointer to user data to be passed to the callback
  */
  void SetFocusEventCallback(FocusEventCallback callback, void *user_data);

/**
  * @brief Set a callback to be called when a lifecycle event occurs.
  * @param callback A function object to use as the callback. Passing nullptr will clear
  * any currently registered callback.
  * @param user_data A pointer to user data to be passed to the callback
  */
  void SetLifecycleEventCallback(LifecycleEventCallback callback, void *user_data);

/**
  * @brief Set a callback to be called when a memory warning event occurs.
  * @param callback A function object to use as the callback. Passing nullptr will clear
  * any currently registered callback.
  * @param user_data A pointer to user data to be passed to the callback
  */
  void SetMemoryWarningEventCallback(MemoryWarningEventCallback callback, void *user_data);

/**
  * @brief Set a callback to be called when a read saved state event occurs.
  * @param callback A function object to use as the callback. Passing nullptr will clear
  * any currently registered callback.
  * @param user_data A pointer to user data to be passed to the callback
  */
  void SetReadSaveStateCallback(ReadSaveStateCallback callback, void *user_data);

/**
  * @brief Write save state information to the device
  * @param save_state A reference to a `SaveState` structure defining the save state information
  */
  void WriteSaveState(const SaveState &save_state);

/**
 * @brief Class destructor, do not call directly, use ::ShutdownInstance.
 */
  ~SystemEventManager() = default;

  SystemEventManager(const SystemEventManager &) = delete;
  SystemEventManager &operator=(const SystemEventManager &) = delete;

 private:
  SystemEventManager();

  void OnFocusEvent(const FocusEvent focus_event);
  void OnLifecycleEvent(const LifecycleEvent lifecycle_event);
  void OnMemoryEvent(const MemoryWarningEvent memory_warning_event);
  void OnReadSaveStateEvent(const SaveState &save_state);

  FocusEventCallback focus_event_callback_ = nullptr;
  void *focus_event_user_data_ = nullptr;

  LifecycleEventCallback lifecycle_event_callback_ = nullptr;
  void *lifecycle_event_user_data_ = nullptr;

  MemoryWarningEventCallback memory_warning_event_callback_ = nullptr;
  void *memory_warning_event_user_data_ = nullptr;

  ReadSaveStateCallback read_save_state_callback_ = nullptr;
  void *read_save_state_user_data_ = nullptr;

  static std::unique_ptr<SystemEventManager> instance_;
};

} // namespace base_game_framework

#endif //BASEGAMEFRAMEWORK_SYSTEMEVENTMANAGER_H_
