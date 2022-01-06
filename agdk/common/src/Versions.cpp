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

#include "Versions.h"

namespace agdk_samples_util {

// App version string from the app's build.gradle file.
int GetAppVersionInfo(JNIEnv* env, jobject context, int* versionCode,
                      std::string* versionName) {
    jstring packageName;
    jobject packageManagerObj;
    jobject packageInfoObj;
    jclass contextClass = env->GetObjectClass(context);
    jmethodID getPackageNameMid = env->GetMethodID(
        contextClass, "getPackageName", "()Ljava/lang/String;");
    jmethodID getPackageManager =
        env->GetMethodID(contextClass, "getPackageManager",
                         "()Landroid/content/pm/PackageManager;");

    jclass packageManagerClass =
        env->FindClass("android/content/pm/PackageManager");
    jmethodID getPackageInfo = env->GetMethodID(
        packageManagerClass, "getPackageInfo",
        "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");

    jclass packageInfoClass = env->FindClass("android/content/pm/PackageInfo");

    packageName = (jstring)env->CallObjectMethod(context, getPackageNameMid);
    packageManagerObj = env->CallObjectMethod(context, getPackageManager);
    packageInfoObj = env->CallObjectMethod(packageManagerObj, getPackageInfo,
                                           packageName, 0x0);

    if (versionCode != nullptr) {
        jfieldID versionCodeFid =
            env->GetFieldID(packageInfoClass, "versionCode", "I");
        *versionCode = env->GetIntField(packageInfoObj, versionCodeFid);
    }

    if (versionName != nullptr) {
        jfieldID versionNameFid = env->GetFieldID(
            packageInfoClass, "versionName", "Ljava/lang/String;");
        jstring jVersionName =
            (jstring)env->GetObjectField(packageInfoObj, versionNameFid);
        auto len = env->GetStringUTFLength(jVersionName);
        const char* cVersionName = env->GetStringUTFChars(jVersionName, NULL);
        *versionName = std::string(cVersionName, len);
        env->ReleaseStringUTFChars(jVersionName, cVersionName);
        env->DeleteLocalRef(jVersionName);
    }

    env->DeleteLocalRef(packageManagerObj);
    env->DeleteLocalRef(packageInfoObj);
    env->DeleteLocalRef(packageName);

    return 0;
}

}  // namespace agdk_samples_util
