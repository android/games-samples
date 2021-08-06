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

import com.beust.jcommander.JCommander;
import com.beust.jcommander.Parameter;
import com.google.common.base.Strings;
import com.google.common.flogger.FluentLogger;
import java.io.File;
import java.io.IOException;

/** APK Validation tool for Tuningfork */
final class TuningforkApkValidationTool {

  private static class Parameters {
    @Parameter(
        names = {"--tuningforkPath"},
        description = "Path to an assets/tuningfork folder")
    public String tuningforkPath;

    @Parameter(
        names = {"--protoCompiler"},
        description = "Path to protoc binary")
    public String protoCompiler;

    @Parameter(
        names = {"--errorOnExit"},
        description = "Exit with error code if there is an error")
    public Boolean failOnError = false;
  }

  private static final FluentLogger logger = FluentLogger.forEnclosingClass();

  public static void main(String[] args) {
    Parameters parameters = new Parameters();
    new JCommander(parameters, args);

    checkArgument(
        !Strings.isNullOrEmpty(parameters.tuningforkPath),
        "You need to specify path to your tuningfork settings folder --tuningforkPath");

    checkArgument(
        !Strings.isNullOrEmpty(parameters.protoCompiler),
        "You need to specify path to proto compiler --protoCompiler");

    File tuningforkFolder = new File(parameters.tuningforkPath);
    if (!tuningforkFolder.exists()) {
      logger.atSevere().log(
          "Tuningfork settings folder does not exist %s", parameters.tuningforkPath);
    }

    if (!tuningforkFolder.isDirectory()) {
      logger.atSevere().log(
          "--tuningforkPath=[%s] is not a path to a folder", parameters.tuningforkPath);
    }

    File protoCompilerFile = new File(parameters.protoCompiler);
    if (!protoCompilerFile.exists()) {
      logger.atSevere().log("Proto compiler file does not exist %s", parameters.protoCompiler);
    }

    if (!protoCompilerFile.isFile() || !protoCompilerFile.canExecute()) {
      logger.atSevere().log(
          "--protoCompiler=[%s] is not a path to an executable file", parameters.protoCompiler);
    }

    logger.atInfo().log("Start validation of %s...", tuningforkFolder.getPath());

    ErrorCollector errors = new ParserErrorCollector();
    DeveloperTuningforkParser parser =
        new DeveloperTuningforkParser(errors, tuningforkFolder, protoCompilerFile);

    try {
      parser.parseFilesInFolder();
      parser.validate();
      for (String warningString: errors.getWarnings().values()) {
        logger.atWarning().log(warningString);
      }
      if (errors.getErrorCount() == 0) {
        logger.atInfo().log("Tuning Fork settings are valid");
      } else {
        logger.atWarning().log("Tuning Fork settings are invalid");
        errors.printStatus();
        if (parameters.failOnError) {
          System.exit(1);
        }
      }
    } catch (IOException | CompilationException e) {
      logger.atSevere().withCause(e).log("An error happened during validation");
      if (parameters.failOnError) {
        System.exit(2);
      }
    }
  }
}
