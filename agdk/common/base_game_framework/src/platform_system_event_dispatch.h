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

#ifndef BASEGAMEFRAMEWORK_PLATFORMSYSTEMEVENTDISPATCH_H_
#define BASEGAMEFRAMEWORK_PLATFORMSYSTEMEVENTDISPATCH_H_

#include "system_event_manager.h"

namespace base_game_framework {

class PlatformSystemEventDispatch {
 public:
  static void DispatchFocusEvent(const SystemEventManager::FocusEvent focus_event);
  static void DispatchLifecycleEvent(const SystemEventManager::LifecycleEvent lifecycle_event);
  static void DispatchMemoryEvent(const SystemEventManager::MemoryWarningEvent
                                  memory_warning_event);
  static void DispatchReadSaveStateEvent(const SystemEventManager::SaveState &save_state);
};

} // namespace base_game_framework

#endif //BASEGAMEFRAMEWORK_PLATFORMSYSTEMEVENTDISPATCH_H_
