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

#ifndef CPP_ANDROID_OS_VIBRATIONATTRIBUTES
#define CPP_ANDROID_OS_VIBRATIONATTRIBUTES

#include <memory>
#include "gni/common/scoped_local_ref.h"
#include "gni/gni.hpp"
#include "gni/object.hpp"

namespace android {
namespace os {

class VibrationAttributes : virtual public ::gni::Object
{
public:
    static jclass GetClass();
    static void destroy(const VibrationAttributes* object);
    explicit VibrationAttributes(jobject object) : ::gni::Object(object) {}
    ~VibrationAttributes() override = default;
    static ::android::os::VibrationAttributes& createForUsage(int32_t usage);
};

}  // namespace os
}  // namespace android

#endif  // CPP_ANDROID_OS_VIBRATIONATTRIBUTES

