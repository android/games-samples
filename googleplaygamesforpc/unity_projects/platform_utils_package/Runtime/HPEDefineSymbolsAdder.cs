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
using System.Collections.Generic;
using System.Linq;
using UnityEditor;

[InitializeOnLoad]
public class HpeDefineSymbolsAdder : Editor
{
    private static string _definePrefix = "UNITY_ANDROID_";
    // Add define symbols as soon as Unity gets done compiling.
    static HpeDefineSymbolsAdder ()
    {
        List<string> architectures = PlayerSettings.Android.targetArchitectures.ToString().Split ( ',' ).ToList();
        List<string> symbols = new List<string>();
        foreach (var architecture in architectures)
        {
            symbols.Add(_definePrefix + architecture.Trim());
        }

        string definesString = PlayerSettings.GetScriptingDefineSymbolsForGroup ( BuildTargetGroup.Android );
        List<string> allDefines = definesString.Split ( ';' ).ToList();
        
        allDefines.RemoveAll(x => x.Contains(_definePrefix));
        allDefines.AddRange ( symbols.Except ( allDefines ) );
        
        PlayerSettings.SetScriptingDefineSymbolsForGroup (
            EditorUserBuildSettings.selectedBuildTargetGroup,
            string.Join ( ";", allDefines.ToArray () ) );
    }

}
#endif