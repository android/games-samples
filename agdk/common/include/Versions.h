/*
 * Copyright 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <jni.h>

#include <string>

// Utility functions to get Apk and Sdk versions via JNI

namespace agdk_samples_util {

/**
   App version code and name from the app's build.gradle file.
   @param versionCode If non-null, this points to the version on return.
   @param versionName If non-null, this points to the version on return.
   @return 0 if no error, otherwise an error code.
*/
int GetAppVersionInfo(JNIEnv* env, jobject context, int* versionCode,
                      std::string* versionName);

}  // namespace agdk_samples_util
