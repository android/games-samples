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

#include "system_event_manager.h"
#include "platform_util_android.h"

namespace base_game_framework {

void SystemEventManager::WriteSaveState(const SaveState &save_state) {
  android_app *app = PlatformUtilAndroid::GetAndroidApp();
  if (app->savedState != nullptr && app->savedStateSize != save_state.state_size) {
    free(app->savedState);
    app->savedState = nullptr;
    app->savedStateSize = save_state.state_size;
  }
  if (app->savedState == nullptr) {
    app->savedState = malloc(save_state.state_size);
  }
  memcpy(app->savedState, save_state.state_data, app->savedStateSize);
}

} // namespace base_game_framework