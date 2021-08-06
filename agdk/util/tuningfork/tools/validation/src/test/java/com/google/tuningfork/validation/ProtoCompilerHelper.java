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

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import java.io.File;
import java.util.Optional;
import org.junit.rules.TemporaryFolder;

/** Base class for tests that need to create proto Descriptors */
public class ProtoCompilerHelper {

  private static final File PROTOC_BINARY = ProtocBinary.get();
  private static final ExternalProtoCompiler compiler = new ExternalProtoCompiler(PROTOC_BINARY);
  private final TestdataHelper testdata;

  public ProtoCompilerHelper(TemporaryFolder tempFolder) {
    this.testdata = new TestdataHelper(tempFolder);
  }

  public Descriptor getDescriptor(String fileName, String message, String descName)
      throws Exception {
    File file = testdata.getFile(fileName);
    FileDescriptor fDesc = compiler.compile(file, Optional.empty());
    assertThat(fDesc).isNotNull();
    Descriptor desc = fDesc.findMessageTypeByName(descName);
    return desc;
  }
}
