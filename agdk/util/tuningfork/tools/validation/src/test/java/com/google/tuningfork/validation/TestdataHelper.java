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

import static com.google.common.base.Preconditions.checkArgument;
import static java.nio.charset.StandardCharsets.UTF_8;

import com.google.common.io.ByteStreams;
import com.google.common.io.Files;
import com.google.protobuf.ByteString;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.UncheckedIOException;
import org.junit.rules.TemporaryFolder;

/** Base class for tests that need to work with testdata files */
public class TestdataHelper {

  private static final String FOLDER = "testdata/";

  public TemporaryFolder tempFolder;

  public TestdataHelper(TemporaryFolder tempFolder) {
    this.tempFolder = tempFolder;
  }

  public File createFile(String fileName, String content) throws IOException {
    File file = tempFolder.newFile(fileName);
    Files.asCharSink(file, UTF_8).write(content);
    return file;
  }

  public File getFile(String fileName) throws IOException {
    byte[] byteContent = readBytes(fileName);
    String content = new String(byteContent, UTF_8);

    File file = tempFolder.newFile(fileName);
    Files.asCharSink(file, UTF_8).write(content);
    return file;
  }

  public static InputStream openStream(String fileName) {
    InputStream is = TestdataHelper.class.getClassLoader().getResourceAsStream(FOLDER + fileName);
    checkArgument(is != null, "Testdata file '%s' not found.", fileName);
    return is;
  }

  public static byte[] readBytes(String fileName) {
    try (InputStream inputStream = openStream(fileName)) {
      return ByteStreams.toByteArray(inputStream);
    } catch (IOException e) {
      // Throw an unchecked exception to allow usage in lambda expressions.
      throw new UncheckedIOException(
          String.format("Failed to read contents of testdata file '%s'.", fileName), e);
    }
  }

  public static ByteString readByteString(String fileName) {
    return ByteString.copyFrom(readBytes(fileName));
  }
}
