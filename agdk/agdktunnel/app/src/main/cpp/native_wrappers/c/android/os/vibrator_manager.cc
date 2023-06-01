/*
 * Copyright 2023 The Android Open Source Project
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

// This file was generated using the Library Wrapper tool
// https://developer.android.com/games/develop/custom/wrapper

#include "android/os/vibrator_manager.h"

#include "android/os/vibrator_manager.hpp"

VibratorManager* VibratorManager_wrapJniReference(jobject jobj) {
  return reinterpret_cast<VibratorManager*>(new ::android::os::VibratorManager(jobj));
}

jobject VibratorManager_getJniReference(const VibratorManager* vibrator_manager) {
  return reinterpret_cast<const ::android::os::VibratorManager*>(vibrator_manager)->GetImpl();
}

void VibratorManager_destroy(const VibratorManager* vibrator_manager) {
  ::android::os::VibratorManager::destroy(reinterpret_cast<const ::android::os::VibratorManager*>(vibrator_manager));
}

Vibrator* VibratorManager_getDefaultVibrator(const VibratorManager* vibrator_manager) {
  return reinterpret_cast<Vibrator*>(&reinterpret_cast<const ::android::os::VibratorManager*>(vibrator_manager)->getDefaultVibrator());
}
