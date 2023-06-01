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

#include "java/lang/object.h"

#include "java/lang/object.hpp"

Object* Object_wrapJniReference(jobject jobj) {
  return reinterpret_cast<Object*>(new ::java::lang::Object(jobj));
}

jobject Object_getJniReference(const Object* object) {
  return reinterpret_cast<const ::java::lang::Object*>(object)->GetImpl();
}

void Object_destroy(const Object* object) {
  ::java::lang::Object::destroy(reinterpret_cast<const ::java::lang::Object*>(object));
}

String* Object_toString(const Object* object) {
  return reinterpret_cast<String*>(&reinterpret_cast<const ::java::lang::Object*>(object)->toString());
}
