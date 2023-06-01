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

#include "android/os/vibrator.h"

#include "android/os/vibrator.hpp"

Vibrator* Vibrator_wrapJniReference(jobject jobj) {
  return reinterpret_cast<Vibrator*>(new ::android::os::Vibrator(jobj));
}

jobject Vibrator_getJniReference(const Vibrator* vibrator) {
  return reinterpret_cast<const ::android::os::Vibrator*>(vibrator)->GetImpl();
}

void Vibrator_destroy(const Vibrator* vibrator) {
  ::android::os::Vibrator::destroy(reinterpret_cast<const ::android::os::Vibrator*>(vibrator));
}

void Vibrator_cancel(const Vibrator* vibrator) {
  reinterpret_cast<const ::android::os::Vibrator*>(vibrator)->cancel();
}

bool Vibrator_hasVibrator(const Vibrator* vibrator) {
  return reinterpret_cast<const ::android::os::Vibrator*>(vibrator)->hasVibrator();
}

void Vibrator_vibrate(const Vibrator* vibrator, int64_t milliseconds) {
  reinterpret_cast<const ::android::os::Vibrator*>(vibrator)->vibrate(milliseconds);
}
