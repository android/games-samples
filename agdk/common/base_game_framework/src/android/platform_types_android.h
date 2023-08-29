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

#ifndef BASEGAMEFRAMEWORK_PLATFORM_TYPES_ANDROID_H_
#define BASEGAMEFRAMEWORK_PLATFORM_TYPES_ANDROID_H_

#include <cstdint>

struct android_app;

namespace base_game_framework {

// The first two axis, X and Y, are always active by default
static constexpr uint64_t kDefaultAxisMask = 3;

struct PlatformEventLoopData {
  uint64_t active_axis_ = kDefaultAxisMask;
  android_app* android_app_ = nullptr;
};

struct PlatformInitParameters {
  android_app* app;
};

} // namespace basegameframework

#endif // BASEGAMEFRAMEWORK_PLATFORM_TYPES_ANDROID_H_
