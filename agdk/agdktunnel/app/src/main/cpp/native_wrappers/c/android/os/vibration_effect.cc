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

#include "android/os/vibration_effect.h"

#include "android/os/vibration_effect.hpp"

VibrationEffect* VibrationEffect_wrapJniReference(jobject jobj) {
  return reinterpret_cast<VibrationEffect*>(new ::android::os::VibrationEffect(jobj));
}

jobject VibrationEffect_getJniReference(const VibrationEffect* vibration_effect) {
  return reinterpret_cast<const ::android::os::VibrationEffect*>(vibration_effect)->GetImpl();
}

void VibrationEffect_destroy(const VibrationEffect* vibration_effect) {
  ::android::os::VibrationEffect::destroy(reinterpret_cast<const ::android::os::VibrationEffect*>(vibration_effect));
}

VibrationEffect* VibrationEffect_createPredefined(int32_t effect_id) {
  return reinterpret_cast<VibrationEffect*>(&::android::os::VibrationEffect::createPredefined(effect_id));
}
