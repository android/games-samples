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

import com.google.common.io.Files;
import com.google.protobuf.ByteString;

import com.google.tuningfork.DevTuningfork.FidelityParams;
import com.google.tuningfork.DevTuningfork.QualitySettings;
import com.google.tuningfork.Tuningfork.Settings;
import com.google.tuningfork.Tuningfork.Settings.AggregationStrategy;
import com.google.tuningfork.Tuningfork.Settings.Histogram;
import java.io.File;
import java.util.Arrays;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)

public final class DeveloperTuningforkParserTest {

  @Rule
  // Override default behavior to allow overwriting files.
  public TemporaryFolder tempFolder =
      new TemporaryFolder() {
        @Override
        public File newFile(String filename) {
          return new File(getRoot(), filename);
        }
      };

  private final TestdataHelper helper = new TestdataHelper(tempFolder);
  private final ErrorCollector errors = new ParserErrorCollector();

  private DeveloperTuningforkParser parser;
  private final Settings settings =
      Settings.newBuilder()
          .setAggregationStrategy(
              AggregationStrategy.newBuilder()
                  .addAllAnnotationEnumSize(Arrays.asList(2))
                  .setMaxInstrumentationKeys(100))
          .addHistograms(Histogram.getDefaultInstance())
          .setApiKey("test-api-key")
          .build();

  private final FidelityParams devParameters1 =
      FidelityParams.newBuilder()
          .setIntField(10)
          .setFloatField(1.5f)
          .setQualitySettings(QualitySettings.FAST)
          .build();

  private final FidelityParams devParameters2 = FidelityParams.newBuilder().setIntField(10).build();
  private final FidelityParams devParameters3 = FidelityParams.getDefaultInstance();

  private static final File PROTOC_BINARY = ProtocBinary.get();

  @Before
  public void setUp() throws Exception {
    parser = new DeveloperTuningforkParser(errors, tempFolder.getRoot(), PROTOC_BINARY);
    helper.getFile("dev_tuningfork.proto");
    helper.createFile("tuningfork_settings.txt", settings.toString());
    helper.createFile("dev_tuningfork_fidelityparams_1.txt", devParameters1.toString());
    helper.createFile("dev_tuningfork_fidelityparams_2.txt", devParameters2.toString());
    helper.createFile("dev_tuningfork_fidelityparams_3.txt", devParameters3.toString());
    parser.parseFilesInFolder();
  }

  @Test
  public void checkNoErrors() throws Exception {
    parser.validate();
    assertThat(errors.getErrorCount()).isEqualTo(0);
  }

  @Test
  public void checkSettings() throws Exception {
    parser.validate();
    File settingsFile = new File(tempFolder.getRoot(), "tuningfork_settings.bin");
    ByteString settingsBinary = ByteString.copyFrom(Files.toByteArray(settingsFile));
    Settings parsed = Settings.parseFrom(settingsBinary);

    assertThat(parsed).isEqualTo(settings);
    assertThat(errors.hasSettingsErrors()).isFalse();
  }

  @Test
  public void checkFidelityParametersFiles() throws Exception {
    parser.validate();
    File file1 = new File(tempFolder.getRoot(), "dev_tuningfork_fidelityparams_1.bin");
    File file2 = new File(tempFolder.getRoot(), "dev_tuningfork_fidelityparams_2.bin");
    File file3 = new File(tempFolder.getRoot(), "dev_tuningfork_fidelityparams_3.bin");
    File file4 = new File(tempFolder.getRoot(), "dev_tuningfork_fidelityparams_4.bin");

    assertThat(file1.exists()).isTrue();
    assertThat(file2.exists()).isTrue();
    assertThat(file3.exists()).isTrue();
    assertThat(file4.exists()).isFalse();
  }

  @Test
  public void checkFidelityParameters() throws Exception {
    parser.validate();
    File devFidelityFile = new File(tempFolder.getRoot(), "dev_tuningfork_fidelityparams_1.bin");
    ByteString devFidelityBinary = ByteString.copyFrom(Files.toByteArray(devFidelityFile));
    FidelityParams parsed = FidelityParams.parseFrom(devFidelityBinary);

    assertThat(parsed).isEqualTo(devParameters1);
    assertThat(errors.hasFidelityParamsErrors()).isFalse();
  }
}
