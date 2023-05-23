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

#ifndef ANDROID_OS_VIBRATIONEFFECT_H_
#define ANDROID_OS_VIBRATIONEFFECT_H_

#include <cstdint>
#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VibrationEffect_ VibrationEffect;

/// Wraps a JNI reference with VibrationEffect object.
/// @param jobj A JNI reference to be wrapped with VibrationEffect object.
/// @see VibrationEffect_destroy
VibrationEffect* VibrationEffect_wrapJniReference(jobject jobj);

jobject VibrationEffect_getJniReference(const VibrationEffect* vibration_effect);

/// Destroys vibration_effect and all internal resources related to it. This function should be
/// called when vibration_effect is no longer needed.
/// @param vibration_effect An object to be destroyed.
void VibrationEffect_destroy(const VibrationEffect* vibration_effect);

VibrationEffect* VibrationEffect_createPredefined(int32_t effect_id);

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // ANDROID_OS_VIBRATIONEFFECT_H_
