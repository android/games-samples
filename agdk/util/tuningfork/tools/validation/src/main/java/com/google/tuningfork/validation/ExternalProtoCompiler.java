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

import com.google.common.base.Preconditions;
import com.google.common.collect.ImmutableList;
import com.google.common.io.Files;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileDescriptorSet;
import com.google.protobuf.Descriptors.DescriptorValidationException;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.InvalidProtocolBufferException;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.Optional;

/** Compiles .proto file into a {@link FileDescriptor} */
public class ExternalProtoCompiler {

  private final String protoPath;
  private final FileDescriptor[] emptyDeps = new FileDescriptor[0];

  public ExternalProtoCompiler(File protoBinary) {
    Preconditions.checkNotNull(protoBinary, "executable");
    protoPath = protoBinary.getAbsolutePath();
  }

  public FileDescriptor compile(File file, Optional<File> outFile)
      throws IOException, CompilationException {
    Preconditions.checkNotNull(file, "file");
    FileDescriptor descriptor = buildAndRunCompilerProcess(file, outFile);
    return descriptor;
  }

  public byte[] runCommand(ProcessBuilder processBuilder) throws IOException, CompilationException {
    Process process = processBuilder.start();
    try {
      process.waitFor();
    } catch (InterruptedException e) {
      throw new CompilationException("Process was interrupted", e);
    }
    InputStream stdin = process.getInputStream();
    byte[] result = new byte[stdin.available()];
    stdin.read(result);
    return result;
  }

  private FileDescriptor buildAndRunCompilerProcess(File file, Optional<File> outFile)
      throws IOException, CompilationException {
    // Using temp file to have support for linux/mac/windows
    // dev/stdout is not working on windows
    File tempOutFile = File.createTempFile("temp", "txt");
    ImmutableList<String> commandLine = createCommandLine(file, tempOutFile);
    runCommand(new ProcessBuilder(commandLine));
    byte[] result = Files.toByteArray(tempOutFile);
    tempOutFile.delete();

    try {
      FileDescriptorSet fileSet = FileDescriptorSet.parseFrom(result);
      if (outFile.isPresent()) {
        Files.write(fileSet.toByteArray(), outFile.get());
      }
      for (FileDescriptorProto descProto : fileSet.getFileList()) {
        if (descProto.getName().equals(file.getName())) {
          return FileDescriptor.buildFrom(descProto, emptyDeps);
        }
      }
    } catch (DescriptorValidationException | InvalidProtocolBufferException e) {
      throw new IllegalStateException(e);
    }
    throw new CompilationException(
        String.format("Descriptor for [%s] does not exist.", file.getName()));
  }

  /* Decode textproto message from text(textprotoFile) into binary(binFile) */
  public File encodeFromTextprotoFile(
      String message,
      File protoFile,
      File textprotoFile,
      String binaryPath,
      Optional<File> errorFile)
      throws IOException, CompilationException {
    ImmutableList<String> command = encodeCommandLine(message, protoFile);

    File binFile = new File(binaryPath);

    ProcessBuilder builder =
        new ProcessBuilder(command).redirectInput(textprotoFile).redirectOutput(binFile);

    if (errorFile.isPresent()) {
      builder.redirectError(errorFile.get());
    }

    runCommand(builder);
    return binFile;
  }

  /* Decode textproto message from binary(binFile) into text(textFile) */
  public File decodeToTextprotoFile(
      String message, File protoFile, String textprotoPath, File binFile, Optional<File> errorFile)
      throws IOException, CompilationException {
    ImmutableList<String> command = decodeCommandLine(message, protoFile);

    File textprotoFile = new File(textprotoPath);

    ProcessBuilder builder =
        new ProcessBuilder(command).redirectInput(binFile).redirectOutput(textprotoFile);

    if (errorFile.isPresent()) {
      builder.redirectError(errorFile.get());
    }

    runCommand(builder);
    return textprotoFile;
  }

  private ImmutableList<String> createCommandLine(File file, File outFile) {
    return ImmutableList.of(
        protoPath,
        "-o",
        outFile.getAbsolutePath(),
        "-I",
        file.getName() + "=" + file.getAbsolutePath(), // That should be one line
        file.getAbsolutePath());
  }

  private ImmutableList<String> encodeCommandLine(String message, File protoFile) {
    return ImmutableList.of(
        protoPath,
        "--encode=" + message,
        "-I",
        protoFile.getName() + "=" + protoFile.getAbsolutePath(), // That should be one line
        protoFile.getAbsolutePath());
  }

  private ImmutableList<String> decodeCommandLine(String message, File protoFile) {
    return ImmutableList.of(
        protoPath,
        "--decode=" + message,
        "-I",
        protoFile.getName() + "=" + protoFile.getAbsolutePath(), // That should be one line
        protoFile.getAbsolutePath());
  }
}
