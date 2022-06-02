/*
 * Copyright 2021 The Android Open Source Project
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

#include "adpf_manager.h"

void nativeThermalStatusChanged(JNIEnv* env,  jclass cls, jint thermalState) {
    ALOGI("Thermal Status updated to:%d", thermalState);
    ADPFManager::getInstance().SetThermalStatus(thermalState);
}

void ADPFManager::Monitor() {
    float current_clock = Clock();

    if (current_clock - last_clock_ >= THERMAL_HEADROOM_UPDATE_THRESHOLD) {
        // Update thermal headroom.
        UpdateThermalStatusHeadRoom();
        last_clock_ = current_clock;
    }
}

void ADPFManager::SetApplication(android_app* app) {
    app_.reset(app);

    // Initialize PowerManager reference.
    InitializePowerManager();
}

bool ADPFManager::InitializePowerManager() {
    JNIEnv *env = app_->activity->env;
    JavaVM *vm = app_->activity->vm;

    // First, attach this thread to the main thread
    JavaVMAttachArgs args;
    args.version = JNI_VERSION_1_6;
    args.name = "NativeThread";
    args.group = nullptr;
    jint res = vm->AttachCurrentThread(&env, &args);
    if (res == JNI_ERR) {
        ALOGE("Failed to attach the thread");
        return false;
    }

    // Retrieve class information
    jclass context = env->FindClass("android/content/Context");

    // Get the value of a constant
    jfieldID fid = env->GetStaticFieldID(context, "POWER_SERVICE", "Ljava/lang/String;");
    jobject str_svc = env->GetStaticObjectField(context, fid);

    // Get the method 'getSystemService' and call it
    jmethodID mid_getss = env->GetMethodID(context, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject obj_powerService = env->CallObjectMethod(app_->activity->javaGameActivity, mid_getss, str_svc);

    // Add global reference to the power service object.
    obj_powerService_ = env->NewGlobalRef(obj_powerService);

    jclass cls_power_service = env->GetObjectClass(obj_powerService_);
    get_thermal_headroom_ = env->GetMethodID(cls_power_service, "getThermalHeadroom", "(I)F");

    // Free references
    env->DeleteLocalRef(cls_power_service);
    env->DeleteLocalRef(obj_powerService);
    env->DeleteLocalRef(str_svc);
    env->DeleteLocalRef(context);

    // Detach thread again
    vm->DetachCurrentThread();
    return true;
}

float ADPFManager::UpdateThermalStatusHeadRoom() {
    if (app_ == NULL) {
        return 0.f;
    }
    JNIEnv *env = app_->activity->env;
    JavaVM *vm = app_->activity->vm;

    // First, attach this thread to the main thread
    JavaVMAttachArgs args;
    args.version = JNI_VERSION_1_6;
    args.name = "NativeThread";
    args.group = nullptr;
    jint res = vm->AttachCurrentThread(&env, &args);
    if (res == JNI_ERR) {
        ALOGE("Failed to attach the thread");
        return 0.f;
    }

    // Get thermal headroom!
    float f = env->CallFloatMethod(obj_powerService_, get_thermal_headroom_, THERMAL_HEADROOM_UPDATE_THRESHOLD);
    ALOGE("Current thermal Headroom %f", f);

    // Detach thread again
    vm->DetachCurrentThread();
    return f;
}

