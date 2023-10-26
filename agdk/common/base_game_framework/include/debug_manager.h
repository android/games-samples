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

#ifndef BASEGAMEFRAMEWORK_DEBUG_MANAGER_H_
#define BASEGAMEFRAMEWORK_DEBUG_MANAGER_H_

#include <cstdarg>
#include <cstdint>
#include <string>

namespace base_game_framework {

/**
 * @brief The base class definition for the `DebugManager` class of BaseGameFramework.
 * This class is used to output debug information, at configurable log levels to
 * configurable output destinations. Note that file and network logging are defined, but
 * currently not implemented on Android. Only console (logcat) output is currently
 * implemented.
 */
 class DebugManager {
 public:

   /**
    * @brief Enum bitmask values to specify active debug message destinations for a
    * debug log command.
    */
  enum DebugLogChannel : uint32_t {
    /**
     * @brief Platform-specific default channel output (generally Console),
     *  can be customized with ::SetDefaultChannels
     */
    kLog_Channel_Default = 0,
    /** @brief Output log to platform console (i.e. Android logcat) */
    kLog_Channel_Console = (1U << 0),
    /** @brief Output log to local file, currently unimplemented */
    kLog_Channel_File = (1U << 1),
    /** @brief Output log to network server, currently unimplemented */
    kLog_Channel_Network = (1U << 2)
  };

   /**
    * @brief Enum value to specify the severity of a debug log command.
    */
  enum DebugLogLevel : int32_t {
    /** @brief Platform-specific default, level, usually equivalent to Debug */
    kLog_Level_Default = 1,
    /** @brief Verbose log severity output */
    kLog_Level_Verbose,
    /** @brief Debug log severity output */
    kLog_Level_Debug,
    /** @brief Informational log severity output */
    kLog_Level_Info,
    /** @brief Warning log severity output */
    kLog_Level_Warning,
    /** @brief Error log severity output */
    kLog_Level_Error,
    /** @brief Fatal error log severity output */
    kLog_Level_Fatal
  };

/**
 * @brief Retrieve an instance of the `DebugManager`. The first time this is called
 * it will construct and initialize the manager.
 * @return Reference to the `DebugManager` class.
 */
  static DebugManager& GetInstance();

/**
 * @brief Shuts down the `DebugManager`.
 */
  static void ShutdownInstance();

/**
 * @brief Class destructor, do not call directly, use ::ShutdownInstance.
 */
  ~DebugManager() = default;

  DebugManager(const DebugManager &) = delete;
  DebugManager& operator=(const DebugManager &) = delete;

/**
 * @brief Set a filename to log debug messages to.
 * @param path A string specifying a file to use to write debug messages
 * to on the local device. If the file already exists, it will be overwritten, otherwise
 * it will be created. The file will be written in the root of the platform-specific
 * application local storage directory. Path information is not currently supported.
 * This channel is currently not implemented.
 */
  void SetDebugLogFile(const std::string& path) { log_file_path_ = path; }

/**
 * @brief Set a filename to log debug messages to.
 * @param address A string specifying a server address to use to write debug messages
 * to. This channel is currently not implemented.
 */
  void SetDebugLogServer(const std::string& address) { log_server_address_ = address; }

/**
 * @brief Set the default log message channels
 * @param defaultChannels A bitmasked value of `DebugLogChannel` values specifying
 * which channels receive messages when `kLog_Channel_Default` is
 * specified as the `log_channels` parameter of a log message.
 */
  void SetDefaultChannels(const uint32_t defaultChannels) {
    log_default_channels_ = defaultChannels;
  }

/**
 * @brief Log a debug message with variable parameters
 * @param log_channels A `DebugLogChannels` flag bitmask specifying the target channels
 * of the message
 * @param log_level A `DebugLogLevel` enum value specifying the debug level of the message
 * @param tag A string specifying a categorization tag for the debug message
 * @param format A printf style format string followed by parameter values
 */
  static void Log(const uint32_t log_channels,
           const DebugLogLevel log_level,
           const char* tag,
           const char* format,
           ...);

/**
 * @brief Log a debug message with variable parameters
 * @param log_channels A `DebugLogChannels` flag bitmask specifying the target channels
 * of the message
 * @param log_level A `DebugLogLevel` enum value specifying the debug level of the message
 * @param tag A string specifying a categorization tag for the debug message
 * @param format A printf style format string
 * @param specifiers A `va_list` of parameters for the `format` string
 */
  static void Log(const uint32_t log_channels,
           const DebugLogLevel log_level,
           const char* tag,
           const char* format,
           va_list specifiers);

/**
 * @brief Log a debug message with a string constant
 * @param log_channels A `DebugLogChannels` flag bitmask specifying the target channels
 * of the message
 * @param log_level A `DebugLogLevel` enum value specifying the debug level of the message
 * @param tag A string specifying a categorization tag for the debug message
 * @param log_string A string constant to use as the log message
 */
  static void LogC(const uint32_t log_channels,
           const DebugLogLevel log_level,
           const char* tag,
           const char* log_string);

 private:
  DebugManager();

  static bool LogToConsole(const uint32_t log_channels);

  std::string log_file_path_;
  std::string log_server_address_;
  uint32_t log_default_channels_ = kLog_Channel_Console;
  static std::unique_ptr<DebugManager> instance_;
  static constexpr const char* BGM_CLASS_TAG = "BGF::DebugManager";
};

}

#endif //BASEGAMEFRAMEWORK_DEBUG_MANAGER_H_
