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

#ifndef CPP_ANDROID_OS_VIBRATOR
#define CPP_ANDROID_OS_VIBRATOR

#include <memory>
#include "android/os/vibration_attributes.hpp"
#include "android/os/vibration_effect.hpp"
#include "gni/common/scoped_local_ref.h"
#include "gni/gni.hpp"
#include "gni/object.hpp"

namespace android {
namespace os {

class Vibrator : virtual public ::gni::Object
{
public:
    static jclass GetClass();
    static void destroy(const Vibrator* object);
    explicit Vibrator(jobject object) : ::gni::Object(object) {}
    ~Vibrator() override = default;
    virtual void cancel() const;
    virtual bool hasVibrator() const;
    virtual void vibrate(int64_t milliseconds) const;
    virtual void vibrate(const ::android::os::VibrationEffect& vibe) const;
    virtual void vibrate(const ::android::os::VibrationEffect& vibe, const ::android::os::VibrationAttributes& attributes) const;
};

}  // namespace os
}  // namespace android

#endif  // CPP_ANDROID_OS_VIBRATOR

