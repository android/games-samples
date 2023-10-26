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

#ifndef BASEGAMEFRAMEWORK_INIT_H_
#define BASEGAMEFRAMEWORK_INIT_H_

#include "platform_defines.h"

namespace base_game_framework {

/**
 * @brief Initialize the Base Game Framework
 * @param init_params A platform-specific structure that provides
 * the information necessary to initialize the Base Game Framework core managers
 */
void BaseGameFramework_Init(const PlatformInitParameters &init_params);

/**
 * @brief Destroy the Base Game Framework
 */
void BaseGameFramework_Destroy();

} // namespace basegameframework

#endif // BASEGAMEFRAMEWORK_INIT_H_
