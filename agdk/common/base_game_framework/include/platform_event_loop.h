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

#ifndef BASEGAMEFRAMEWORK_PLATFORM_EVENT_LOOP_H_
#define BASEGAMEFRAMEWORK_PLATFORM_EVENT_LOOP_H_

#include <cstdint>
#include <memory>
#include "platform_defines.h"

namespace base_game_framework {

/**
 * @brief The base class definition for the `PlatformEventLoop` class of BaseGameFramework.
 * This class is used to process pending platform specific event data. The game should
 * generally call the ::PollEvents function in its primary game loops to respond to
 * system events.
 */
class PlatformEventLoop {
 public:
/**
 * @brief Retrieve an instance of the `PlatformEventLoop`. The first time this is called
 * it will construct and initialize the manager.
 * @return Reference to the `PlatformEventLoop` class.
 */
  static PlatformEventLoop &GetInstance();

/**
 * @brief Shuts down the `PlatformEventLoop`.
 */
  static void ShutdownInstance();

/**
 * @brief Class destructor, do not call directly, use ::ShutdownInstance.
 */
   ~PlatformEventLoop();

/**
 * @brief Processes and pending system events and handles dispatch to the
 * appropriate manager subsystems. This should be called by your game loops.
 */
  void PollEvents();

  PlatformEventLoop(const PlatformEventLoop &) = delete;
  PlatformEventLoop &operator=(const PlatformEventLoop &) = delete;
 private:
  PlatformEventLoop();

  std::unique_ptr<PlatformEventLoopData> platform_data_ = nullptr;

  static std::unique_ptr<PlatformEventLoop> instance_;
  static constexpr const char *BGM_CLASS_TAG = "BGF::PlatformEventLoop";
};

}  // namespace base_game_framework

#endif // BASEGAMEFRAMEWORK_PLATFORM_EVENT_LOOP_H_
