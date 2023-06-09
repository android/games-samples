// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

using System.IO;
using System.Collections;
using System.Collections.Generic;
using UnityEditor;
using UnityEngine;

public class BuildRuntimeAssets
{
    private const string streamingName = "StreamingAssets";
    private const string assetPacksName = "AssetPacks";
    private const string astcSuffix = "#tcf_astc";

    [MenuItem("Assets/Build PAD Demo Runtime Assets Streaming")]
    static void BuildRTAssets_Streaming()
    {
        // Clean out any old data
        DeleteTargetDirectory(streamingName);
        // Build the asset bundles
        BuildAssetBundles(streamingName, "", MobileTextureSubtarget.Generic);
        // Copy our discrete test image asset
        CopyDiscreteAsset("Discrete1.jpg", "Images", streamingName);
        AssetDatabase.Refresh();
    }

    static void BuildAssetBundles(string targetDirectory, string directorySuffix, 
        MobileTextureSubtarget compressionFormat)
    {
        EnsureTargetDirectoryExists(targetDirectory);
        EditorUserBuildSettings.androidBuildSubtarget = compressionFormat;
        if (!EditorUserBuildSettings.SwitchActiveBuildTarget(BuildTargetGroup.Android, 
            BuildTarget.Android))
        {
            Debug.Log("Failed to switch build target");
        }

        // Build the AssetBundles
        string targetPath = Path.Combine(Application.dataPath, targetDirectory);
        BuildPipeline.BuildAssetBundles(targetPath,
            BuildAssetBundleOptions.UncompressedAssetBundle, 
            BuildTarget.Android);

        // We don't want to include a format suffix in the actual asset
        // bundle name, so we build without the suffix and then
        // rename the output directory to have the suffix
        if (!string.IsNullOrEmpty(directorySuffix))
        {
            string renamedBundlesDirectory = targetDirectory + directorySuffix;
            DeleteTargetDirectory(renamedBundlesDirectory);
            string renamedPath = Path.Combine(Application.dataPath, renamedBundlesDirectory);
            Directory.Move(targetPath, renamedPath);
        }
    }

    static void DeleteTargetDirectory(string targetDirectory)
    {
        if (!string.IsNullOrEmpty(targetDirectory))
        {
            string targetPath = Path.Combine(Application.dataPath, targetDirectory);
            if (Directory.Exists(targetPath))
            {
                string assetsPath = "Assets" + Path.DirectorySeparatorChar + 
                    targetDirectory;
                if (!AssetDatabase.DeleteAsset(assetsPath))
                {
                    Debug.Log("Failed to delete: " + assetsPath);
                }
            }
        }
    }

    static void EnsureTargetDirectoryExists(string targetDirectory)
    {
        string targetPath = Path.Combine(Application.dataPath, targetDirectory);
        if (Directory.Exists(targetPath) == false)
        {
            AssetDatabase.CreateFolder("Assets", targetDirectory);
        }
    }

    static void CopyDiscreteAsset(string assetName, string assetPath,
        string targetDirectory)
    {
        string sourceDirectory = Path.Combine("Assets", assetPath);
        string destinationDirectory = Path.Combine("Assets", targetDirectory);
        string sourcePath = Path.Combine(sourceDirectory, assetName);
        string destinationPath = Path.Combine(destinationDirectory, assetName);

        if (AssetDatabase.CopyAsset(sourcePath, destinationPath) == false)
        {
            Debug.Log("Failed to copy: " + assetName);
        }

    }
}
