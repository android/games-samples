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

#include "java/lang/string.h"

#include "java/lang/string.hpp"

String* String_fromCString(const char* c_string) {
  return String_wrapJniReference(gni::GniCore::GetInstance()->ConvertString(c_string).GetImpl());
}

const char* String_toCString(String* string) {
  return gni::GniCore::GetInstance()->ConvertString(String_getJniReference(string));
}

void String_destroyCString(const char* c_string) {
  free(const_cast<char*>(c_string));
}

String* String_wrapJniReference(jobject jobj) {
  return reinterpret_cast<String*>(new ::java::lang::String(jobj));
}

jobject String_getJniReference(const String* string) {
  return reinterpret_cast<const ::java::lang::String*>(string)->GetImpl();
}

void String_destroy(const String* string) {
  ::java::lang::String::destroy(reinterpret_cast<const ::java::lang::String*>(string));
}
