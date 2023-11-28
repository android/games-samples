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

#include "basegameframework_init.h"
#include "debug_manager.h"
#include "display_manager.h"
#include "filesystem_manager.h"
#include "platform_event_loop.h"
#include "platform_util_android.h"
#include "system_event_manager.h"
#include "user_input_manager.h"
#include <android/log.h>

namespace base_game_framework {

void BaseGameFramework_Init(const PlatformInitParameters &init_params) {
  JNIEnv *env = nullptr;
  if (0 != init_params.app->activity->vm->AttachCurrentThread(&env, nullptr)) {
    __android_log_print(ANDROID_LOG_ERROR, "BaseGameFramework",
                        "BaseGameFramework_Init *** FATAL ERROR: Failed to attach thread to JNI.");
    *((volatile char *) 0) = 'a';
  }

  // This is needed to allow controller events through to us.
  // By default, only touch-screen events are passed through, to match the
  // behaviour of NativeActivity.
  android_app_set_motion_event_filter(init_params.app, nullptr);

  PlatformUtilAndroid::SetMainThreadID(std::this_thread::get_id());
  PlatformUtilAndroid::SetAndroidApp(init_params.app);
  PlatformUtilAndroid::SetMainThreadJniEnv(env);
  PlatformUtilAndroid::SetActivityClassObject(init_params.app->activity->javaGameActivity);

  DebugManager::GetInstance();
  FilesystemManager::GetInstance();
  PlatformEventLoop::GetInstance();
  SystemEventManager::GetInstance();
  UserInputManager::GetInstance();
  DisplayManager::GetInstance();
}

void BaseGameFramework_Destroy() {
  DisplayManager::ShutdownInstance();
  UserInputManager::ShutdownInstance();
  SystemEventManager::ShutdownInstance();
  PlatformEventLoop::ShutdownInstance();
  FilesystemManager::ShutdownInstance();
  DebugManager::ShutdownInstance();
  android_app *app = PlatformUtilAndroid::GetAndroidApp();
  app->activity->vm->DetachCurrentThread();
}

} // namespace basegameframework