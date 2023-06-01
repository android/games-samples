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

#include "android/os/vibrator.hpp"
#include <memory>
#include "android/os/vibration_attributes.hpp"
#include "android/os/vibration_effect.hpp"
#include "gni/common/scoped_local_ref.h"
#include "gni/gni.hpp"
#include "gni/object.hpp"

namespace android {
namespace os {

jclass Vibrator::GetClass()
{
  static const jclass cached_class = gni::GniCore::GetInstance()->GetClassGlobalRef("android/os/Vibrator");
  return cached_class;
}

void Vibrator::destroy(const Vibrator* object)
{
  delete object;
}

void Vibrator::cancel() const
{
  JNIEnv *env = gni::GniCore::GetInstance()->GetJniEnv();
  static const jmethodID method_id = env->GetMethodID(GetClass(), "cancel", "()V");
  env->CallVoidMethod(GetImpl(), method_id);
}

bool Vibrator::hasVibrator() const
{
  JNIEnv *env = gni::GniCore::GetInstance()->GetJniEnv();
  static const jmethodID method_id = env->GetMethodID(GetClass(), "hasVibrator", "()Z");
  bool ret = env->CallBooleanMethod(GetImpl(), method_id);
  return ret;
}

void Vibrator::vibrate(int64_t milliseconds) const
{
  JNIEnv *env = gni::GniCore::GetInstance()->GetJniEnv();
  static const jmethodID method_id = env->GetMethodID(GetClass(), "vibrate", "(J)V");
  env->CallVoidMethod(GetImpl(), method_id, milliseconds);
}

void Vibrator::vibrate(const ::android::os::VibrationEffect& vibe) const
{
  JNIEnv *env = gni::GniCore::GetInstance()->GetJniEnv();
  static const jmethodID method_id = env->GetMethodID(GetClass(), "vibrate", "(Landroid/os/VibrationEffect;)V");
  env->CallVoidMethod(GetImpl(), method_id, vibe.GetImpl());
}

void Vibrator::vibrate(const ::android::os::VibrationEffect& vibe, const ::android::os::VibrationAttributes& attributes) const
{
  JNIEnv *env = gni::GniCore::GetInstance()->GetJniEnv();
  static const jmethodID method_id = env->GetMethodID(GetClass(), "vibrate", "(Landroid/os/VibrationEffect;Landroid/os/VibrationAttributes;)V");
  env->CallVoidMethod(GetImpl(), method_id, vibe.GetImpl(), attributes.GetImpl());
}

}  // namespace os
}  // namespace android

