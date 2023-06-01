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

#ifndef ANDROID_OS_VIBRATOR_H_
#define ANDROID_OS_VIBRATOR_H_

#include <cstdint>
#include <jni.h>
#include "android/os/vibration_attributes.h"
#include "android/os/vibration_effect.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Vibrator_ Vibrator;

/// Wraps a JNI reference with Vibrator object.
/// @param jobj A JNI reference to be wrapped with Vibrator object.
/// @see Vibrator_destroy
Vibrator* Vibrator_wrapJniReference(jobject jobj);

jobject Vibrator_getJniReference(const Vibrator* vibrator);

/// Destroys vibrator and all internal resources related to it. This function should be
/// called when vibrator is no longer needed.
/// @param vibrator An object to be destroyed.
void Vibrator_destroy(const Vibrator* vibrator);

void Vibrator_cancel(const Vibrator* vibrator);

bool Vibrator_hasVibrator(const Vibrator* vibrator);

void Vibrator_vibrate(const Vibrator* vibrator, int64_t milliseconds);

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // ANDROID_OS_VIBRATOR_H_
