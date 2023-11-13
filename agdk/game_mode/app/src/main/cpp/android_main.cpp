/*
 * Copyright 2022 The Android Open Source Project
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

#include "ndk_helper/NDKHelper.h"
#include "adpf_manager.h"
#include "game_mode_manager.h"
#include "native_engine.h"

extern "C" {
void android_main(struct android_app *app);
}

// Register classes to Java.
jint JNI_OnLoad(JavaVM *vm, void * /* reserved */) {
  JNIEnv *env;
  if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
    return JNI_ERR;
  }

  // Find your class. JNI_OnLoad is called from the correct class loader context
  // for this to work.
  jclass c = env->FindClass("com/android/example/games/ADPFManager");
  if (c == nullptr) return JNI_ERR;

  // Register your class' native methods.
  static const JNINativeMethod methods[] = {
      {"nativeThermalStatusChanged", "(I)V",
       reinterpret_cast<void *>(nativeThermalStatusChanged)},
      {"nativeRegisterThermalStatusListener", "()V",
       reinterpret_cast<void *>(nativeRegisterThermalStatusListener)},
      {"nativeUnregisterThermalStatusListener", "()V",
       reinterpret_cast<void *>(nativeUnregisterThermalStatusListener)},
  };
  int rc = env->RegisterNatives(c, methods,
                                sizeof(methods) / sizeof(JNINativeMethod));

  if (rc != JNI_OK) return rc;

  return JNI_VERSION_1_6;
}

/*
    android_main (not main) is our game entry function, it is called from
    the native app glue utility code as part of the onCreate handler.
*/

void android_main(struct android_app *app) {
  std::shared_ptr<NativeEngine> engine(new NativeEngine(app));

  // Set android_app to ADPF manager.
  ADPFManager::getInstance().SetApplication(app);
  GameModeManager::getInstance().SetApplication(app);
  ndk_helper::JNIHelper::Init(app);

  engine->GameLoop();
}
