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

#include "filesystem_manager.h"
#include "debug_manager.h"
#include "platform_util_android.h"
#include <android/asset_manager.h>

namespace base_game_framework {

std::unique_ptr<FilesystemManager> FilesystemManager::instance_ = nullptr;

FilesystemManager &FilesystemManager::GetInstance() {
  if (!instance_) {
    instance_ = std::unique_ptr<FilesystemManager>(new FilesystemManager());
  }
  return *instance_;
}
void FilesystemManager::ShutdownInstance() {
  FilesystemManager::instance_.reset();
}

FilesystemManager::FilesystemManager() {
  JNIEnv *env = PlatformUtilAndroid::GetMainThreadJNIEnv();
  jobject util_object = PlatformUtilAndroid::NewUtilObjectLocalRef(env);
  if (util_object != nullptr) {
    jclass util_class = env->GetObjectClass(util_object);
    jmethodID method_getrootpath = env->GetMethodID(util_class, "GetRootPath",
                                                    "(I)Ljava/lang/String;");
    auto get_root_path = [env, util_object, method_getrootpath](RootPathType path_type) {
      jstring path_jstring = static_cast<jstring>(
          env->CallObjectMethod(util_object, method_getrootpath,
                                static_cast<jint>(path_type)));
      std::string path;
      if (!PlatformUtilAndroid::ClearPendingException(env)) {
        path = PlatformUtilAndroid::JStringToString(env, path_jstring);
      }
      env->DeleteLocalRef(path_jstring);
      return path;
    };
    if (method_getrootpath != nullptr) {
      root_path_internal_ = get_root_path(kRootPathInternalStorage);
      root_path_external_ = get_root_path(kRootPathExternalStorage);
      root_path_cache_ = get_root_path(kRootPathCache);
    } else {
      PlatformUtilAndroid::ClearPendingException(env);
    }
    env->DeleteLocalRef(util_class);
    env->DeleteLocalRef(util_object);
  } else if (env != nullptr) {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      "FilesystemManager",
                      "No BaseGameFrameworkUtils object, probably missing from Activity!");
  }
}

bool FilesystemManager::GetExternalStorageEmulated() const {
  bool emulated = true;
  JNIEnv *env = PlatformUtilAndroid::GetMainThreadJNIEnv();
  jobject util_object = PlatformUtilAndroid::NewUtilObjectLocalRef(env);
  if (util_object != nullptr) {
    jclass util_class = env->GetObjectClass(util_object);
    jmethodID method_getemulated = env->GetMethodID(util_class, "GetExternalStorageEmulated",
                                                    "()I");
    if (method_getemulated != nullptr) {
      const jint jemulated = env->CallIntMethod(util_object, method_getemulated);
      if (!PlatformUtilAndroid::ClearPendingException(env)) {
        emulated = (jemulated == 1);
      }
    } else {
      PlatformUtilAndroid::ClearPendingException(env);
    }
    env->DeleteLocalRef(util_class);
    env->DeleteLocalRef(util_object);
  }
  return emulated;
}

const std::string &FilesystemManager::GetRootPath(const RootPathType path_type) const {
  switch (path_type) {
    case kRootPathInternalStorage:return root_path_internal_;
    case kRootPathExternalStorage:return root_path_external_;
    case kRootPathCache:return root_path_cache_;
  }
  return root_path_internal_;
}

int64_t FilesystemManager::GetFreeSpace(const RootPathType path_type) const {
  int64_t free_space = -1;

  JNIEnv *env = PlatformUtilAndroid::GetMainThreadJNIEnv();
  jobject util_object = PlatformUtilAndroid::NewUtilObjectLocalRef(env);
  if (util_object != nullptr) {
    jclass util_class = env->GetObjectClass(util_object);
    jmethodID method_getfreespace = env->GetMethodID(util_class, "GetEstimatedFreeSpace",
                                                     "(I)J");
    if (method_getfreespace != nullptr) {
      const jlong jfree_space = env->CallLongMethod(util_object, method_getfreespace,
                                                    static_cast<jint>(path_type));
      if (!PlatformUtilAndroid::ClearPendingException(env)) {
        free_space = static_cast<int64_t>(jfree_space);
      }
    } else {
      PlatformUtilAndroid::ClearPendingException(env);
    }
    env->DeleteLocalRef(util_class);
    env->DeleteLocalRef(util_object);
  }
  return free_space;
}

uint64_t FilesystemManager::GetPackageFileSize(const std::string &file_path) {
  android_app *app = PlatformUtilAndroid::GetAndroidApp();
  size_t asset_size = 0;
  AAsset *asset = AAssetManager_open(app->activity->assetManager, file_path.c_str(),
                                     AASSET_MODE_STREAMING);
  if (asset != NULL) {
    asset_size = AAsset_getLength(asset);
    AAsset_close(asset);
  }
  return static_cast<uint64_t>(asset_size);
}

uint64_t FilesystemManager::LoadPackageFile(const std::string file_path, const uint64_t buffer_size,
                                            void *load_buffer) {

  android_app *app = PlatformUtilAndroid::GetAndroidApp();
  size_t asset_size = 0;
  uint64_t bytes_read = 0;
  AAsset *asset = AAssetManager_open(app->activity->assetManager, file_path.c_str(),
                                     AASSET_MODE_STREAMING);
  if (asset != NULL) {
    asset_size = AAsset_getLength(asset);
    if (asset_size <= buffer_size) {
      bytes_read = AAsset_read(asset, load_buffer, static_cast<size_t>(buffer_size));
    }
    AAsset_close(asset);
  }
  return bytes_read;
}

} // namespace base_game_framework