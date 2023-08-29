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

#include <android/log.h>
#include "platform_util_log.h"

namespace base_game_framework {

void PlatformUtilLog::Log(const DebugManager::DebugLogLevel log_level,
                          const char* tag,
                          const char* string) {
  __android_log_write(log_level, tag, string);
}

void PlatformUtilLog::Log(const DebugManager::DebugLogLevel log_level,
                          const char* tag,
                          const char* format,
                          va_list specifiers) {
  __android_log_vprint(log_level, tag, format, specifiers);
}

} // namespace base_game_framework
