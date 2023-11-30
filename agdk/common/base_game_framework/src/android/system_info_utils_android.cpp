/*
 * Copyright 2023 Google LLC
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

#include "system_info_utils.h"
#include "platform_util_android.h"

namespace base_game_framework {
std::string SystemInfoUtils::application_package_name_;

const std::string &SystemInfoUtils::GetApplicationPackageName() {
  if (application_package_name_.size() == 0) {
    JNIEnv *env = PlatformUtilAndroid::GetMainThreadJNIEnv();
    jobject activity_object = PlatformUtilAndroid::GetActivityClassObject();
    if (env != nullptr && activity_object != nullptr) {
      jclass activity_class = env->GetObjectClass(activity_object);
      jmethodID method_getpackagename = env->GetMethodID(activity_class, "getPackageName",
                                                         "()Ljava/lang/String;");
      jstring package_name_jstring = static_cast<jstring>(
          env->CallObjectMethod(activity_object, method_getpackagename));
      const char *package_name = env->GetStringUTFChars(package_name_jstring, NULL);
      if (package_name != NULL) {
        application_package_name_ = package_name;
      }
      env->DeleteLocalRef(activity_class);
      env->DeleteLocalRef(package_name_jstring);
    }
  }
  return application_package_name_;
}

} // namespace base_game_framework
