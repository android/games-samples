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

#include "game_controller.h"
#include "platform_util_android.h"
#include <paddleboat/paddleboat.h>

namespace base_game_framework {

void GameController::SetControllerLight(const GameControllerLightType light_type,
                                        uint32_t light_data) {
  Paddleboat_setControllerLight(controller_index_,
                                static_cast<Paddleboat_LightType>(light_type),
                                light_data,
                                PlatformUtilAndroid::GetMainThreadJNIEnv());
}

void GameController::SetControllerVibrationData(const GameControllerVibrationData &vibration_data) {
  Paddleboat_Vibration_Data paddleboat_vibration_data {
    vibration_data.duration_left,
    vibration_data.duration_right,
    vibration_data.intensity_left,
    vibration_data.intensity_right
  };
  Paddleboat_setControllerVibrationData(controller_index_, &paddleboat_vibration_data,
                                        PlatformUtilAndroid::GetMainThreadJNIEnv());
}

} // namespace base_game_framework