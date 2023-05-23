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

#include "android/content/context.h"

#include "android/content/context.hpp"

Context* Context_wrapJniReference(jobject jobj) {
  return reinterpret_cast<Context*>(new ::android::content::Context(jobj));
}

jobject Context_getJniReference(const Context* context) {
  return reinterpret_cast<const ::android::content::Context*>(context)->GetImpl();
}

void Context_destroy(const Context* context) {
  ::android::content::Context::destroy(reinterpret_cast<const ::android::content::Context*>(context));
}

Object* Context_getSystemService(const Context* context, String* name) {
  return reinterpret_cast<Object*>(&reinterpret_cast<const ::android::content::Context*>(context)->getSystemService(*reinterpret_cast<const ::java::lang::String*>(name)));
}
