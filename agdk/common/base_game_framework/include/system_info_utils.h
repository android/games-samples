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

#ifndef BASEGAMEFRAMEWORK_SYSTEMINFOUTILS_H_
#define BASEGAMEFRAMEWORK_SYSTEMINFOUTILS_H_

#include <string>

#include "platform_defines.h"

namespace base_game_framework {

/**
 * @brief The base class definition for the `SystemInfoUtils` class of BaseGameFramework.
 * This class consists entirely of static utility functions and does not require
 * instance initialization. However, its functions should not be called prior to
 * calling ::BaseGameFramework_Init
 */
 class SystemInfoUtils {
 public:
/**
 * @brief Retrieve the application package name
 * @return A string containing the application package name (i.e. com.foo.mygame)
 */
  static const std::string &GetApplicationPackageName();

 private:
  static std::string application_package_name_;
};
} // namespace base_game_framework

#endif // BASEGAMEFRAMEWORK_SYSTEMINFOUTILS_H_
