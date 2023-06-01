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

#ifndef ANDROID_CONTENT_CONTEXT_H_
#define ANDROID_CONTENT_CONTEXT_H_

#include <cstdint>
#include <jni.h>
#include "java/lang/object.h"
#include "java/lang/string.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Context_ Context;

/// Wraps a JNI reference with Context object.
/// @param jobj A JNI reference to be wrapped with Context object.
/// @see Context_destroy
Context* Context_wrapJniReference(jobject jobj);

jobject Context_getJniReference(const Context* context);

/// Destroys context and all internal resources related to it. This function should be
/// called when context is no longer needed.
/// @param context An object to be destroyed.
void Context_destroy(const Context* context);

Object* Context_getSystemService(const Context* context, String* name);

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // ANDROID_CONTENT_CONTEXT_H_
