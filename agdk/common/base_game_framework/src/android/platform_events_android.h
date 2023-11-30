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

#ifndef BASEGAMEFRAMEWORK_PLATFORM_EVENTS_ANDROID_H_
#define BASEGAMEFRAMEWORK_PLATFORM_EVENTS_ANDROID_H_

#include <cstdint>
#include <memory>
#include "platform_defines.h"
#include "game-activity/native_app_glue/android_native_app_glue.h"

namespace base_game_framework {

class PlatformEventAndroid {
 public:
  static void ProcessApplicationEvent(struct android_app *app, int32_t cmd);
  static void ProcessKeyEvent(struct android_app *app, const GameActivityKeyEvent &event);
  static void ProcessMotionEvent(struct android_app *app, const GameActivityMotionEvent &event);
  static constexpr const char *BGM_CLASS_TAG = "BGF::PlatformEventAndroid";
};

} // namespace base_game_framework

#endif //BASEGAMEFRAMEWORK_PLATFORM_EVENTS_ANDROID_H_
