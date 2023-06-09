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
using Google.Android.AppBundle.Editor;
using Google.Android.AppBundle.Editor.AssetPacks;

public class BuildRuntimeAssets
{
    private const string streamingName = "StreamingAssets";
    private const string assetPacksName = "AssetPacks";
    private const string astcSuffix = "#tcf_astc";

    [MenuItem("Assets/Build PAD Demo Runtime Assets AssetPacks - Scripted")]
    static void BuildRTAssets_AssetPacks_Scripted()
    {
        // Save the current setting
        MobileTextureSubtarget originalSetting = EditorUserBuildSettings.androidBuildSubtarget;

        // Clean out any old data
        DeleteTargetDirectory(streamingName);
        DeleteTargetDirectory(assetPacksName);

        // Build the AssetBundles, both in ETC2 and ASTC texture formats
        BuildAssetBundles(assetPacksName, astcSuffix, MobileTextureSubtarget.ASTC);
        BuildAssetBundles(assetPacksName, "", MobileTextureSubtarget.ETC2);
        AssetDatabase.Refresh();

        // Copy our discrete test image asset into a new directory
        // which will be used for the 'discrete' asset pack source
        string discreteFileName = "Discrete1.jpg";
        string discretePackName = "discretepack";

        string discretePath = Path.Combine(Path.GetTempPath(), 
            discretePackName);
        Directory.CreateDirectory(discretePath);
        string destPath = Path.Combine(discretePath, discreteFileName);
        if (File.Exists(destPath))
        {
            File.Delete(destPath);
        }

        string sourcePath = Path.Combine(Application.dataPath,
            "Images");
        sourcePath = Path.Combine(sourcePath, discreteFileName);
        File.Copy(sourcePath, destPath);
        Debug.Log("Copied discrete file to : " + destPath);

        // Create an AssetPackConfig and start creating asset packs
        AssetPackConfig assetPackConfig = new AssetPackConfig();

        // Create asset packs using AssetBundles
        string assetBundlePath = Path.Combine(Application.dataPath,
            assetPacksName);

        // Add the default ETC2 bundles
        assetPackConfig.AddAssetBundle(Path.Combine(assetBundlePath,
            "installtime"), AssetPackDeliveryMode.InstallTime);

        assetPackConfig.AddAssetBundle(Path.Combine(assetBundlePath,
            "fastfollow"), AssetPackDeliveryMode.FastFollow);

        assetPackConfig.AddAssetBundle(Path.Combine(assetBundlePath,
            "ondemand"), AssetPackDeliveryMode.OnDemand);
        
        // Add the ASTC bundles
        assetBundlePath += astcSuffix;
        assetPackConfig.AddAssetBundle(Path.Combine(assetBundlePath,
            "installtime"), AssetPackDeliveryMode.InstallTime);

        assetPackConfig.AddAssetBundle(Path.Combine(assetBundlePath,
            "fastfollow"), AssetPackDeliveryMode.FastFollow);

        assetPackConfig.AddAssetBundle(Path.Combine(assetBundlePath,
            "ondemand"), AssetPackDeliveryMode.OnDemand);

        // Create an asset pack from our discrete directory
        assetPackConfig.AddAssetsFolder(discretePackName, discretePath,
            AssetPackDeliveryMode.OnDemand);

        // Configures the build system to use the newly created 
        // assetPackConfig when calling Google > Build and Run or 
        // Google > Build Android App Bundle.
        AssetPackConfigSerializer.SaveConfig(assetPackConfig);

        EditorUserBuildSettings.androidBuildSubtarget = originalSetting;
    }

    [MenuItem("Assets/Build PAD Demo Runtime Assets AssetPacks - Plugin")]
    static void BuildRTAssets_AssetPacks_Plugin()
    {
        // Save the current setting
        MobileTextureSubtarget originalSetting = EditorUserBuildSettings.androidBuildSubtarget;

        // Clean out any old data
        DeleteTargetDirectory(streamingName);
        DeleteTargetDirectory(assetPacksName);

        // Build the AssetBundles, both in ETC2 and ASTC texture formats
        BuildAssetBundles(assetPacksName, astcSuffix, MobileTextureSubtarget.ASTC);
        BuildAssetBundles(assetPacksName, "", MobileTextureSubtarget.ETC2);
        AssetDatabase.Refresh();

        EditorUserBuildSettings.androidBuildSubtarget = originalSetting;
    }

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
