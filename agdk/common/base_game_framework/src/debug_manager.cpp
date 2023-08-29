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

#include "debug_manager.h"
#include "platform_util_log.h"

namespace base_game_framework {

std::unique_ptr <DebugManager> DebugManager::instance_ = nullptr;

DebugManager& DebugManager::GetInstance() {
  if (!instance_) {
    instance_ = std::unique_ptr<DebugManager>(new DebugManager());
  }
  return *instance_;
}

void DebugManager::ShutdownInstance() {
  DebugManager::instance_.reset();
}

DebugManager::DebugManager() {
}

bool DebugManager::LogToConsole(const uint32_t log_channels) {
  bool print_console = false;
  if (log_channels == kLog_Channel_Default) {
    if ((DebugManager::GetInstance().log_default_channels_ &
        DebugManager::kLog_Channel_Console) != 0) {
      print_console = true;
    }
  } else if ((log_channels & DebugManager::kLog_Channel_Console) != 0) {
    print_console = true;
  }
  return print_console;
}

void DebugManager::Log(const uint32_t log_channels,
                const DebugLogLevel log_level,
                const char* tag,
                const char* format,
                ...) {
  va_list specifiers;
  va_start(specifiers, format);

  if (LogToConsole(log_channels)) {
    PlatformUtilLog::Log(log_level, tag, format, specifiers);
  }
  va_end(specifiers);
}

void DebugManager::LogC(const uint32_t log_channels,
                const DebugLogLevel log_level,
                const char* tag,
                const char* string) {
  if (LogToConsole(log_channels)) {
    PlatformUtilLog::Log(log_level, tag, string);
  }
}

void DebugManager::Log(const uint32_t log_channels,
                const DebugLogLevel log_level,
                const char* tag,
                const char* format,
                va_list specifiers) {
  if (LogToConsole(log_channels)) {
    PlatformUtilLog::Log(log_level, tag, format, specifiers);
  }
}

} // namespace base_game_framework
