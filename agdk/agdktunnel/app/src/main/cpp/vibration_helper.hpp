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

#ifndef agdktunnel_vibration_helper_hpp
#define agdktunnel_vibration_helper_hpp

#include "android/os/vibrator.hpp"
#include "android/os/vibration_attributes.hpp"
#include "android/os/vibration_effect.hpp"

class VibrationHelper {
 public:
  VibrationHelper(jobject mainActivity, jobject vibratorString, jobject vibrationManagerString);
  ~VibrationHelper();

  void DoVibrateEffect();

 private:
  android::os::Vibrator *mVibrator;
  android::os::VibrationAttributes *mVibrationAttributes;
  android::os::VibrationEffect *mVibrationEffect;
  bool mHasVibrator;
};

#endif // agdktunnel_vibration_helper_hpp
