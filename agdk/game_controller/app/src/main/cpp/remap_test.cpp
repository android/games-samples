/*
 * Copyright 2024 The Android Open Source Project
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

/*
 * This file is an example of how to format controller mapping data to pass to
 * Paddleboat_addControllerRemapDataFromFileBuffer to remap the controller axis/buttons
 * for a specific controller. One potential use of this is to 'patch' incorrect mappings
 * that aren't caught by the default Paddleboat library. For example: Android device X
 * has incorrect left/right thumbsticks on Controller Y.
 *
 * The below code will match against a specific Android OS.BRAND/OS.DEVICE with a specific API
 * range and if matching will apply a patch for the DS5 controller. The patch swaps the
 * left/right thumbstick mappings and swaps the A/Y (square/triangle) button mappings.
 *
 * To try on a specific device, change the 'matchBrand' and 'matchDevice' fields from the
 * placeholder names in the ControllerRemapTest constructor. For example 'google' and 'raven'
 * for the Pixel 6 Pro.
 *
 * The internal default controller mapping tables for Paddleboat can be found at:
 * https://android.googlesource.com/platform/frameworks/opt/gamesdk/+/refs/heads/main/games-controller/src/main/cpp/InternalControllerTable.cpp
 */

#include "remap_test.hpp"
#include "Log.h"
#include "paddleboat/paddleboat.h"
#include <android/api-level.h>
#include <android/input.h>
#include <android/keycodes.h>
#include <cstring>
#include <malloc.h>
#include <string>

// GameControllerMappingFile.h sourced from the Paddleboat library source at:
// https://android.googlesource.com/platform/frameworks/opt/gamesdk/+/refs/heads/main/games-controller/src/main/cpp/GameControllerMappingFile.h
#include "GameControllerMappingFile.h"

using namespace paddleboat;

#define LOG_TAG "RemapTest"

// Device info class and field name constants for Android
constexpr const char *kAndroidBuildClass = "android/os/Build";
constexpr const char *kBrandField = "BRAND";
constexpr const char *kDeviceField = "DEVICE";

static std::string GetStaticStringField(JNIEnv *env, jclass clz, const char *name) {
  jfieldID field_id = env->GetStaticFieldID(clz, name, "Ljava/lang/String;");
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    ALOGE("Failed to get string field %s", name);
    return "";
  }

  auto jstr = reinterpret_cast<jstring>(env->GetStaticObjectField(clz, field_id));
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    ALOGE("Failed to get string %s", name);
    return "";
  }
  auto cstr = env->GetStringUTFChars(jstr, nullptr);
  auto length = env->GetStringUTFLength(jstr);
  std::string ret_value(cstr, length);
  env->ReleaseStringUTFChars(jstr, cstr);
  env->DeleteLocalRef(jstr);
  return ret_value;
}

#define PADDLEBOAT_AXIS_BUTTON_DPAD_UP 0
#define PADDLEBOAT_AXIS_BUTTON_DPAD_LEFT 1
#define PADDLEBOAT_AXIS_BUTTON_DPAD_DOWN 2
#define PADDLEBOAT_AXIS_BUTTON_DPAD_RIGHT 3
#define PADDLEBOAT_AXIS_BUTTON_L2 9
#define PADDLEBOAT_AXIS_BUTTON_R2 12

static constexpr size_t kMaxStringSize = PADDLEBOAT_STRING_TABLE_ENTRY_MAX_SIZE; // including null terminator

enum kPatchStrings : uint32_t {
  kPatchString_Axis = 0,
  kPatchString_Button,
  kPatchString_None,
  kPatchString_Count
};

static const char *kPatchStringTable[kPatchString_Count] = {
    "ds5_axis_patch",
    "ds5_button_patch",
    "None"
};

// Flipped left/right thumbstick mapping
static constexpr Paddleboat_Controller_Mapping_File_Axis_Entry kAxisPatchData {
    kPatchString_Axis,  // axisNameStringTableIndex
    {   // axisMapping
        AMOTION_EVENT_AXIS_Z,       // PADDLEBOAT_MAPPING_AXIS_LEFTSTICK_X
        AMOTION_EVENT_AXIS_RZ,       // PADDLEBOAT_MAPPING_AXIS_LEFTSTICK_Y
        AMOTION_EVENT_AXIS_X,       // PADDLEBOAT_MAPPING_AXIS_RIGHTSTICK_X
        AMOTION_EVENT_AXIS_Y,      // PADDLEBOAT_MAPPING_AXIS_RIGHTSTICK_Y
        PADDLEBOAT_AXIS_IGNORED,    // PADDLEBOAT_MAPPING_AXIS_L1
        AMOTION_EVENT_AXIS_BRAKE,   // PADDLEBOAT_MAPPING_AXIS_L2
        PADDLEBOAT_AXIS_IGNORED,    // PADDLEBOAT_MAPPING_AXIS_R1
        AMOTION_EVENT_AXIS_GAS,     // PADDLEBOAT_MAPPING_AXIS_R2
        AMOTION_EVENT_AXIS_HAT_X,   // PADDLEBOAT_MAPPING_AXIS_HATX
        AMOTION_EVENT_AXIS_HAT_Y    // PADDLEBOAT_MAPPING_AXIS_HATY
    },
    {   // axisPositiveButtonMapping
        PADDLEBOAT_AXIS_BUTTON_IGNORED,     // PADDLEBOAT_MAPPING_AXIS_LEFTSTICK_X
        PADDLEBOAT_AXIS_BUTTON_IGNORED,     // PADDLEBOAT_MAPPING_AXIS_LEFTSTICK_Y
        PADDLEBOAT_AXIS_BUTTON_IGNORED,     // PADDLEBOAT_MAPPING_AXIS_RIGHTSTICK_X
        PADDLEBOAT_AXIS_BUTTON_IGNORED,     // PADDLEBOAT_MAPPING_AXIS_RIGHTSTICK_Y
        PADDLEBOAT_AXIS_BUTTON_IGNORED,     // PADDLEBOAT_MAPPING_AXIS_L1
        PADDLEBOAT_AXIS_BUTTON_L2,          // PADDLEBOAT_MAPPING_AXIS_L2
        PADDLEBOAT_AXIS_BUTTON_IGNORED,     // PADDLEBOAT_MAPPING_AXIS_R1
        PADDLEBOAT_AXIS_BUTTON_R2,          // PADDLEBOAT_MAPPING_AXIS_R2
        PADDLEBOAT_AXIS_BUTTON_DPAD_RIGHT,  // PADDLEBOAT_MAPPING_AXIS_HATX
        PADDLEBOAT_AXIS_BUTTON_DPAD_DOWN    // PADDLEBOAT_MAPPING_AXIS_HATY
    },
    {   // axisNegativeButtonMapping
        PADDLEBOAT_AXIS_BUTTON_IGNORED,     // PADDLEBOAT_MAPPING_AXIS_LEFTSTICK_X
        PADDLEBOAT_AXIS_BUTTON_IGNORED,     // PADDLEBOAT_MAPPING_AXIS_LEFTSTICK_Y
        PADDLEBOAT_AXIS_BUTTON_IGNORED,     // PADDLEBOAT_MAPPING_AXIS_RIGHTSTICK_X
        PADDLEBOAT_AXIS_BUTTON_IGNORED,     // PADDLEBOAT_MAPPING_AXIS_RIGHTSTICK_Y
        PADDLEBOAT_AXIS_BUTTON_IGNORED,     // PADDLEBOAT_MAPPING_AXIS_L1
        PADDLEBOAT_AXIS_BUTTON_IGNORED,     // PADDLEBOAT_MAPPING_AXIS_L2
        PADDLEBOAT_AXIS_BUTTON_IGNORED,     // PADDLEBOAT_MAPPING_AXIS_R1
        PADDLEBOAT_AXIS_BUTTON_IGNORED,     // PADDLEBOAT_MAPPING_AXIS_R2
        PADDLEBOAT_AXIS_BUTTON_DPAD_LEFT,   // PADDLEBOAT_MAPPING_AXIS_HATX
        PADDLEBOAT_AXIS_BUTTON_DPAD_UP      // PADDLEBOAT_MAPPING_AXIS_HATY
    },
    0   // axisInversionBitmask
};

// Flipped A/Y (square/triangle) button mapping)
static constexpr Paddleboat_Controller_Mapping_File_Button_Entry kButtonPatchData {
    kPatchString_Button,    // buttonNameStringTableIndex
    {   // buttonMapping
        AKEYCODE_DPAD_UP,               // PADDLEBOAT_BUTTON_DPAD_UP
        AKEYCODE_DPAD_LEFT,             // PADDLEBOAT_BUTTON_DPAD_LEFT
        AKEYCODE_DPAD_DOWN,             // PADDLEBOAT_BUTTON_DPAD_DOWN
        AKEYCODE_DPAD_RIGHT,            // PADDLEBOAT_BUTTON_DPAD_RIGHT
        AKEYCODE_BUTTON_Y,              // PADDLEBOAT_BUTTON_A
        AKEYCODE_BUTTON_B,              // PADDLEBOAT_BUTTON_B
        AKEYCODE_BUTTON_X,              // PADDLEBOAT_BUTTON_X
        AKEYCODE_BUTTON_A,              // PADDLEBOAT_BUTTON_Y
        AKEYCODE_BUTTON_L1,             // PADDLEBOAT_BUTTON_L1
        AKEYCODE_BUTTON_L2,             // PADDLEBOAT_BUTTON_L2
        AKEYCODE_BUTTON_THUMBL,         // PADDLEBOAT_BUTTON_L3
        AKEYCODE_BUTTON_R1,             // PADDLEBOAT_BUTTON_R1
        AKEYCODE_BUTTON_R2,             // PADDLEBOAT_BUTTON_R2
        AKEYCODE_BUTTON_THUMBR,         // PADDLEBOAT_BUTTON_R3
        AKEYCODE_BUTTON_SELECT,         // PADDLEBOAT_BUTTON_SELECT
        AKEYCODE_BUTTON_START,          // PADDLEBOAT_BUTTON_START
        AKEYCODE_BUTTON_MODE,           // PADDLEBOAT_BUTTON_SYSTEM
        PADDLEBOAT_BUTTON_IGNORED,      // PADDLEBOAT_BUTTON_TOUCHPAD
        PADDLEBOAT_BUTTON_IGNORED,      // PADDLEBOAT_BUTTON_AUX1
        PADDLEBOAT_BUTTON_IGNORED,      // PADDLEBOAT_BUTTON_AUX2
        PADDLEBOAT_BUTTON_IGNORED,      // PADDLEBOAT_BUTTON_AUX3
        PADDLEBOAT_BUTTON_IGNORED       // PADDLEBOAT_BUTTON_AUX4
    }
};

static constexpr Paddleboat_Controller_Mapping_File_Controller_Entry kControllerPatchData {
    // minimumEffectiveApiLevel
    31,
    // maximumEffectiveApiLevel
    0,
    // vendorId
    0x054C,
    // productId
    0x0CE6,
    // flags
    PADDLEBOAT_CONTROLLER_LAYOUT_SHAPES | PADDLEBOAT_CONTROLLER_FLAG_TOUCHPAD,
    // axisTableIndex
    0,
    // buttonTableIndex
    0,
    // deviceAllowlistStringTableIndex
    kPatchString_None,
    // deviceDenylistStringTableIndex
    kPatchString_None
};

typedef struct ControllerPatchData {
  const Paddleboat_Controller_Mapping_File_Axis_Entry *axis;
  const Paddleboat_Controller_Mapping_File_Button_Entry *button;
  const Paddleboat_Controller_Mapping_File_Controller_Entry *controller;
  const char * const *string_table;
  uint32_t string_count;
} ControllerPatchData;

static constexpr ControllerPatchData kControllerPatch {
    &kAxisPatchData,
    &kButtonPatchData,
    &kControllerPatchData,
    kPatchStringTable,
    kPatchString_Count
};

class ControllerPatcher {
public:
  ControllerPatcher(JNIEnv *env, const ControllerPatchData *patchData, const char *matchBrand,
                    const char *matchDevice, int matchMinApi);

  bool ApplyPatch();

private:
  ControllerPatcher() = default;
  bool IsMatchingDevice();
  void CreatePatchFile(const ControllerPatchData &patch_data);
  size_t CalculateMappingFileSize(const ControllerPatchData &patch_data);
  void InitializeHeader(const ControllerPatchData &patch_data);
  void PopulateFileData(const ControllerPatchData &patch_data);

  JNIEnv *mEnv;
  const ControllerPatchData *mPatchData;
  const char *mMatchBrand;
  const char *mMatchDevice;
  int mMatchMinApi;

  Paddleboat_Controller_Mapping_File_Header *mHeader;
  size_t mFileSize;
  size_t mStringBufferOffset;
};

ControllerPatcher::ControllerPatcher(JNIEnv *env, const ControllerPatchData *patchData,
                                     const char *matchBrand, const char *matchDevice,
                                     int matchMinApi) :
  mEnv(env),
  mPatchData(patchData),
  mMatchBrand(matchBrand),
  mMatchDevice(matchDevice),
  mMatchMinApi(matchMinApi) {
}

bool ControllerPatcher::ApplyPatch() {
  bool matchDevice = false;
  if (mPatchData != nullptr) {
    matchDevice = IsMatchingDevice();
    if (matchDevice) {
      CreatePatchFile(*mPatchData);
      Paddleboat_ErrorCode result = Paddleboat_addControllerRemapDataFromFileBuffer(
          PADDLEBOAT_REMAP_ADD_MODE_DEFAULT,
          reinterpret_cast<const void*>(mHeader), mFileSize);
      if (result != PADDLEBOAT_NO_ERROR) {
        ALOGE("Paddleboat_addControllerRemapDataFromFileBuffer returned %d", result);
      }
    }
  }
  return matchDevice;
}

bool ControllerPatcher::IsMatchingDevice() {
  jclass build_class = mEnv->FindClass(kAndroidBuildClass);
  if (mEnv->ExceptionCheck()) {
    mEnv->ExceptionClear();
    ALOGE("Failed to get Build class");
    return false;
  }

  std::string brandString = GetStaticStringField(mEnv, build_class, kBrandField);
  if (brandString.empty()) return false;
  std::string deviceString = GetStaticStringField(mEnv, build_class, kDeviceField);
  if (deviceString.empty()) return false;
  int api_level = android_get_device_api_level();

  if (api_level >= mMatchMinApi &&
      strcmp(brandString.c_str(), mMatchBrand) == 0 &&
      strcmp(deviceString.c_str(), mMatchDevice) == 0) {
    return true;
  }

  return false;
}

void ControllerPatcher::CreatePatchFile(const ControllerPatchData &patch_data) {
  mFileSize = CalculateMappingFileSize(patch_data);
  mHeader = reinterpret_cast<Paddleboat_Controller_Mapping_File_Header *>(
      malloc(mFileSize));
  InitializeHeader(patch_data);
  PopulateFileData(patch_data);
}

size_t ControllerPatcher::CalculateMappingFileSize(const ControllerPatchData &patch_data) {
  const size_t header_size =
      sizeof(Paddleboat_Controller_Mapping_File_Header);
  const size_t axis_size =
      sizeof(Paddleboat_Controller_Mapping_File_Axis_Entry);
  const size_t button_size =
      sizeof(Paddleboat_Controller_Mapping_File_Button_Entry);
  const size_t controller_size =
      sizeof(Paddleboat_Controller_Mapping_File_Controller_Entry);
  const size_t string_offsets_size = sizeof(uint64_t) * patch_data.string_count;
  const size_t string_buffer_size = kMaxStringSize * patch_data.string_count;

  const size_t file_size = header_size + axis_size + button_size +
                           controller_size + string_offsets_size + string_buffer_size;
  return file_size;
}

void ControllerPatcher::InitializeHeader(const ControllerPatchData &patch_data) {
  uint64_t offset = sizeof(Paddleboat_Controller_Mapping_File_Header);
  mHeader->fileIdentifier = PADDLEBOAT_MAPPING_FILE_IDENTIFIER;
  mHeader->libraryMinimumVersion = PADDLEBOAT_PACKED_VERSION;
  mHeader->axisTableEntryCount = 1;
  mHeader->buttonTableEntryCount = 1;
  mHeader->controllerTableEntryCount = 1;
  mHeader->stringTableEntryCount = patch_data.string_count;
  mHeader->axisTableOffset = offset;
  offset += sizeof(Paddleboat_Controller_Mapping_File_Axis_Entry);
  mHeader->buttonTableOffset = offset;
  offset += sizeof(Paddleboat_Controller_Mapping_File_Button_Entry);
  mHeader->controllerTableOffset = offset;
  offset += sizeof(Paddleboat_Controller_Mapping_File_Controller_Entry);
  mHeader->stringTableOffset = offset;
  offset += sizeof(uint64_t) * patch_data.string_count;
  mStringBufferOffset = offset;
}

void ControllerPatcher::PopulateFileData(const ControllerPatchData &patch_data) {
  uint8_t *base = reinterpret_cast<uint8_t*>(mHeader);

  uint8_t *axis = base + mHeader->axisTableOffset;
  memcpy(axis, patch_data.axis,
         sizeof(Paddleboat_Controller_Mapping_File_Axis_Entry));

  uint8_t *buttons = base + mHeader->buttonTableOffset;
  memcpy(buttons, patch_data.button,
         sizeof(Paddleboat_Controller_Mapping_File_Button_Entry));

  uint8_t *controller = base + mHeader->controllerTableOffset;
  memcpy(controller, patch_data.controller,
         sizeof(Paddleboat_Controller_Mapping_File_Controller_Entry));

  uint64_t *string_offsets = reinterpret_cast<uint64_t *>(
      (base + mHeader->stringTableOffset));
  uint64_t current_string_offset = mStringBufferOffset;
  for (uint32_t i = 0; i < mHeader->stringTableEntryCount; ++i) {
    string_offsets[i] = current_string_offset;
    uint8_t *string_buffer = base + current_string_offset;
    // Manual max length termination because strncpy doesn't do it
    // if it reaches max length, hence the -1 on size param.
    string_buffer[kMaxStringSize - 1] = 0;
    strncpy((char*)string_buffer, patch_data.string_table[i], kMaxStringSize - 1);
    current_string_offset += kMaxStringSize;
  }
}

ControllerRemapTest::ControllerRemapTest(JNIEnv *env) : mPatcher(new ControllerPatcher(
    env, &kControllerPatch, "brand", "device", 31)){
}

ControllerRemapTest::~ControllerRemapTest() {
  delete mPatcher;
}

bool ControllerRemapTest::DoRemapTest() {
  return mPatcher->ApplyPatch();
}
