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
#include "JNIHelper.h"

#ifdef __ANDROID__

#include <string.h>

#include <fstream>
#include <iostream>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#else

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#endif
namespace ndk_helper {
    //---------------------------------------------------------------------------
    // JNI Helper functions
    //---------------------------------------------------------------------------

    //---------------------------------------------------------------------------
    // Singleton
    //---------------------------------------------------------------------------
    JNIHelper* JNIHelper::GetInstance() {
      static JNIHelper helper;
      return &helper;
    }

    //---------------------------------------------------------------------------
    // Ctor
    //---------------------------------------------------------------------------
    JNIHelper::JNIHelper() : app_(nullptr) {}

    //---------------------------------------------------------------------------
    // Dtor
    //---------------------------------------------------------------------------
    JNIHelper::~JNIHelper() {
      // Lock mutex
      std::lock_guard<std::mutex> lock(mutex_);

      JNIEnv* env = AttachCurrentThread();
      env->DeleteGlobalRef(jni_helper_java_ref_);
      env->DeleteGlobalRef(jni_helper_java_class_);

      DetachCurrentThread();
    }

    //---------------------------------------------------------------------------
    // Init
    //---------------------------------------------------------------------------
    void JNIHelper::Init(android_app* app) {
      JNIHelper& helper = *GetInstance();

      helper.app_ = app;
    }

    void JNIHelper::Init(android_app* app,
                         const char* native_soname) {
      Init(app);
      if (native_soname) {
        JNIHelper& helper = *GetInstance();
        // Lock mutex
        std::lock_guard<std::mutex> lock(helper.mutex_);

        JNIEnv* env = helper.AttachCurrentThread();

        // Setup soname
        jstring soname = env->NewStringUTF(native_soname);

        jmethodID mid = env->GetMethodID(helper.jni_helper_java_class_,
                                         "loadLibrary", "(Ljava/lang/String;)V");
        env->CallVoidMethod(helper.jni_helper_java_ref_, mid, soname);

        env->DeleteLocalRef(soname);
      }
    }

    //---------------------------------------------------------------------------
    // readFile
    //---------------------------------------------------------------------------
    bool JNIHelper::ReadFile(const char* fileName,
                             std::vector<uint8_t>* buffer_ref) {
      if (app_ == nullptr) {
        LOGI(
            "JNIHelper has not been initialized.Call init() to initialize the "
            "helper");
        return false;
      }

      // Lock mutex
      std::lock_guard<std::mutex> lock(mutex_);

      // First, try reading from externalFileDir;
      JNIEnv* env = AttachCurrentThread();
      jstring str_path = GetExternalFilesDirJString(env);

      std::string s;
      if(str_path) {
        const char* path = env->GetStringUTFChars(str_path, NULL);
        s = std::string(path);
        if (fileName[0] != '/') {
          s.append("/");
        }
        s.append(fileName);
        env->ReleaseStringUTFChars(str_path, path);
        env->DeleteLocalRef(str_path);
      }
      std::ifstream f(s.c_str(), std::ios::binary);
      if (f) {
        LOGI("reading:%s", s.c_str());
        f.seekg(0, std::ifstream::end);
        int32_t fileSize = f.tellg();
        f.seekg(0, std::ifstream::beg);
        buffer_ref->reserve(fileSize);
        buffer_ref->assign(std::istreambuf_iterator<char>(f),
                           std::istreambuf_iterator<char>());
        f.close();
        return true;
      } else {
        // Fallback to assetManager
        AAssetManager* assetManager = app_->activity->assetManager;
        AAsset* assetFile =
            AAssetManager_open(assetManager, fileName, AASSET_MODE_BUFFER);
        if (!assetFile) {
          return false;
        }
        uint8_t* data = (uint8_t*)AAsset_getBuffer(assetFile);
        int32_t size = AAsset_getLength(assetFile);
        if (data == NULL) {
          AAsset_close(assetFile);

          LOGI("Failed to load:%s", fileName);
          return false;
        }

        buffer_ref->reserve(size);
        buffer_ref->assign(data, data + size);

        AAsset_close(assetFile);
        return true;
      }
    }

    std::string JNIHelper::GetExternalFilesDir() {
      if (app_ == nullptr) {
        LOGI(
            "JNIHelper has not been initialized. Call init() to initialize the "
            "helper");
        return std::string("");
      }

      // Lock mutex
      std::lock_guard<std::mutex> lock(mutex_);

      // First, try reading from externalFileDir;
      JNIEnv* env = AttachCurrentThread();

      jstring strPath = GetExternalFilesDirJString(env);
      const char* path = env->GetStringUTFChars(strPath, NULL);
      std::string s(path);

      env->ReleaseStringUTFChars(strPath, path);
      env->DeleteLocalRef(strPath);
      return s;
    }

    std::string JNIHelper::ConvertString(const char* str, const char* encode) {
      if (app_ == nullptr) {
        LOGI(
            "JNIHelper has not been initialized. Call init() to initialize the "
            "helper");
        return std::string("");
      }

      // Lock mutex
      std::lock_guard<std::mutex> lock(mutex_);

      JNIEnv* env = AttachCurrentThread();
      env->PushLocalFrame(16);

      int32_t iLength = strlen((const char*)str);

      jbyteArray array = env->NewByteArray(iLength);
      env->SetByteArrayRegion(array, 0, iLength, (const signed char*)str);

      jstring strEncode = env->NewStringUTF(encode);

      jclass cls = env->FindClass("java/lang/String");
      jmethodID ctor = env->GetMethodID(cls, "<init>", "([BLjava/lang/String;)V");
      jstring object = (jstring)env->NewObject(cls, ctor, array, strEncode);

      const char* cparam = env->GetStringUTFChars(object, NULL);

      std::string s = std::string(cparam);

      env->ReleaseStringUTFChars(object, cparam);
      env->DeleteLocalRef(array);
      env->DeleteLocalRef(strEncode);
      env->DeleteLocalRef(object);
      env->DeleteLocalRef(cls);

      env->PopLocalFrame(NULL);

      return s;
    }
    /*
     * Retrieve string resource with a given name
     * arguments:
     *  in: resourceName, name of string resource to retrieve
     * return: string resource value, returns "" when there is no string resource
     * with given name
     */
    std::string JNIHelper::GetStringResource(const std::string& resourceName) {
      if (app_ == nullptr) {
        LOGI(
            "JNIHelper has not been initialized. Call init() to initialize the "
            "helper");
        return std::string("");
      }

      // Lock mutex
      std::lock_guard<std::mutex> lock(mutex_);

      JNIEnv* env = AttachCurrentThread();
      jstring name = env->NewStringUTF(resourceName.c_str());

      jstring ret = (jstring)CallObjectMethod(
          "getStringResource", "(Ljava/lang/String;)Ljava/lang/String;", name);

      const char* resource = env->GetStringUTFChars(ret, NULL);
      std::string s = std::string(resource);

      env->ReleaseStringUTFChars(ret, resource);
      env->DeleteLocalRef(ret);
      env->DeleteLocalRef(name);

      return s;
    }
    /*
     * Audio helpers
     */
    int32_t JNIHelper::GetNativeAudioBufferSize() {
      if (app_ == nullptr) {
        LOGI(
            "JNIHelper has not been initialized. Call init() to initialize the "
            "helper");
        return 0;
      }

      JNIEnv* env = AttachCurrentThread();
      jmethodID mid = env->GetMethodID(jni_helper_java_class_,
                                       "getNativeAudioBufferSize", "()I");
      int32_t i = env->CallIntMethod(jni_helper_java_ref_, mid);
      return i;
    }

    int32_t JNIHelper::GetNativeAudioSampleRate() {
      if (app_ == nullptr) {
        LOGI(
            "JNIHelper has not been initialized. Call init() to initialize the "
            "helper");
        return 0;
      }

      JNIEnv* env = AttachCurrentThread();
      jmethodID mid = env->GetMethodID(jni_helper_java_class_,
                                       "getNativeAudioSampleRate", "()I");
      int32_t i = env->CallIntMethod(jni_helper_java_ref_, mid);
      return i;
    }

    jstring JNIHelper::GetExternalFilesDirJString(JNIEnv* env) {
      if (app_ == nullptr) {
        LOGI(
                "JNIHelper has not been initialized. Call init() to initialize the "
                "helper");
        return NULL;
      }

      jstring obj_Path = nullptr;
      // Invoking getExternalFilesDir() java API
      // Retrieve class information
      jclass context = env->FindClass("android/content/Context");
      jmethodID mid = env->GetMethodID(context, "getExternalFilesDir",
                                       "(Ljava/lang/String;)Ljava/io/File;");
      jobject obj_File = env->CallObjectMethod(app_->activity->javaGameActivity, mid, NULL);
      if (obj_File) {
        jclass cls_File = env->FindClass("java/io/File");
        jmethodID mid_getPath =
                env->GetMethodID(cls_File, "getPath", "()Ljava/lang/String;");
        obj_Path = (jstring)env->CallObjectMethod(obj_File, mid_getPath);
      }
      return obj_Path;
    }

    void JNIHelper::DeleteObject(jobject obj) {
      if (app_ == nullptr) {
        LOGI("obj can not be NULL");
        return;
      }

      JNIEnv* env = AttachCurrentThread();
      env->DeleteGlobalRef(obj);
    }

    jobject JNIHelper::CallObjectMethod(const char* strMethodName,
                                        const char* strSignature, ...) {
        if (app_ == nullptr) {
        LOGI(
            "JNIHelper has not been initialized. Call init() to initialize the "
            "helper");
        return NULL;
      }

      JNIEnv* env = AttachCurrentThread();
      jmethodID mid =
          env->GetMethodID(jni_helper_java_class_, strMethodName, strSignature);
      if (mid == NULL) {
        LOGI("method ID %s, '%s' not found", strMethodName, strSignature);
        return NULL;
      }

      va_list args;
      va_start(args, strSignature);
      jobject obj = env->CallObjectMethodV(jni_helper_java_ref_, mid, args);
      va_end(args);

      return obj;
    }

    void JNIHelper::CallVoidMethod(const char* strMethodName,
                                   const char* strSignature, ...) {
      if (app_ == nullptr) {
        LOGI(
            "JNIHelper has not been initialized. Call init() to initialize the "
            "helper");
        return;
      }

      JNIEnv* env = AttachCurrentThread();
      jmethodID mid =
          env->GetMethodID(jni_helper_java_class_, strMethodName, strSignature);
      if (mid == NULL) {
        LOGI("method ID %s, '%s' not found", strMethodName, strSignature);
        return;
      }
      va_list args;
      va_start(args, strSignature);
      env->CallVoidMethodV(jni_helper_java_ref_, mid, args);
      va_end(args);

      return;
    }

    jobject JNIHelper::CallObjectMethod(jobject object, const char* strMethodName,
                                        const char* strSignature, ...) {
      if (app_ == nullptr) {
        LOGI(
            "JNIHelper has not been initialized. Call init() to initialize the "
            "helper");
        return NULL;
      }

      JNIEnv* env = AttachCurrentThread();
      jclass cls = env->GetObjectClass(object);
      jmethodID mid = env->GetMethodID(cls, strMethodName, strSignature);
      if (mid == NULL) {
        LOGI("method ID %s, '%s' not found", strMethodName, strSignature);
        return NULL;
      }

      va_list args;
      va_start(args, strSignature);
      jobject obj = env->CallObjectMethodV(object, mid, args);
      va_end(args);

      env->DeleteLocalRef(cls);
      return obj;
    }

    void JNIHelper::CallVoidMethod(jobject object, const char* strMethodName,
                                   const char* strSignature, ...) {
      if (app_ == nullptr) {
        LOGI(
            "JNIHelper has not been initialized. Call init() to initialize the "
            "helper");
        return;
      }

      JNIEnv* env = AttachCurrentThread();
      jclass cls = env->GetObjectClass(object);
      jmethodID mid = env->GetMethodID(cls, strMethodName, strSignature);
      if (mid == NULL) {
        LOGI("method ID %s, '%s' not found", strMethodName, strSignature);
        return;
      }

      va_list args;
      va_start(args, strSignature);
      env->CallVoidMethodV(object, mid, args);
      va_end(args);

      env->DeleteLocalRef(cls);
      return;
    }

    float JNIHelper::CallFloatMethod(jobject object, const char* strMethodName,
                                     const char* strSignature, ...) {
      float f = 0.f;
      if (app_ == nullptr) {
        LOGI(
            "JNIHelper has not been initialized. Call init() to initialize the "
            "helper");
        return f;
      }

      JNIEnv* env = AttachCurrentThread();
      jclass cls = env->GetObjectClass(object);
      jmethodID mid = env->GetMethodID(cls, strMethodName, strSignature);
      if (mid == NULL) {
        LOGI("method ID %s, '%s' not found", strMethodName, strSignature);
        return f;
      }
      va_list args;
      va_start(args, strSignature);
      f = env->CallFloatMethodV(object, mid, args);
      va_end(args);

      env->DeleteLocalRef(cls);
      return f;
    }

    int32_t JNIHelper::CallIntMethod(jobject object, const char* strMethodName,
                                     const char* strSignature, ...) {
      int32_t i = 0;
      if (app_ == nullptr) {
        LOGI(
            "JNIHelper has not been initialized. Call init() to initialize the "
            "helper");
        return i;
      }

      JNIEnv* env = AttachCurrentThread();
      jclass cls = env->GetObjectClass(object);
      jmethodID mid = env->GetMethodID(cls, strMethodName, strSignature);
      if (mid == NULL) {
        LOGI("method ID %s, '%s' not found", strMethodName, strSignature);
        return i;
      }
      va_list args;
      va_start(args, strSignature);
      i = env->CallIntMethodV(object, mid, args);
      va_end(args);

      env->DeleteLocalRef(cls);
      return i;
    }

    bool JNIHelper::CallBooleanMethod(jobject object, const char* strMethodName,
                                      const char* strSignature, ...) {
      bool b;
      if (app_ == nullptr) {
        LOGI(
            "JNIHelper has not been initialized. Call init() to initialize the "
            "helper");
        return false;
      }

      JNIEnv* env = AttachCurrentThread();
      jclass cls = env->GetObjectClass(object);
      jmethodID mid = env->GetMethodID(cls, strMethodName, strSignature);
      if (mid == NULL) {
        LOGI("method ID %s, '%s' not found", strMethodName, strSignature);
        return false;
      }
      va_list args;
      va_start(args, strSignature);
      b = env->CallBooleanMethodV(object, mid, args);
      va_end(args);

      env->DeleteLocalRef(cls);
      return b;
    }

    jobject JNIHelper::CreateObject(const char* class_name) {
      JNIEnv* env = AttachCurrentThread();

      jclass cls = env->FindClass(class_name);
      jmethodID constructor = env->GetMethodID(cls, "<init>", "()V");

      jobject obj = env->NewObject(cls, constructor);
      jobject objGlobal = env->NewGlobalRef(obj);
      env->DeleteLocalRef(obj);
      env->DeleteLocalRef(cls);
      return objGlobal;
    }

}  // namespace ndkHelper