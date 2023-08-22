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

#include "vibration_helper.hpp"
#include "android/content/context.hpp"
#include "android/os/vibrator_manager.hpp"
#include <android/api-level.h>
#include <gni/gni.h>
#include <jni.h>

// Add a VibrationAttribute to the vibrate call on API 33 or higher
#define USE_VIBRATION_ATTRIBUTE_API_LEVEL 33

// Use VibrationManager to obtain a vibrator on API 31 and higher
#define USE_VIBRATION_MANAGER_API_LEVEL 31

// Use the VibrationEffect version of the vibrate call on API 26 or higher
#define USE_VIBRATION_EFFECT_API_LEVEL 26

// android.os.VibrationAttributes.USAGE_MEDIA
#define USAGE_MEDIA 19

// android.os.VibrationEffect.EFFECT_HEAVY_CLICK
#define EFFECT_HEAVY_CLICK 5

// Amount of time to vibrate in milliseconds using the legacy vibrate method
#define VIBRATION_TIME_MILLISECONDS 30

VibrationHelper::VibrationHelper(jobject mainActivity, jobject vibratorString,
                                 jobject vibrationManagerString) {
  mHasVibrator = false;
  mVibrator = NULL;
  mVibrationAttributes = NULL;
  mVibrationEffect = NULL;

  // We need to create a Vibrator. Depending on the version of Android, we either
  // do this directly from getSystemService, or use getSystemService to retrieve
  // the VibratorManager, and use VibratorManager to get the default Vibrator
  android::content::Context context(mainActivity);

  int apiLevel = android_get_device_api_level();
  if (apiLevel >= USE_VIBRATION_MANAGER_API_LEVEL) {
    java::lang::String systemServiceName(vibrationManagerString);
    java::lang::Object& vibratorManagerAsObject = context.getSystemService(systemServiceName);
    android::os::VibratorManager vibratorManager(vibratorManagerAsObject.GetImpl());
    mVibrator = &vibratorManager.getDefaultVibrator();
  } else {
    java::lang::String systemServiceName(vibratorString);
    java::lang::Object& vibratorAsObject = context.getSystemService(systemServiceName);
    mVibrator = new android::os::Vibrator(vibratorAsObject.GetImpl());
  }

  if (mVibrator != NULL) {
    // Check if we actually have a vibrator on this device
    mHasVibrator = mVibrator->hasVibrator();
    if (mHasVibrator) {
      // There are a couple different ways to call vibrate on a vibrator. Depending on the
      // version of Android, some may have been deprecated. We pick the appropriate
      // combination based on API level
      if (apiLevel >= USE_VIBRATION_ATTRIBUTE_API_LEVEL) {
        const int32_t usage = USAGE_MEDIA;
        mVibrationAttributes = &android::os::VibrationAttributes::createForUsage(
            usage);
      }
      if (apiLevel >= USE_VIBRATION_EFFECT_API_LEVEL) {
        const int32_t effect = EFFECT_HEAVY_CLICK;
        mVibrationEffect = &android::os::VibrationEffect::createPredefined(effect);
      }
    }
  }
}

VibrationHelper::~VibrationHelper() {
  if (mVibrationEffect != NULL) {
    delete(mVibrationEffect);
  }
  if (mVibrationAttributes != NULL) {
    delete(mVibrationAttributes);
  }
  if (mVibrator != NULL) {
    delete mVibrator;
  }
}

void VibrationHelper::DoVibrateEffect() {
  if (mVibrator != NULL && mHasVibrator) {
    if (mVibrationEffect != NULL && mVibrationAttributes != NULL) {
      mVibrator->vibrate(*mVibrationEffect, *mVibrationAttributes);
    } else if (mVibrationEffect != NULL) {
      mVibrator->vibrate(*mVibrationEffect);
    } else {
      const int64_t duration = VIBRATION_TIME_MILLISECONDS;
      mVibrator->vibrate(duration);
    }
  }
}