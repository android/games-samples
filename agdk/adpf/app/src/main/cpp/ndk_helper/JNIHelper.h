/*
 * Copyright 2013 The Android Open Source Project
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

#ifndef _JNI_HELPER
#define _JNI_HELPER

#include <vector>
#include <string>
#include <functional>
#include <assert.h>
#include <mutex>

#ifdef __ANDROID__
#include <jni.h>
#include <pthread.h>
#include <android/log.h>

#include "common.h"

#define LOGI(...)                                                           \
  ((void)__android_log_print(                                               \
      ANDROID_LOG_INFO, ndk_helper::JNIHelper::GetInstance()->GetAppName(), \
      __VA_ARGS__))
#define LOGW(...)                                                           \
  ((void)__android_log_print(                                               \
      ANDROID_LOG_WARN, ndk_helper::JNIHelper::GetInstance()->GetAppName(), \
      __VA_ARGS__))
#define LOGE(...)                                                            \
  ((void)__android_log_print(                                                \
      ANDROID_LOG_ERROR, ndk_helper::JNIHelper::GetInstance()->GetAppName(), \
      __VA_ARGS__))
#else
#define LOGI(...) printf(__VA_ARGS__)
#define LOGW(...) printf(__VA_ARGS__)
#define LOGE(...) printf(__VA_ARGS__)
#endif
namespace ndk_helper {
    /******************************************************************
     * Helper functions for JNI calls
     * This class wraps JNI calls and provides handy interface calling commonly used
     *features
     * in Java SDK.
     * Such as
     * - loading graphics files (e.g. PNG, JPG)
     * - character code conversion
     * - retrieving system properties which only supported in Java SDK
     *
     * NOTE: To use this class, add NDKHelper.java as a corresponding helpers in
     *Java code
     */
    class JNIHelper {
     private:
      std::string app_name_;

      android_app* app_;
      jobject jni_helper_java_ref_;
      jclass jni_helper_java_class_;

      jstring GetExternalFilesDirJString(JNIEnv* env);

      JNIHelper();
      ~JNIHelper();
      JNIHelper(const JNIHelper& rhs);
      JNIHelper& operator=(const JNIHelper& rhs);

      std::string app_label_;

      // mutex for synchronization
      // This class uses singleton pattern and can be invoked from multiple threads,
      // each methods locks the mutex for a thread safety
      mutable std::mutex mutex_;

      /*
       * Call method in JNIHelper class
       */
      jobject CallObjectMethod(const char* strMethodName, const char* strSignature,
                               ...);
      void CallVoidMethod(const char* strMethodName, const char* strSignature, ...);

      /*
       * Unregister this thread from the VM
       */
      static void DetachCurrentThreadDtor(void* p) {
        LOGI("detached current thread");
        android_app* app = static_cast<android_app*>(p);
        JavaVM *vm = app->activity->vm;
        vm->DetachCurrentThread();
      }

     public:
      /*
       * To load your own Java classes, JNIHelper requires to be initialized with a
       *ANativeActivity handle.
       * This methods need to be called before any call to the helper class.
       * Static member of the class
       *
       * arguments:
       * in: app, pointer to android_app. Used internally to set up JNI
       *environment
       */
      static void Init(android_app* app);

      /*
       * Init() that accept so name.
       * When using a JUI helper class, Java side requires SO name to initialize JNI
       * calls to invoke native callbacks.
       * Use this version when using JUI helper.
       *
       * arguments:
       * in: app, pointer to android_app. Used internally to set up JNI
       * environment
       * in: native_soname, pointer to soname of native library. (e.g.
       * "NativeActivity" for "libNativeActivity.so" )
       */
      static void Init(android_app* app,
                       const char* native_soname);

      /*
       * Retrieve the singleton object of the helper.
       * Static member of the class

       * Methods in the class are designed as thread safe.
       */
      static JNIHelper* GetInstance();

      /*
       * Read a file from a strorage.
       * First, the method tries to read the file from an external storage.
       * If it fails to read, it falls back to use assset manager and try to read
       * the file from APK asset.
       *
       * arguments:
       * in: file_name, file name to read
       * out: buffer_ref, pointer to a vector buffer to read a file.
       *      when the call succeeded, the buffer includes contents of specified
       *file
       *      when the call failed, contents of the buffer remains same
       * return:
       * true when file read succeeded
       * false when it failed to read the file
       */
      bool ReadFile(const char* file_name, std::vector<uint8_t>* buffer_ref);

      /*
       * Convert string from character code other than UTF-8
       *
       * arguments:
       *  in: str, pointer to a string which is encoded other than UTF-8
       *  in: encoding, pointer to a character encoding string.
       *  The encoding string can be any valid java.nio.charset.Charset name
       *  e.g. "UTF-16", "Shift_JIS"
       * return: converted input string as an UTF-8 std::string
       */
      std::string ConvertString(const char* str, const char* encode);
      /*
       * Retrieve external file directory through JNI call
       *
       * return: std::string containing external file diretory
       */
      std::string GetExternalFilesDir();

      /*
       * Retrieve string resource with a given name
       * arguments:
       *  in: resourceName, name of string resource to retrieve
       * return: string resource value, returns "" when there is no string resource
       * with given name
       */
      std::string GetStringResource(const std::string& resourceName);

      /*
       * Audio helper
       * Retrieves native audio buffer size which is required to achieve low latency
       * audio
       *
       * return: Native audio buffer size which is a hint to achieve low latency
       * audio
       * If the API is not supported (API level < 17), it returns 0
       */
      int32_t GetNativeAudioBufferSize();

      /*
       * Audio helper
       * Retrieves native audio sample rate which is required to achieve low latency
       * audio
       *
       * return: Native audio sample rate which is a hint to achieve low latency
       * audio
       */
      int32_t GetNativeAudioSampleRate();

      /*
       * Retrieves application bundle name
       *
       * return: pointer to an app name string
       *
       */
      const char* GetAppName() const { return app_name_.c_str(); }

      /*
       * Retrieves application label
       *
       * return: pointer to an app label string
       *
       */
      const char* GetAppLabel() const { return app_label_.c_str(); }

      /*
       * Attach current thread
       * In Android, the thread doesn't have to be 'Detach' current thread
       * as application process is only killed and VM does not shut down
       */
      JNIEnv* AttachCurrentThread() {
        JNIEnv* env;
        JavaVM *vm = app_->activity->vm;
        if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK)
          return env;
        vm->AttachCurrentThread(&env, NULL);
        pthread_key_create((int32_t*)app_, DetachCurrentThreadDtor);
        return env;
      }

      void DetachCurrentThread() {
        app_->activity->vm->DetachCurrentThread();
        return;
      }

      /*
       * Decrement a global reference to the object
       * arguments:
       *  in: obj, obj to decrement a global reference
       */
      void DeleteObject(jobject obj);

      /*
       * Helper methods to call a method in given object
       */
      jobject CreateObject(const char* class_name);
      jobject CallObjectMethod(jobject object, const char* strMethodName,
                               const char* strSignature, ...);
      void CallVoidMethod(jobject object, const char* strMethodName,
                          const char* strSignature, ...);
      float CallFloatMethod(jobject object, const char* strMethodName,
                            const char* strSignature, ...);
      int32_t CallIntMethod(jobject object, const char* strMethodName,
                            const char* strSignature, ...);
      bool CallBooleanMethod(jobject object, const char* strMethodName,
                             const char* strSignature, ...);
    };
}  // namespace ndkHelper

#endif //_JNI_HELPER
