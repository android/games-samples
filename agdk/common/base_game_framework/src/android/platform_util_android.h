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

#ifndef BASEGAMEFRAMEWORK_PLATFORM_UTIL_ANDROID_H_
#define BASEGAMEFRAMEWORK_PLATFORM_UTIL_ANDROID_H_

#include <android/native_window.h>
#include <jni.h>
#include <thread>
#include "game-activity/native_app_glue/android_native_app_glue.h"

namespace base_game_framework {

class PlatformUtilAndroid {
  public:

  enum AndroidFeature : int32_t {
    // OpenGL ES Android Extension Pack
    kAndroidFeatureAndroidExtensionPack = 1
  };

  static bool GetAndroidFeatureSupported(const AndroidFeature android_feature);
  static int GetDisplayDPI();

  static android_app* GetAndroidApp() { return android_app_;}
  static void SetAndroidApp(android_app* app) { android_app_ = app; }

  static JNIEnv* GetMainThreadJNIEnv() { return jni_env_; }
  static void SetMainThreadJniEnv(JNIEnv* jni_env) { jni_env_ = jni_env; }

  static jobject GetActivityClassObject() { return activity_class_object_; }
  static void SetActivityClassObject(jobject activity_class_object) {
    activity_class_object_ = activity_class_object;
  }

  static jclass GetUtilClass() { return util_class_; }
  static void SetUtilClass(jclass clazz) { util_class_ = clazz; }
  static jobject GetUtilClassObject() { return util_class_object_; }
  static void SetUtilClassObject(jobject util_class_object) {
    util_class_object_ = util_class_object;
  }

  static ANativeWindow* GetNativeWindow() { return native_window_; }
  static void SetNativeWindow(ANativeWindow* native_window) { native_window_ = native_window; }

  static bool GetHasFocus() { return has_focus_; }
  static void SetHasFocus(bool focus) { has_focus_ = focus; }

  static void SetMainThreadID(const std::thread::id thread_id) { main_thread_id_ = thread_id; }
  static std::thread::id GetMainThreadID() { return main_thread_id_; }
  static bool RunningOnMainThread() { return std::this_thread::get_id() == main_thread_id_; }

  static void InitMethods(JNIEnv *env);

 private:
  static android_app* android_app_;
  static JNIEnv* jni_env_;
  static jclass activity_class_;
  static jobject activity_class_object_;
  static jclass util_class_;
  static jobject util_class_object_;
  static jmethodID get_android_feature_method_;
  static jmethodID get_display_dpi_method_;
  static ANativeWindow *native_window_;
  static std::thread::id main_thread_id_;
  static bool has_focus_;
};

} // namespace base_game_framework

#endif //BASEGAMEFRAMEWORK_PLATFORM_UTIL_ANDROID_H_