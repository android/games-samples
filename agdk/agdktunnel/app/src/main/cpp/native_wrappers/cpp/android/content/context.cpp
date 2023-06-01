/*
 * Copyright 2023 The Android Open Source Project
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

// This file was generated using the Library Wrapper tool
// https://developer.android.com/games/develop/custom/wrapper

#include "android/content/context.hpp"
#include <memory>
#include "gni/common/scoped_local_ref.h"
#include "gni/gni.hpp"
#include "gni/object.hpp"
#include "java/lang/object.hpp"
#include "java/lang/string.hpp"

namespace android {
namespace content {

jclass Context::GetClass()
{
  static const jclass cached_class = gni::GniCore::GetInstance()->GetClassGlobalRef("android/content/Context");
  return cached_class;
}

void Context::destroy(const Context* object)
{
  delete object;
}

::java::lang::Object& Context::getSystemService(const ::java::lang::String& name) const
{
  JNIEnv *env = gni::GniCore::GetInstance()->GetJniEnv();
  static const jmethodID method_id = env->GetMethodID(GetClass(), "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
  ::java::lang::Object* ret = new ::java::lang::Object(gni::common::ScopedLocalRef<jobject>(env, env->CallObjectMethod(GetImpl(), method_id, name.GetImpl())).Get());
  return *ret;
}

}  // namespace content
}  // namespace android

