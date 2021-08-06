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

import com.google.common.collect.LinkedListMultimap;
import com.google.common.collect.ListMultimap;
import com.google.common.collect.Multimap;
import com.google.common.flogger.FluentLogger;

/** Collects validation errors */
final class ParserErrorCollector implements ErrorCollector {
  private static final FluentLogger logger = FluentLogger.forEnclosingClass();

  private final ListMultimap<ErrorType, String> errors = LinkedListMultimap.create();
  private final ListMultimap<ErrorType, String> warnings = LinkedListMultimap.create();

  @Override
  public Multimap<ErrorType, String> getErrors() {
    return errors;
  }

  @Override
  public void addError(ErrorType errorType, String message) {
    errors.put(errorType, message);
  }

  @Override
  public void addError(ErrorType errorType, String message, Exception e) {
    errors.put(errorType, message);
  }

  @Override
  public Integer getErrorCount() {
    return errors.size();
  }

  @Override
  public Integer getErrorCount(ErrorType errorType) {
    return errors.get(errorType).size();
  }

  @Override
  public void printStatus() {
    StringBuilder builder = new StringBuilder();
    for (ErrorType errorType : ErrorType.values()) {
      int errorCount = errors.get(errorType).size();
      if (errorCount != 0) {
        builder
            .append(errorType)
            .append(" : ")
            .append(errorCount)
            .append(" ERRORS\n\t")
            .append(errors.get(errorType))
            .append("\n");
      }
    }
    logger.atWarning().log(builder.toString());
  }

  @Override
  public Boolean hasErrors(ErrorType.ErrorGroup group) {
    for (ErrorType errorType : ErrorType.values()) {
      if (errorType.getGroup() == group && errors.containsKey(errorType)) {
        return true;
      }
    }
    return false;
  }

  @Override
  public Boolean hasAnnotationErrors() {
    return hasErrors(ErrorType.ErrorGroup.ANNOTATION);
  }

  @Override
  public Boolean hasFidelityParamsErrors() {
    return hasErrors(ErrorType.ErrorGroup.FIDELITY);
  }

  @Override
  public Boolean hasSettingsErrors() {
    return hasErrors(ErrorType.ErrorGroup.SETTINGS);
  }

  @Override
  public Integer getWarningCount() {
    return warnings.size();
  }

  @Override
  public Integer getWarningCount(ErrorType errorType) {
    return warnings.get(errorType).size();
  }

  @Override
  public void addWarning(ErrorType errorType, String message) {
    warnings.put(errorType, message);
  }

  @Override
  public Multimap<ErrorType, String> getWarnings() {
    return warnings;
  }

}
