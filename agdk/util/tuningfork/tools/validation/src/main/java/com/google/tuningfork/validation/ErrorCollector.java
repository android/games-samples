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

import com.google.common.collect.Multimap;

/** Collecting validation errors */
public interface ErrorCollector {
  void addError(ErrorType errorType, String message);

  void addError(ErrorType errorType, String message, Exception e);

  Integer getErrorCount();

  Integer getErrorCount(ErrorType errorType);

  void printStatus();

  Boolean hasErrors(ErrorType.ErrorGroup group);

  Boolean hasAnnotationErrors();

  Boolean hasFidelityParamsErrors();

  Boolean hasSettingsErrors();

  Multimap<ErrorType, String> getErrors();

  void addWarning(ErrorType errorType, String message);

  Multimap<ErrorType, String> getWarnings();

  Integer getWarningCount();

  Integer getWarningCount(ErrorType errorType);
};
