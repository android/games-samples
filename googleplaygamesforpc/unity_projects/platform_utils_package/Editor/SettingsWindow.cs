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

[ExecuteInEditMode]
public class SettingsWindow : EditorWindow
{
    [MenuItem("Tools/Asset Preprocessor Settings")]
    static void Init()
    {
        SettingsWindow window = (SettingsWindow)EditorWindow.GetWindow(typeof(SettingsWindow), true, "Platform Define and Asset Preprocessor Settings");
        window.Show();
    }

    private void OnGUI()
    {
        EditorGUILayout.Space();
        GUILayout.Label("Platform define settings", EditorStyles.boldLabel);
        
        
        GeneralSettings.IsSeparatePlatformDefineUsed = EditorGUILayout.Toggle("Use x86 Platform Define", GeneralSettings.IsSeparatePlatformDefineUsed);
        GeneralSettings.PlatformDefinePrefix = EditorGUILayout.TextField("Platform Define Prefix", GeneralSettings.PlatformDefinePrefix);

        EditorGUILayout.Separator();
        GUILayout.Label("Texture import settings", EditorStyles.boldLabel);
        
        GeneralSettings.IsOverriddenForAndroid = EditorGUILayout.Toggle("Override for Android", GeneralSettings.IsOverriddenForAndroid);
        GeneralSettings.ImportFormat = (GeneralSettings.ImportFormats) EditorGUILayout.EnumPopup("Texture Import Format", GeneralSettings.ImportFormat);
        EditorGUILayout.Space();

        EditorGUILayout.Separator();
        GUILayout.Label("Logging settings", EditorStyles.boldLabel);
        
        
        GeneralSettings.LoggingEnabled = EditorGUILayout.Toggle("Log Actions", GeneralSettings.LoggingEnabled);
        GeneralSettings.LoggingLevel = (GeneralSettings.LogLevels) EditorGUILayout.EnumPopup("Log Level", GeneralSettings.LoggingLevel);
    }
}

#endif
