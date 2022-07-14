/*
 * Copyright 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#if UNITY_EDITOR && UNITY_ANDROID

using UnityEditor;

public class GeneralSettings
{
    public static bool IsSeparatePlatformDefineUsed = true;
    public static string PlatformDefinePrefix = "UNITY_ANDROID_";
    public static bool IsOverriddenForAndroid = true;
    public static ImportFormats ImportFormat = ImportFormats.DXTC;
    public static bool LoggingEnabled = true;
    public static LogLevels LoggingLevel = LogLevels.Error;

    public enum ImportFormats
    {
        None = TextureImporterFormat.RGBA32,
        Low = TextureImporterFormat.DXT5,
        DXTC = TextureImporterFormat.DXT1,
        High = TextureImporterFormat.DXT1Crunched
    }

    public enum LogLevels
    {
        None,
        Infrom,
        Warning,
        Error
    }
}

#endif
