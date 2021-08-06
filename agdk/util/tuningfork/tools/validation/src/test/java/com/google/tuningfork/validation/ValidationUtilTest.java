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

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.ByteString;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.DynamicMessage;
import com.google.tuningfork.Tuningfork.Settings;
import com.google.tuningfork.Tuningfork.Settings.AggregationStrategy;
import com.google.tuningfork.Tuningfork.Settings.Histogram;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Optional;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)

public final class ValidationUtilTest {

  @Rule
  // Override default behavior to allow overwriting files.
  public TemporaryFolder tempFolder =
      new TemporaryFolder() {
        @Override
        public File newFile(String filename) {
          return new File(getRoot(), filename);
        }
      };

  private final ProtoCompilerHelper helper = new ProtoCompilerHelper(tempFolder);

  final String tuningforkPath = "assets/tuningfork";

  private final ErrorCollector errors = new ParserErrorCollector();

  @Test
  public void validateSettingsHistogramValid() throws Exception {
    Histogram histogram = Histogram.newBuilder()
        .setBucketMin(6.54f)
        .setBucketMax(60f)
        .setNBuckets(30)
        .setInstrumentKey(0)
        .build();
    Settings settings = Settings.newBuilder().addHistograms(histogram).build();
    ValidationUtil.validateSettingsHistograms(settings, errors);
    assertThat(errors.getWarningCount()).isEqualTo(0);
  }

  @Test
  public void validateSettingsHistogramNegativeCoverage() throws Exception {
    Histogram histogram = Histogram.newBuilder()
        .setBucketMin(-10f)
        .setBucketMax(10f)
        .setNBuckets(30)
        .setInstrumentKey(0)
        .build();
    Settings settings = Settings.newBuilder().addHistograms(histogram).build();
    ValidationUtil.validateSettingsHistograms(settings, errors);
    assertThat(errors.getWarningCount()).isEqualTo(2);
  }

  @Test
  public void validateSettingsHistogramNegativeNBuckets() throws Exception {
    Histogram histogram = Histogram.newBuilder()
        .setBucketMin(0f)
        .setBucketMax(100f)
        .setNBuckets(-10)
        .setInstrumentKey(0)
        .build();
    Settings settings = Settings.newBuilder().addHistograms(histogram).build();
    ValidationUtil.validateSettingsHistograms(settings, errors);
    assertThat(errors.getWarningCount()).isEqualTo(1);
  }

  @Test
  public void validateSettingsHistogramCoversNothing() throws Exception {
    Histogram histogram = Histogram.newBuilder()
        .setBucketMin(100f)
        .setBucketMax(1000f)
        .setNBuckets(30)
        .setInstrumentKey(0)
        .build();
    Settings settings = Settings.newBuilder().addHistograms(histogram).build();
    ValidationUtil.validateSettingsHistograms(settings, errors);
    assertThat(errors.getWarningCount()).isEqualTo(1);
  }

  @Test
  public void validateSettingsHistogramCoversOne() throws Exception {
    Histogram histogram = Histogram.newBuilder()
        .setBucketMin(0f)
        .setBucketMax(20f)
        .setNBuckets(30)
        .setInstrumentKey(0)
        .build();
    Settings settings = Settings.newBuilder().addHistograms(histogram).build();
    ValidationUtil.validateSettingsHistograms(settings, errors);
    assertThat(errors.getWarningCount()).isEqualTo(1);
  }

  @Test
  public void validateSettingsHistogramWrongMinMaxOrder() throws Exception {
    Histogram histogram = Histogram.newBuilder()
        .setBucketMin(100f)
        .setBucketMax(0f)
        .setNBuckets(30)
        .setInstrumentKey(0)
        .build();
    Settings settings = Settings.newBuilder().addHistograms(histogram).build();
    ValidationUtil.validateSettingsHistograms(settings, errors);
    assertThat(errors.getWarningCount()).isEqualTo(2);
  }

  @Test
  public void settingsWithHistogramsValid() throws Exception {
    AggregationStrategy aggregation =
            AggregationStrategy.newBuilder()
                    .addAllAnnotationEnumSize(Arrays.asList(5, 10))
                    .setMaxInstrumentationKeys(100)
                    .build();
    Histogram histogram = Histogram.newBuilder()
            .setBucketMin(0f)
            .setBucketMax(100f)
            .setNBuckets(30)
            .setInstrumentKey(0)
            .build();
    Settings settings =
            Settings.newBuilder()
                    .setAggregationStrategy(aggregation)
                    .addHistograms(histogram)
                    .setApiKey("test-api-key")
                    .setLoadingAnnotationIndex(1)
                    .build();

    Optional<Settings> parsedSettings =
            ValidationUtil.validateSettings(Arrays.asList(5, 10), settings.toString(), errors);

    assertThat(errors.getErrorCount()).isEqualTo(0);
    assertThat(errors.getWarningCount()).isEqualTo(0);
    assertThat(parsedSettings.get()).isEqualTo(settings);
  }

  @Test
  public void settingsValid() throws Exception {
    AggregationStrategy aggregation =
        AggregationStrategy.newBuilder()
            .addAllAnnotationEnumSize(Arrays.asList(5, 10))
            .setMaxInstrumentationKeys(100)
            .build();
    Settings settings =
        Settings.newBuilder()
            .setAggregationStrategy(aggregation)
            .setApiKey("test-api-key")
            .setLoadingAnnotationIndex(1)
            .build();

    Optional<Settings> parsedSettings =
        ValidationUtil.validateSettings(Arrays.asList(5, 10), settings.toString(), errors);

    assertThat(errors.getErrorCount()).isEqualTo(0);
    assertThat(errors.getWarningCount()).isEqualTo(0);
    assertThat(parsedSettings.get()).isEqualTo(settings);
  }

    @Test
  public void settingsLoadingAnnotationIndexWarning() throws Exception {
    Settings settings = Settings.getDefaultInstance();

    ValidationUtil.validateSettings(Arrays.asList(5, 10), settings.toString(), errors);

    assertThat(errors.getWarningCount()).isEqualTo(1);
    assertThat(errors.getWarnings().containsKey(ErrorType.LOADING_ANNOTATION_INDEX_MISSING))
        .isTrue();
  }

  @Test
  public void settingsHistogramsValid() throws Exception {
    Settings settings = Settings.newBuilder().addHistograms(Histogram.getDefaultInstance()).build();

    ValidationUtil.validateSettingsHistograms(settings, errors);
    assertThat(errors.getErrorCount()).isEqualTo(0);
  }

  @Test
  public void settingsAggregationValid() throws Exception {
    AggregationStrategy aggregation =
        AggregationStrategy.newBuilder()
            .addAllAnnotationEnumSize(Arrays.asList(5, 10))
            .setMaxInstrumentationKeys(100)
            .build();

    Settings settings = Settings.newBuilder().setAggregationStrategy(aggregation).build();

    ValidationUtil.validateSettingsAggregation(settings, Arrays.asList(5, 10), errors);
    assertThat(errors.getErrorCount()).isEqualTo(0);
  }

  @Test
  public void settingsAggregationIncorrectCount() throws Exception {
    AggregationStrategy aggregation =
        AggregationStrategy.newBuilder()
            .addAllAnnotationEnumSize(Arrays.asList(5, 10))
            .setMaxInstrumentationKeys(100)
            .build();

    Settings settings = Settings.newBuilder().setAggregationStrategy(aggregation).build();

    ValidationUtil.validateSettingsAggregation(settings, Arrays.asList(5, 10, 20), errors);
    assertThat(errors.getErrorCount(ErrorType.AGGREGATION_ANNOTATIONS)).isEqualTo(1);
    assertThat(errors.getErrorCount()).isEqualTo(1);
  }

  @Test
  public void settingsAggregationWrongInstrumentationKey() throws Exception {
    AggregationStrategy aggregation =
        AggregationStrategy.newBuilder()
            .addAllAnnotationEnumSize(Arrays.asList(5, 10))
            .setMaxInstrumentationKeys(99999)
            .build();

    Settings settings = Settings.newBuilder().setAggregationStrategy(aggregation).build();
    ValidationUtil.validateSettingsAggregation(settings, Arrays.asList(5, 10), errors);
    assertThat(errors.getErrorCount(ErrorType.AGGREGATION_INSTRUMENTATION_KEY)).isEqualTo(1);
    assertThat(errors.getErrorCount()).isEqualTo(1);
  }

  @Test
  public void settingsAggregationIncorrectSizes() throws Exception {
    AggregationStrategy aggregation =
        AggregationStrategy.newBuilder()
            .addAllAnnotationEnumSize(Arrays.asList(5, 10, 20))
            .setMaxInstrumentationKeys(100)
            .build();

    Settings settings = Settings.newBuilder().setAggregationStrategy(aggregation).build();

    ValidationUtil.validateSettingsAggregation(settings, Arrays.asList(6, 11, 21, 31), errors);
    assertThat(errors.getErrorCount(ErrorType.AGGREGATION_ANNOTATIONS)).isEqualTo(1);
    assertThat(errors.getErrorCount()).isEqualTo(1);
  }

  @Test
  public void devFidelityParamsValid() throws Exception {
    ByteString content = TestdataHelper.readByteString("dev_tuningfork_fidelityparams_valid.bin");
    Descriptor desc =
        helper.getDescriptor(
            "fidelity_params_valid.proto",
            "Getting FidelityParams field from proto",
            "FidelityParams");

    ValidationUtil.validateDevFidelityParams(content, desc, errors);
    assertThat(errors.getErrorCount()).isEqualTo(0);
  }

  @Test
  public void devFidelityParamsInvalid() throws Exception {
    Descriptor desc =
        helper.getDescriptor(
            "fidelity_params_valid.proto",
            "Getting FidelityParams field from proto",
            "FidelityParams");

    // not a valid file
    ByteString content = TestdataHelper.readByteString("annotation_complex.proto");

    ValidationUtil.validateDevFidelityParams(content, desc, errors);
    assertThat(errors.getErrorCount(ErrorType.DEV_FIDELITY_PARAMETERS_PARSING)).isEqualTo(1);
    assertThat(errors.getErrorCount()).isEqualTo(1);
  }

  @Test
  public void fidelityParamsComplex() throws Exception {
    Descriptor desc =
        helper.getDescriptor(
            "fidelity_params_complex.proto",
            "Getting FidelityParams field from proto",
            "FidelityParams");
    assertThat(desc).isNotNull();
    ValidationUtil.validateFidelityParams(desc, errors);
    assertThat(errors.getErrorCount(ErrorType.FIDELITY_PARAMS_COMPLEX)).isEqualTo(1);
    assertThat(errors.getErrorCount()).isEqualTo(1);
  }

  @Test
  public void fidelityParamsNotScalar() throws Exception {
    Descriptor desc =
        helper.getDescriptor(
            "fidelity_params_not_scalar.proto",
            "Getting FidelityParams field from proto",
            "FidelityParams");
    assertThat(desc).isNotNull();
    ValidationUtil.validateFidelityParams(desc, errors);
    assertThat(errors.getErrorCount(ErrorType.FIDELITY_PARAMS_TYPE)).isEqualTo(1);
    assertThat(errors.getErrorCount()).isEqualTo(1);
  }

  @Test
  public void fidelityParamsValid() throws Exception {
    Descriptor desc =
        helper.getDescriptor(
            "fidelity_params_valid.proto",
            "Getting FidelityParams field from proto",
            "FidelityParams");
    assertThat(desc).isNotNull();
    ValidationUtil.validateFidelityParams(desc, errors);
    assertThat(errors.getErrorCount()).isEqualTo(0);
  }

  @Test
  public void annotationComplex() throws Exception {
    Descriptor desc =
        helper.getDescriptor(
            "annotation_complex.proto", "Getting Annotation field from proto", "Annotation");
    assertThat(desc).isNotNull();
    assertThat(ValidationUtil.validateAnnotationAndGetEnumSizes(desc, errors)).isEqualTo(null);
    assertThat(errors.getErrorCount(ErrorType.ANNOTATION_COMPLEX)).isEqualTo(1);
    assertThat(errors.getErrorCount()).isEqualTo(1);
  }

  @Test
  public void annotationEmpty() throws Exception {
    assertThat(ValidationUtil.validateAnnotationAndGetEnumSizes(null, errors)).isNull();
    assertThat(errors.getErrorCount(ErrorType.ANNOTATION_EMPTY)).isEqualTo(1);
    assertThat(errors.getErrorCount()).isEqualTo(1);
  }

  @Test
  public void annotationNotOnlyEnums() throws Exception {
    Descriptor desc =
        helper.getDescriptor(
            "annotation_not_only_enums.proto", "Getting Annotation field from proto", "Annotation");
    assertThat(desc).isNotNull();
    assertThat(ValidationUtil.validateAnnotationAndGetEnumSizes(desc, errors)).isNull();
    assertThat(errors.getErrorCount(ErrorType.ANNOTATION_TYPE)).isEqualTo(1);
    assertThat(errors.getErrorCount()).isEqualTo(1);
  }

  @Test
  public void annotationValid() throws Exception {
    Descriptor desc =
        helper.getDescriptor(
            "annotation_valid.proto", "Getting Annotation field from proto", "Annotation");
    assertThat(desc).isNotNull();
    assertThat(ValidationUtil.validateAnnotationAndGetEnumSizes(desc, errors))
        .isEqualTo(Arrays.asList(2, 3, 6, 6));
    assertThat(errors.getErrorCount()).isEqualTo(0);
  }

  @Test
  public void settingsBaseURIAPIKeyAndTimesInvalid() throws Exception {
    AggregationStrategy aggregation =
        AggregationStrategy.newBuilder()
            .addAllAnnotationEnumSize(Arrays.asList(5, 10))
            .setMaxInstrumentationKeys(100)
            .build();
    Settings settings =
        Settings.newBuilder()
            .setAggregationStrategy(aggregation)
            .addHistograms(Histogram.getDefaultInstance())
            .setBaseUri("not-a-uri*&")
            // Missing API key
            .setInitialRequestTimeoutMs(-1)
            .setUltimateRequestTimeoutMs(-1)
            .build();

    ValidationUtil.validateSettings(Arrays.asList(5, 10), settings.toString(), errors);

    assertThat(errors.getErrorCount(ErrorType.BASE_URI_NOT_URL)).isEqualTo(1);
    assertThat(errors.getErrorCount(ErrorType.API_KEY_MISSING)).isEqualTo(1);
    assertThat(errors.getErrorCount(ErrorType.API_KEY_INVALID)).isEqualTo(0);
    assertThat(errors.getErrorCount(ErrorType.INITIAL_REQUEST_TIMEOUT_INVALID)).isEqualTo(1);
    assertThat(errors.getErrorCount(ErrorType.ULTIMATE_REQUEST_TIMEOUT_INVALID)).isEqualTo(1);
  }

  @Test
  public void settingsBadApiKey() throws Exception {
    AggregationStrategy aggregation =
        AggregationStrategy.newBuilder()
            .addAllAnnotationEnumSize(Arrays.asList(5, 10))
            .setMaxInstrumentationKeys(100)
            .build();
    Settings settings2 =
        Settings.newBuilder()
            .setAggregationStrategy(aggregation)
            .addHistograms(Histogram.getDefaultInstance())
            .setApiKey("enter-your-api-key-here")
            .build();

    ValidationUtil.validateSettings(Arrays.asList(5, 10), settings2.toString(), errors);

    assertThat(errors.getErrorCount(ErrorType.API_KEY_INVALID)).isEqualTo(1);
  }

  @Test
  public void checkZeroEnumWarning() throws Exception {
    Descriptor desc =
        helper.getDescriptor(
            "dev_tuningfork.proto",
            "Getting FidelityParams field from proto",
            "FidelityParams");

    List<FieldDescriptor> fieldDescriptors = desc.getFields();
    List<EnumValueDescriptor> enumValues = fieldDescriptors.get(0).getEnumType().getValues();
    DynamicMessage message1 = DynamicMessage.newBuilder(desc)
        .setField(fieldDescriptors.get(0), enumValues.get(2))
        .setField(fieldDescriptors.get(1), 10) // int field
        .setField(fieldDescriptors.get(2), (float) 1.2) // float field
        .build();
    DynamicMessage message2 = DynamicMessage.newBuilder(desc)
        .setField(fieldDescriptors.get(0), enumValues.get(0))
        .setField(fieldDescriptors.get(1), 9) //int field
        .setField(fieldDescriptors.get(2), (float) 1.24) // float field
        .build();
    List<DynamicMessage> dynamicMessages = new ArrayList<>();
    dynamicMessages.add(message1);
    dynamicMessages.add(message2);

    ValidationUtil.validateDevFidelityParamsZero(desc, dynamicMessages, errors);

    assertThat(errors.getWarningCount(ErrorType.DEV_FIDELITY_PARAMETERS_ENUMS_ZERO)).isEqualTo(1);
  }

  @Test
  public void devParamsCorrectOrder() throws Exception {
    Descriptor desc =
        helper.getDescriptor(
            "dev_tuningfork.proto",
            "Getting FidelityParams field from proto",
            "FidelityParams");

    List<FieldDescriptor> fieldDescriptors = desc.getFields();
    List<EnumValueDescriptor> enumValues = fieldDescriptors.get(0).getEnumType().getValues();
    DynamicMessage message1 = DynamicMessage.newBuilder(desc)
        .setField(fieldDescriptors.get(0), enumValues.get(2))
        .setField(fieldDescriptors.get(1), 10) // int field
        .setField(fieldDescriptors.get(2), (float) 1.2) // float field
        .build();
    DynamicMessage message2 = DynamicMessage.newBuilder(desc)
        .setField(fieldDescriptors.get(0), enumValues.get(2))
        .setField(fieldDescriptors.get(1), 9) //int field
        .setField(fieldDescriptors.get(2), (float) 1.24) // float field
        .build();
    DynamicMessage message3 = DynamicMessage.newBuilder(desc)
        .setField(fieldDescriptors.get(0), enumValues.get(3))
        .setField(fieldDescriptors.get(1), 8) //intfield
        .setField(fieldDescriptors.get(2), (float) 1.29) // float field
        .build();
    List<DynamicMessage> dynamicMessages = new ArrayList<>();
    dynamicMessages.add(message1);
    dynamicMessages.add(message2);
    dynamicMessages.add(message3);

    ValidationUtil.validateDevFidelityParamsOrder(desc, dynamicMessages, errors);

    assertThat(errors.getWarningCount(ErrorType.DEV_FIDELITY_PARAMETERS_ORDER)).isEqualTo(0);
  }
}
