/*
 * Copyright (C) 2019 The Android Open Source Project
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
 * limitations under the License
 */


package com.google.tuningfork.validation;

import java.util.regex.Pattern;

/** Exposes Tuning Fork location patterns for apks/bundles. */
final class TuningForkPaths {

  static final String BUNDLE_TUNINGFORK_PATH = "base/assets/tuningfork/";
  static final String APK_TUNINGFORK_PATH = "assets/tuningfork/";

  static Pattern getDescriptorFilePattern(String base) {
    return Pattern.compile(String.format("(%s)dev_tuningfork.descriptor", base));
  }

  static Pattern getSettingsFilePattern(String base) {
    return Pattern.compile(String.format("(%s)tuningfork_settings.bin", base));
  }

  static Pattern getFidelityParamFilePattern(String base) {
    return Pattern.compile(String.format("(%s)dev_tuningfork_fidelityparams_(\\d+).bin", base));
  }
}
