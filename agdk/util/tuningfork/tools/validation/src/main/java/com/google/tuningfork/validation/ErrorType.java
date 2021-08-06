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

/**
 * Tuning Fork validation and parse errors. Some of these errors are shared between the validation
 * tool and the Play Console upload flows.
 */
public enum ErrorType {
  // Annotation field is empty.
  ANNOTATION_EMPTY(ErrorGroup.ANNOTATION),
  // Annotation field is too complex; contains oneofs/nestedtypes/extensions, etc.
  ANNOTATION_COMPLEX(ErrorGroup.ANNOTATION),
  // Annotation must contains enums only.
  ANNOTATION_TYPE(ErrorGroup.ANNOTATION),
  // FidelityParams fied is empty.
  FIDELITY_PARAMS_EMPTY(ErrorGroup.FIDELITY),
  // FidelityParams field is complex; contains oneof/nestedtypes/extensions.
  FIDELITY_PARAMS_COMPLEX(ErrorGroup.FIDELITY),
  // FidelityParams can only contains float, int32 or enum.
  FIDELITY_PARAMS_TYPE(ErrorGroup.FIDELITY),
  // Fidelity parameters are empty.
  DEV_FIDELITY_PARAMETERS_EMPTY(ErrorGroup.DEV_FIDELITY),
  // Fidelity parameters parse error.
  DEV_FIDELITY_PARAMETERS_PARSING(ErrorGroup.DEV_FIDELITY),
  // Fidelity parameters: error encoding textproto file.
  DEV_FIDELITY_PARAMETERS_ENCODING(ErrorGroup.DEV_FIDELITY),
  // Fidelity parameters: error reading file fidelity parameters file.
  DEV_FIDELITY_PARAMETERS_READING(ErrorGroup.DEV_FIDELITY),
  // Fidelity parameters: not in increasing/decreasing order.
  DEV_FIDELITY_PARAMETERS_ORDER(ErrorGroup.DEV_FIDELITY),
  // Fidelity parameters enums can't have zero-values.
  DEV_FIDELITY_PARAMETERS_ENUMS_ZERO(ErrorGroup.DEV_FIDELITY),
  // Failed to parse the settings file.
  SETTINGS_PARSING(ErrorGroup.SETTINGS),
  // Failed to find the settings file.
  SETTINGS_MISSING(ErrorGroup.SETTINGS),
  // No histogram specified in the settings.
  HISTOGRAM_EMPTY(ErrorGroup.SETTINGS),
  // Histogram should have n_Buckets set as at least one
  // Histogram may have non-negative Bucket_Min less
  // than Bucket_Max or none of them (the default is used)
  HISTOGRAM_BUCKET_INVALID(ErrorGroup.SETTINGS),
  // No aggregation strategy specified in the settings.
  AGGREGATION_EMPTY(ErrorGroup.SETTINGS),
  // Aggregation contains incorrect  max_instrumentation_keys field.
  AGGREGATION_INSTRUMENTATION_KEY(ErrorGroup.SETTINGS),
  // Aggregation contains incorrect annotation_enum_sizes.
  AGGREGATION_ANNOTATIONS(ErrorGroup.SETTINGS),
  // More than one descriptor file was found during the parse stage.
  // This should never happen. It is not a user error.
  TOO_MANY_DESCRIPTORS(ErrorGroup.DEV_FIDELITY),
  // No descriptor file was found during the parse stage.
  DESCRIPTOR_MISSING(ErrorGroup.DEV_FIDELITY),
  // Could not parse the descriptor file from the bundle/apk.
  DESCRIPTOR_PARSE_ERROR(ErrorGroup.DEV_FIDELITY),
  // Field base_uri is not a URL
  BASE_URI_NOT_URL(ErrorGroup.SETTINGS),
  // Field api_key is wrong
  API_KEY_INVALID(ErrorGroup.SETTINGS),
  // Field api_key is missing
  API_KEY_MISSING(ErrorGroup.SETTINGS),
  // Field initial_request_timeout_ms < 0
  INITIAL_REQUEST_TIMEOUT_INVALID(ErrorGroup.SETTINGS),
  // Field ultimate_request_timeout_ms < 0
  ULTIMATE_REQUEST_TIMEOUT_INVALID(ErrorGroup.SETTINGS),
  // Field loading_annotation_index is missing
  LOADING_ANNOTATION_INDEX_MISSING(ErrorGroup.SETTINGS);;

  private final ErrorGroup group;

  public ErrorGroup getGroup() {
    return group;
  }

  ErrorType(ErrorGroup group) {
    this.group = group;
  }

  /**
   * Validation group of errors
   */
  public enum ErrorGroup {
    ANNOTATION,
    FIDELITY,
    DEV_FIDELITY,
    SETTINGS,
  }
};
