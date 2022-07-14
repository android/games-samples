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
using UnityEngine;

public class AssetImporter : AssetPostprocessor
{
    void OnPreprocessTexture()
    {
        // Check if this is the first import. If not, skip.
        if (!assetImporter.importSettingsMissing)
        {
            return;
        }

        Log("First import of a texture, changing import settings.");

        TextureImporter textureImporter = assetImporter as TextureImporter;

        TextureImporterPlatformSettings platformSettings = new TextureImporterPlatformSettings
        {
            name = "Android",
            format = (TextureImporterFormat) GeneralSettings.ImportFormat,
            overridden = true
        };
        if (textureImporter != null) textureImporter.SetPlatformTextureSettings(platformSettings);
    }

    private void Log(string message)
    {
        if (GeneralSettings.LoggingEnabled)
        {
            Debug.Log("[ImportPreProcessor] " + message, assetImporter);
        }
    }
}

#endif
