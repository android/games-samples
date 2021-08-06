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

import com.google.common.collect.ImmutableList;
import com.google.protobuf.ByteString;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.DynamicMessage;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.TextFormat;
import com.google.protobuf.TextFormat.ParseException;
import com.google.tuningfork.Tuningfork.Settings;
import com.google.tuningfork.Tuningfork.Settings.Histogram;
import com.google.tuningfork.Tuningfork.Settings.AggregationStrategy;
import java.net.URL;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Optional;
import java.util.stream.IntStream;
import java.util.stream.Stream;
import org.checkerframework.checker.nullness.Opt;

/** Utility methods for validating Tuning Fork protos and settings. */
final class ValidationUtil {

  private ValidationUtil() {}

  private static final Integer MAX_INTSTRUMENTATION_KEYS = 256;

  private static final ImmutableList<FieldDescriptor.Type> ALLOWED_FIDELITYPARAMS_TYPES =
      ImmutableList.of(
          FieldDescriptor.Type.ENUM, FieldDescriptor.Type.FLOAT, FieldDescriptor.Type.INT32);

  /* Validate settings */
  public static Optional<Settings> validateSettings(
      List<Integer> enumSizes, String settingsTextProto, ErrorCollector errors) {
    try {
      Settings.Builder builder = Settings.newBuilder();
      TextFormat.merge(settingsTextProto, builder);
      Settings settings = builder.build();
      validateSettings(settings, enumSizes, errors);
      return Optional.of(settings);
    } catch (ParseException e) {
      errors.addError(ErrorType.SETTINGS_PARSING, "Parsing tuningfork_settings.txt", e);
      return Optional.empty();
    }
  }

  /* Validate settings */
  public static final void validateSettings(
      List<Integer> enumSizes, ByteString settingsContent, ErrorCollector errors) {
    try {
      Settings settings = Settings.parseFrom(settingsContent);
      validateSettings(settings, enumSizes, errors);
    } catch (InvalidProtocolBufferException e) {
      errors.addError(ErrorType.SETTINGS_PARSING, "Parsing tuningfork_settings.bin", e);
    }
  }

  /* Validate settings */
  public static final void validateSettings(
      Settings settings, List<Integer> enumSizes, ErrorCollector errors) {
    validateSettingsAggregation(settings, enumSizes, errors);
    validateSettingsHistograms(settings, errors);
    validateSettingsBaseUri(settings, errors);
    validateSettingsApiKey(settings, errors);
    // We don't validate default_fidelity_parameters_filename
    validateSettingsRequestTimeouts(settings, errors);
    validateSettingsLoadingAnnotationIndex(settings, errors);
  }

  /*
   * Validate Histograms
   * Tuning Fork Scaled allows empty settings (no warnings will be collected in that case).
   * Number of buckets should be valid (may not be specified)
   * The max and min fps should cover 30 or 60 (in milliseconds that is 1000 / 30 and 1000/60
   * which is roughly 33.3 and 16.7)
   * If they are not there, the correct default will be chosen and no check is needed
   * */
  public static void validateSettingsHistograms(Settings settings, ErrorCollector errors) {
    final float FRAME_TIME_60FPS_MS = 16.7f;
    final float FRAME_TIME_30FPS_MS = 33.3f;
    List<Settings.Histogram> histograms = settings.getHistogramsList();
    for (Settings.Histogram histogram: histograms) {
      if (histogram.getBucketMin() == 0 && histogram.getBucketMax() == 0) {
        continue;
      }
      if (histogram.getNBuckets() < 1) {
        errors.addWarning(ErrorType.HISTOGRAM_BUCKET_INVALID,
                "Set n_Buckets to a positive integer. It is currently "
                        + histogram.getNBuckets());
      }
      boolean cover60fps = histogram.getBucketMin() < FRAME_TIME_60FPS_MS
              && histogram.getBucketMax() > FRAME_TIME_60FPS_MS;
      boolean cover30fps = histogram.getBucketMax() > FRAME_TIME_30FPS_MS
              && histogram.getBucketMin() < FRAME_TIME_30FPS_MS;
      if (histogram.hasBucketMax() && histogram.getBucketMax() < histogram.getBucketMin()) {
        errors.addWarning(ErrorType.HISTOGRAM_BUCKET_INVALID,
                "Bucket_Min has to be less than Bucket_Max. Max currently is "
                        + histogram.getBucketMax()
                        + "Min currently is " + histogram.getBucketMin());
      }

      if (histogram.getBucketMin() < 0.0f || histogram.getBucketMax() < 0.0f) {
        errors.addWarning(ErrorType.HISTOGRAM_BUCKET_INVALID,
                "Bucket_Min or Bucket_Max covers negative fps");
      }
      if (!cover30fps && !cover60fps) {
        errors.addWarning(ErrorType.HISTOGRAM_BUCKET_INVALID,
                "Histogram does not cover neither 30 nor 60 fps. It covers from "
                +  1000 / histogram.getBucketMax() + " to "
                        + 1000 / histogram.getBucketMin() + " fps");
      }
      if (cover30fps ^ cover60fps) {
        int num = cover30fps ? 30 : 60;
        errors.addWarning(ErrorType.HISTOGRAM_BUCKET_INVALID,
                "Histogram covers only " + num + " fps");
      }
    }
  }

  /*
   * Validate Aggregation
   * */
  public static void validateSettingsAggregation(
      Settings settings, List<Integer> enumSizes, ErrorCollector errors) {

    if (!settings.hasAggregationStrategy()) {
      errors.addError(ErrorType.AGGREGATION_EMPTY, "Aggregation message is null");
      return;
    }

    AggregationStrategy aggregation = settings.getAggregationStrategy();

    if (!aggregation.hasMaxInstrumentationKeys()) {
      errors.addError(
          ErrorType.AGGREGATION_INSTRUMENTATION_KEY,
          "Aggregation strategy doesn't have max_instrumentation_key field");
    }
    int maxKey = aggregation.getMaxInstrumentationKeys();
    if (maxKey < 1 || maxKey > MAX_INTSTRUMENTATION_KEYS) {
      errors.addError(
          ErrorType.AGGREGATION_INSTRUMENTATION_KEY,
          String.format(
              "max_instrumentation_keys should be between 1 and %d, current value is %d",
              MAX_INTSTRUMENTATION_KEYS, maxKey));
    }

    int annotationCount = aggregation.getAnnotationEnumSizeCount();
    if (annotationCount != enumSizes.size()) {
      errors.addError(
          ErrorType.AGGREGATION_ANNOTATIONS,
          "\"tuningfork_settings.bin\" should contains same number of annotations"
              + " as \"dev_tuningfork.proto\"");
      return;
    }

    for (int i = 0; i < annotationCount; ++i) {
      Integer annSize = (Integer) aggregation.getAnnotationEnumSize(i);
      if (!annSize.equals(enumSizes.get(i))) {
        errors.addError(
            ErrorType.AGGREGATION_ANNOTATIONS,
            String.format(
                "\"tuningfork_settings.bin\" should contains same annotations as "
                    + "\"dev_tuningfork.proto\", expected %d, but was %d",
                enumSizes.get(i), annSize));
      }
    }
  }

  private static final float getValueHelper(int i, List<DynamicMessage> fidelityMessages,
      FieldDescriptor field) {
    Object fieldValue = fidelityMessages.get(i).getField(field);
    if (fieldValue instanceof EnumValueDescriptor) {
      return ((EnumValueDescriptor) fieldValue).getNumber();
    } else if (fieldValue instanceof Integer) {
      return (float) ((Integer) fieldValue).intValue();
    }
    return (float) fieldValue;
  }

  /*
   * Validate content of fidelity parameters from all dev_tuningfork_fidelityparams_*.bin files.
   * These should be in either increasing/decreasing order.
   * Only works for int32, float, enums.
   */
  public static final void validateDevFidelityParamsOrder(
      Descriptor fidelityParamsDesc,
      List<DynamicMessage> fidelityMessages,
      ErrorCollector errors) {
    long isIncreasing = fidelityParamsDesc
        .getFields()
        .stream()
        .filter(field ->
            IntStream.range(1, fidelityMessages.size())
                .allMatch(
                    i -> getValueHelper(i - 1, fidelityMessages, field) <= getValueHelper(i,
                        fidelityMessages, field)))
        .count();

    long isDecreasing = fidelityParamsDesc
        .getFields()
        .stream()
        .filter(field ->
            IntStream.range(1, fidelityMessages.size())
                .allMatch(
                    i -> getValueHelper(i - 1, fidelityMessages, field) >= getValueHelper(i,
                        fidelityMessages, field)))
        .count();

    long allEqual = fidelityParamsDesc
        .getFields()
        .stream()
        .filter(field ->
            IntStream.range(1, fidelityMessages.size())
                .allMatch(
                    i -> getValueHelper(i - 1, fidelityMessages, field) == getValueHelper(i,
                        fidelityMessages, field)))
        .count();

    if (isIncreasing + isDecreasing - allEqual != fidelityParamsDesc.getFields().size()) {
      errors
          .addWarning(ErrorType.DEV_FIDELITY_PARAMETERS_ORDER, "Fidelity parameters should be " +
              "in either increasing or decreasing order.");
    }
  }

  /*
   * Validate content of fidelity parameters from all dev_tuningfork_fidelityparams_*.bin files.
   * Enums usually have values different than 0.
   */
  public static final void validateDevFidelityParamsZero(
      Descriptor fidelityParamsDesc,
      List<DynamicMessage> fidelityMessages,
      ErrorCollector errors) {
    Stream<FieldDescriptor> hasZeroEnum = fidelityParamsDesc
        .getFields()
        .stream()
        .filter(field -> fidelityMessages.stream()
            .filter(message -> message.getField(field) instanceof EnumValueDescriptor).
                anyMatch(
                    message -> ((EnumValueDescriptor) message.getField(field)).getNumber() == 0)
        );

    if (hasZeroEnum.count() > 0) {
      errors.addWarning(ErrorType.DEV_FIDELITY_PARAMETERS_ENUMS_ZERO, "A");
    }
  }

  public static final List<DynamicMessage> validateDevFidelityParams(
      Collection<ByteString> devFidelityList,
      Descriptor fidelityParamsDesc,
      ErrorCollector errors) {
    if (devFidelityList.isEmpty()) {
      errors.addError(ErrorType.DEV_FIDELITY_PARAMETERS_EMPTY, "Fidelity parameters list is empty");
      return null;
    }
    List<DynamicMessage> fidelityMessages = new ArrayList<>();
    devFidelityList.forEach(
        entry -> fidelityMessages
            .add(validateDevFidelityParams(entry, fidelityParamsDesc, errors).get()));
    return fidelityMessages;
  }

  /*
   * Validate content of dev_tuningfork_fidelityparams_*.bin files
   * Each file should be parsed to Fidelity proto
   * */
  public static final Optional<DynamicMessage> validateDevFidelityParams(
      ByteString devFidelityContent, Descriptor fidelityParamsDesc, ErrorCollector errors) {
    Optional<DynamicMessage> fidelityMessage = Optional.empty();
    try {
      fidelityMessage = Optional.of(
          DynamicMessage.parseFrom(fidelityParamsDesc, devFidelityContent));
      if (!fidelityMessage.isPresent()) {
        errors.addError(ErrorType.DEV_FIDELITY_PARAMETERS_EMPTY, "Fidelity parameters is empty");
      }
    } catch (InvalidProtocolBufferException e) {
      errors.addError(
          ErrorType.DEV_FIDELITY_PARAMETERS_PARSING, "Fidelity parameters not parsed properly", e);
    }
    return fidelityMessage;
  }

  /*
   * Validate FidelityParams in proto file.
   * FidelityParams message can only contains ENUM, FLOAT and INT32.
   * FidelityParams should not be complex - no oneofs, nested types or extensions
   * Enums should not start with 0 index.
   * */
  public static final void validateFidelityParams(
      Descriptor fidelityParamsDesc, ErrorCollector errors) {
    if (fidelityParamsDesc == null) {
      errors.addError(ErrorType.FIDELITY_PARAMS_EMPTY, "FidelityParams descriptor is not exist");
      return;
    }

    if (!fidelityParamsDesc.getOneofs().isEmpty()) {
      errors.addError(
          ErrorType.FIDELITY_PARAMS_COMPLEX, "FidelityParams too complex - has oneof field");
      return;
    }

    if (!fidelityParamsDesc.getNestedTypes().isEmpty()) {
      errors.addError(
          ErrorType.FIDELITY_PARAMS_COMPLEX, "FidelityParams too complex - has nested types");
      return;
    }

    if (!fidelityParamsDesc.getExtensions().isEmpty()) {
      errors.addError(
          ErrorType.FIDELITY_PARAMS_COMPLEX, "FidelityParams too complex - has extensions");
      return;
    }

    for (FieldDescriptor field : fidelityParamsDesc.getFields()) {
      FieldDescriptor.Type type = field.getType();

      if (!ALLOWED_FIDELITYPARAMS_TYPES.contains(type)) {
        errors.addError(
            ErrorType.FIDELITY_PARAMS_TYPE,
            String.format(
                "FidelityParams can only be of type FLOAT, INT32 or ENUM, %s field has type %s",
                field.getName(), type));
      }
    }
  }

  /**
   * Validate Annotation field in proto file. Annotation can only contains enums, each enum should
   * not start from 0 index. Annotation message should not be complex - no oneof, nestedTypes ot
   * extensions Check for all _fields_ in proto Return list of enum size for annotation's field
   */
  public static final ImmutableList<Integer> validateAnnotationAndGetEnumSizes(
      Descriptor annotationDesc, ErrorCollector errors) {
    if (annotationDesc == null) {
      errors.addError(ErrorType.ANNOTATION_EMPTY, "Annotation descriptor is not exist");
      return null;
    }

    if (!annotationDesc.getOneofs().isEmpty()) {
      errors.addError(ErrorType.ANNOTATION_COMPLEX, "Annotation too complex - has oneofs");
      return null;
    }

    if (!annotationDesc.getNestedTypes().isEmpty()) {
      errors.addError(ErrorType.ANNOTATION_COMPLEX, "Annotation too complex - has nested types");
      return null;
    }

    if (!annotationDesc.getExtensions().isEmpty()) {
      errors.addError(ErrorType.ANNOTATION_COMPLEX, "Annotation too complex - hes extension");
      return null;
    }

    List<Integer> sizes = new ArrayList<>();

    for (FieldDescriptor field : annotationDesc.getFields()) {
      if (field.getType() != FieldDescriptor.Type.ENUM) {
        errors.addError(
            ErrorType.ANNOTATION_TYPE,
            String.format(
                "Annotation can only contains enums, but contains %s field with type %s",
                field.getName(), field.getType()));
        return null;
      }
      sizes.add(field.getEnumType().getValues().size());
    }
    return ImmutableList.copyOf(sizes);
  }

  private static final void validateSettingsBaseUri(Settings settings, ErrorCollector errors) {
    if (settings.hasBaseUri()) {
      // Check it's a valid URL
      final URL url;
      try {
        url = new URL(settings.getBaseUri());
        url.getHost();
      } catch (Exception e) {
        errors.addError(ErrorType.BASE_URI_NOT_URL, "base_uri is not a valid URL");
      }
    }
    // Missing is OK
  }

  private static final void validateSettingsApiKey(Settings settings, ErrorCollector errors) {
    if (settings.hasApiKey()) {
      // This checks that someone has changed from the default value in the samples.
      if (settings.getApiKey().equals("enter-your-api-key-here")) {
        errors.addError(ErrorType.API_KEY_INVALID, "api_key not set to a valid value");
      }
    } else {
      errors.addError(ErrorType.API_KEY_MISSING, "api_key is missing");
    }
  }

  private static final void validateSettingsLoadingAnnotationIndex(
      Settings settings, ErrorCollector errors) {
    if (!settings.hasLoadingAnnotationIndex()) {
      errors.addWarning(ErrorType.LOADING_ANNOTATION_INDEX_MISSING,
          "loading_annotation_index not set");
    }
  }

  private static final void validateSettingsRequestTimeouts(
      Settings settings, ErrorCollector errors) {
    if (settings.hasInitialRequestTimeoutMs()) {
      int initialRequestTimeoutMs = settings.getInitialRequestTimeoutMs();
      if (initialRequestTimeoutMs < 0) {
        errors.addError(ErrorType.INITIAL_REQUEST_TIMEOUT_INVALID,
            "initial_request_timeout_ms is invalid");
      }
    }
    if (settings.hasUltimateRequestTimeoutMs()) {
      int ultimateRequestTimeoutMs = settings.getUltimateRequestTimeoutMs();
      if (ultimateRequestTimeoutMs < 0) {
        errors.addError(ErrorType.ULTIMATE_REQUEST_TIMEOUT_INVALID,
            "ultimate_request_timeout_ms is invalid");
      }
    }
  }
}
