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

#ifndef ANDROID_OS_VIBRATIONATTRIBUTES_H_
#define ANDROID_OS_VIBRATIONATTRIBUTES_H_

#include <cstdint>
#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VibrationAttributes_ VibrationAttributes;

/// Wraps a JNI reference with VibrationAttributes object.
/// @param jobj A JNI reference to be wrapped with VibrationAttributes object.
/// @see VibrationAttributes_destroy
VibrationAttributes* VibrationAttributes_wrapJniReference(jobject jobj);

jobject VibrationAttributes_getJniReference(const VibrationAttributes* vibration_attributes);

/// Destroys vibration_attributes and all internal resources related to it. This function should be
/// called when vibration_attributes is no longer needed.
/// @param vibration_attributes An object to be destroyed.
void VibrationAttributes_destroy(const VibrationAttributes* vibration_attributes);

VibrationAttributes* VibrationAttributes_createForUsage(int32_t usage);

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // ANDROID_OS_VIBRATIONATTRIBUTES_H_
