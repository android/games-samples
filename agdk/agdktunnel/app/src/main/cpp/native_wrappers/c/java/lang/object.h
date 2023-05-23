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

#ifndef JAVA_LANG_OBJECT_H_
#define JAVA_LANG_OBJECT_H_

#include <cstdint>
#include <jni.h>
#include "java/lang/string.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Object_ Object;

/// Wraps a JNI reference with Object object.
/// @param jobj A JNI reference to be wrapped with Object object.
/// @see Object_destroy
Object* Object_wrapJniReference(jobject jobj);

jobject Object_getJniReference(const Object* object);

/// Destroys object and all internal resources related to it. This function should be
/// called when object is no longer needed.
/// @param object An object to be destroyed.
void Object_destroy(const Object* object);

String* Object_toString(const Object* object);

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // JAVA_LANG_OBJECT_H_
