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

#ifndef ANDROID_OS_VIBRATORMANAGER_H_
#define ANDROID_OS_VIBRATORMANAGER_H_

#include <cstdint>
#include <jni.h>
#include "android/os/vibrator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VibratorManager_ VibratorManager;

/// Wraps a JNI reference with VibratorManager object.
/// @param jobj A JNI reference to be wrapped with VibratorManager object.
/// @see VibratorManager_destroy
VibratorManager* VibratorManager_wrapJniReference(jobject jobj);

jobject VibratorManager_getJniReference(const VibratorManager* vibrator_manager);

/// Destroys vibrator_manager and all internal resources related to it. This function should be
/// called when vibrator_manager is no longer needed.
/// @param vibrator_manager An object to be destroyed.
void VibratorManager_destroy(const VibratorManager* vibrator_manager);

Vibrator* VibratorManager_getDefaultVibrator(const VibratorManager* vibrator_manager);

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // ANDROID_OS_VIBRATORMANAGER_H_
