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

#ifndef CPP_ANDROID_OS_VIBRATIONEFFECT
#define CPP_ANDROID_OS_VIBRATIONEFFECT

#include <memory>
#include "gni/common/scoped_local_ref.h"
#include "gni/gni.hpp"
#include "gni/object.hpp"

namespace android {
namespace os {

class VibrationEffect : virtual public ::gni::Object
{
public:
    static jclass GetClass();
    static void destroy(const VibrationEffect* object);
    explicit VibrationEffect(jobject object) : ::gni::Object(object) {}
    ~VibrationEffect() override = default;
    static ::android::os::VibrationEffect& createPredefined(int32_t effectId);
};

}  // namespace os
}  // namespace android

#endif  // CPP_ANDROID_OS_VIBRATIONEFFECT

