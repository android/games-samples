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

#include "platform_util_android.h"

namespace base_game_framework {
static constexpr const char* kGetAndroidFeatureSupportedName = "GetAndroidFeatureSupported";
static constexpr const char* kGetAndroidFeatureSupportedSignature = "(I)Z";
static constexpr const char* kGetDisplayDPIName = "GetDisplayDPI";
static constexpr const char* kGetDisplayDPISignature = "()I";

android_app* PlatformUtilAndroid::android_app_ = nullptr;
JNIEnv* PlatformUtilAndroid::jni_env_ = nullptr;
jclass PlatformUtilAndroid::activity_class_ = nullptr;
jobject PlatformUtilAndroid::activity_class_object_ = nullptr;
jclass PlatformUtilAndroid::util_class_ = nullptr;
jobject PlatformUtilAndroid::util_class_object_ = nullptr;
jmethodID PlatformUtilAndroid::get_android_feature_method_ = nullptr;
jmethodID PlatformUtilAndroid::get_display_dpi_method_ = nullptr;
ANativeWindow *PlatformUtilAndroid::native_window_ = nullptr;
bool PlatformUtilAndroid::has_focus_ = false;
std::thread::id PlatformUtilAndroid::main_thread_id_;

bool PlatformUtilAndroid::GetAndroidFeatureSupported(const AndroidFeature android_feature) {
  bool supported = false;
  if (util_class_ != nullptr && util_class_object_ != nullptr && jni_env_ != nullptr &&
      get_android_feature_method_ != nullptr) {
    jboolean result = jni_env_->CallBooleanMethod(util_class_object_,
                                                  get_android_feature_method_,
                                                  static_cast<jint>(android_feature));
    supported = (result == JNI_TRUE);
  }
  return supported;
}

int PlatformUtilAndroid::GetDisplayDPI() {
  int dpi = 0;
  if (util_class_ != nullptr && util_class_object_ != nullptr && jni_env_ != nullptr &&
      get_display_dpi_method_ != nullptr) {
    dpi = jni_env_->CallIntMethod(util_class_object_, get_display_dpi_method_);
  }
  return dpi;
}

void PlatformUtilAndroid::InitMethods(JNIEnv* env) {
  get_android_feature_method_ = env->GetMethodID(util_class_,
                                                 kGetAndroidFeatureSupportedName,
                                                 kGetAndroidFeatureSupportedSignature);
  get_display_dpi_method_ = env->GetMethodID(util_class_,
                                             kGetDisplayDPIName,
                                             kGetDisplayDPISignature);
}
} // namespace base_game_framework

extern "C"
JNIEXPORT void JNICALL
Java_com_google_android_games_basegameframework_BaseGameFrameworkUtils_registerUtilObject(
    JNIEnv* env, jobject thiz) {
  jclass activity_class = env->GetObjectClass(thiz);
  if (activity_class != nullptr) {
    base_game_framework::PlatformUtilAndroid::SetUtilClass(
        static_cast<jclass>(env->NewGlobalRef(activity_class)));
    base_game_framework::PlatformUtilAndroid::SetUtilClassObject(
        env->NewGlobalRef(thiz));
    env->DeleteLocalRef(activity_class);

    base_game_framework::PlatformUtilAndroid::InitMethods(env);
  }
}