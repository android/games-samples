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

#include <mutex>
#include <vector>

namespace base_game_framework {
static constexpr const char* kGetAndroidFeatureSupportedName = "GetAndroidFeatureSupported";
static constexpr const char* kGetAndroidFeatureSupportedSignature = "(I)Z";
static constexpr const char* kGetDisplayDPIName = "GetDisplayDPI";
static constexpr const char* kGetDisplayDPISignature = "()I";

namespace {
// The registered Java BaseGameFrameworkUtils object (global ref) and the Activity that
// owns it (weak global ref). Written on the Java UI thread by RegisterUtilObject and
// read/released on the native app thread, so all access is guarded by util_object_mutex.
std::mutex util_object_mutex;
jobject util_object = nullptr;
jweak util_owner_activity = nullptr;

void DeleteUtilObjectRefsLocked(JNIEnv *env) {
  if (util_object != nullptr) {
    env->DeleteGlobalRef(util_object);
    util_object = nullptr;
  }
  if (util_owner_activity != nullptr) {
    env->DeleteWeakGlobalRef(util_owner_activity);
    util_owner_activity = nullptr;
  }
}
} // anonymous namespace

android_app* PlatformUtilAndroid::android_app_ = nullptr;
JNIEnv* PlatformUtilAndroid::jni_env_ = nullptr;
jclass PlatformUtilAndroid::activity_class_ = nullptr;
jobject PlatformUtilAndroid::activity_class_object_ = nullptr;
ANativeWindow *PlatformUtilAndroid::native_window_ = nullptr;
bool PlatformUtilAndroid::has_focus_ = false;
std::thread::id PlatformUtilAndroid::main_thread_id_;

bool PlatformUtilAndroid::GetAndroidFeatureSupported(const AndroidFeature android_feature) {
  bool supported = false;
  JNIEnv *env = jni_env_;
  jobject util = NewUtilObjectLocalRef(env);
  if (util != nullptr) {
    jclass util_class = env->GetObjectClass(util);
    jmethodID method = env->GetMethodID(util_class, kGetAndroidFeatureSupportedName,
                                        kGetAndroidFeatureSupportedSignature);
    if (method != nullptr) {
      const jboolean result = env->CallBooleanMethod(util, method,
                                                     static_cast<jint>(android_feature));
      supported = !ClearPendingException(env) && (result == JNI_TRUE);
    } else {
      ClearPendingException(env);
    }
    env->DeleteLocalRef(util_class);
    env->DeleteLocalRef(util);
  }
  return supported;
}

int PlatformUtilAndroid::GetDisplayDPI() {
  int dpi = 0;
  JNIEnv *env = jni_env_;
  jobject util = NewUtilObjectLocalRef(env);
  if (util != nullptr) {
    jclass util_class = env->GetObjectClass(util);
    jmethodID method = env->GetMethodID(util_class, kGetDisplayDPIName, kGetDisplayDPISignature);
    if (method != nullptr) {
      const jint result = env->CallIntMethod(util, method);
      dpi = ClearPendingException(env) ? 0 : result;
    } else {
      ClearPendingException(env);
    }
    env->DeleteLocalRef(util_class);
    env->DeleteLocalRef(util);
  }
  return dpi;
}

jobject PlatformUtilAndroid::NewUtilObjectLocalRef(JNIEnv *env) {
  if (env == nullptr) {
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(util_object_mutex);
  return (util_object != nullptr) ? env->NewLocalRef(util_object) : nullptr;
}

void PlatformUtilAndroid::RegisterUtilObject(JNIEnv *env, jobject util, jobject activity) {
  std::lock_guard<std::mutex> lock(util_object_mutex);
  // Any thread still using a previous registration holds its own local ref
  // (see NewUtilObjectLocalRef), so the old global refs can be deleted right away.
  DeleteUtilObjectRefsLocked(env);
  util_object = env->NewGlobalRef(util);
  util_owner_activity = (activity != nullptr) ? env->NewWeakGlobalRef(activity) : nullptr;
}

void PlatformUtilAndroid::ReleaseUtilObject(JNIEnv *env, jobject activity) {
  if (env == nullptr || activity == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(util_object_mutex);
  if (util_owner_activity != nullptr && env->IsSameObject(util_owner_activity, activity)) {
    DeleteUtilObjectRefsLocked(env);
  }
}

std::string PlatformUtilAndroid::JStringToString(JNIEnv *env, jstring jstr) {
  std::string result;
  if (env != nullptr && jstr != nullptr) {
    const char *chars = env->GetStringUTFChars(jstr, nullptr);
    if (chars != nullptr) {
      result = chars;
      env->ReleaseStringUTFChars(jstr, chars);
    }
  }
  return result;
}

bool PlatformUtilAndroid::ClearPendingException(JNIEnv *env) {
  if (env != nullptr && env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    return true;
  }
  return false;
}
} // namespace base_game_framework

extern "C"
JNIEXPORT void JNICALL
Java_com_google_android_games_basegameframework_BaseGameFrameworkUtils_registerUtilObject(
    JNIEnv* env, jobject thiz, jobject activity) {
  base_game_framework::PlatformUtilAndroid::RegisterUtilObject(env, thiz, activity);
}
