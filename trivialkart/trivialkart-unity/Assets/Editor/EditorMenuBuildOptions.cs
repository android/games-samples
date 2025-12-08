// Copyright 2022 Google LLC
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

using UnityEditor;
using System.Collections.Generic;
using System.Linq;

[InitializeOnLoad]
public static class EditorMenuBuildOptions
{
    private const string IAP_MENU_NAME = "TrivialKart/BuildOptions/Build with IAP";
    private const string PLAY_GAMES_MENU_NAME =
        "TrivialKart/BuildOptions/Build for Google Play Games PC";
    private const string PLAY_GAMES_SERVICES_MENU_NAME =
        "TrivialKart/BuildOptions/Build with Google Play Games Services";
    private const string PLAY_INTEGRITY_MENU_NAME =
        "TrivialKart/BuildOptions/Build with Play Integrity";
    private const string RECALL_API_MENU_NAME =
        "TrivialKart/PlayGameServices/Recall API";
    private const string AGE_GATE_MENU_NAME = 
        "TrivialKart/PlayGameServices/Age Gate Example";

    private static BuildMenuItem _iapItem;
    private static BuildMenuItem _playGamesPCItem;
    private static BuildMenuItem _playGamesServicesItem;
    private static BuildMenuItem _playIntegrityItem;
    private static BuildMenuItem _recallAPI;
    private static BuildMenuItem _ageGate;

    private static IList<BuildMenuItem> _buildItems;
    
    private static readonly HashSet<string> AllManagedDirectives = new HashSet<string>()
    {
        "USE_IAP", "NO_IAP",
        "PLAY_GAMES_PC",
        "PLAY_GAMES_SERVICES",
        "PLAY_INTEGRITY",
        "RECALL_API",
        "AGE_GATE"
    };

    // InitializeOnLoad attribute means this is called on load
    static EditorMenuBuildOptions()
    {
        _iapItem = new BuildMenuItem(IAP_MENU_NAME,
            EditorPrefs.GetBool(IAP_MENU_NAME, false), "USE_IAP", "NO_IAP");
        _playGamesPCItem = new BuildMenuItem(PLAY_GAMES_MENU_NAME,
            EditorPrefs.GetBool(PLAY_GAMES_MENU_NAME, false), "PLAY_GAMES_PC");
        _playGamesServicesItem = new BuildMenuItem(PLAY_GAMES_SERVICES_MENU_NAME,
            EditorPrefs.GetBool(PLAY_GAMES_SERVICES_MENU_NAME, false), "PLAY_GAMES_SERVICES");
        _playIntegrityItem = new BuildMenuItem(PLAY_INTEGRITY_MENU_NAME,
            EditorPrefs.GetBool(PLAY_INTEGRITY_MENU_NAME, false), "PLAY_INTEGRITY");
        _recallAPI = new BuildMenuItem(RECALL_API_MENU_NAME,
            EditorPrefs.GetBool(RECALL_API_MENU_NAME, false), "RECALL_API");
        _ageGate = new BuildMenuItem(AGE_GATE_MENU_NAME,
            EditorPrefs.GetBool(AGE_GATE_MENU_NAME, false), "AGE_GATE");

        _buildItems = new List<BuildMenuItem>()
        {
            _iapItem,
            _playGamesPCItem,
            _playGamesServicesItem,
            _playIntegrityItem,
            _recallAPI,
            _ageGate
        };

        // Delaying until first editor tick so that the menu
        // will be populated before setting check state, and
        // re-apply correct action
        EditorApplication.delayCall += () =>
        {
            foreach (BuildMenuItem buildItem in _buildItems)
            {
                UpdateMenu(buildItem);
            }

            SetBuildDirectives();
        };
    }

    private static void UpdateMenu(BuildMenuItem buildItem)
    {
        // Set checkmark on menu item
        Menu.SetChecked(buildItem.ItemName, buildItem.IsEnabled);
        // Saving editor state
        EditorPrefs.SetBool(buildItem.ItemName, buildItem.IsEnabled);
    }

    [MenuItem(IAP_MENU_NAME)]
    private static void ToggleIAPAction()
    {
        // Toggling action
        PerformAction(_iapItem);
    }

    [MenuItem(PLAY_GAMES_MENU_NAME)]
    private static void TogglePlayGamesPCAction()
    {
        // Toggling action
        PerformAction(_playGamesPCItem);
    }

    [MenuItem(PLAY_GAMES_SERVICES_MENU_NAME)]
    private static void TogglePlayGamesServicesAction()
    {
        // Toggling action
        _playGamesServicesItem.IsEnabled = !_playGamesServicesItem.IsEnabled;
        UpdateMenu(_playGamesServicesItem);

        if (!_playGamesServicesItem.IsEnabled && _recallAPI.IsEnabled)
        {
            _recallAPI.IsEnabled = false;
            UpdateMenu(_recallAPI);
        }

        SetBuildDirectives();
    }

    [MenuItem(PLAY_INTEGRITY_MENU_NAME)]
    private static void TogglePlayIntegrityAction()
    {
        // Toggling action
        PerformAction(_playIntegrityItem);
    }

    [MenuItem(RECALL_API_MENU_NAME)]
    private static void ToggleRecallAPIAction()
    {
        if (_playGamesServicesItem.IsEnabled)
        {
            _recallAPI.IsEnabled = !_recallAPI.IsEnabled;
            UpdateMenu(_recallAPI);
            SetBuildDirectives();
        }
    }
    
    [MenuItem(AGE_GATE_MENU_NAME)]
    private static void ToggleAgeGateAction()
    {
        if (_playGamesServicesItem.IsEnabled)
        {
            _ageGate.IsEnabled = !_ageGate.IsEnabled;
            UpdateMenu(_ageGate);
            SetBuildDirectives();
        }
    }

    [MenuItem(RECALL_API_MENU_NAME, true)]
    private static bool ValidateRecallAPIAction()
    {
        return isPlayGameServicesEnabled();
    }
    
    [MenuItem(AGE_GATE_MENU_NAME, true)]
    private static bool ValidateAgeGateAction()
    {
        return isPlayGameServicesEnabled();
    }

    private static bool isPlayGameServicesEnabled()
    {
        return _playGamesServicesItem == null ? EditorPrefs.GetBool(PLAY_GAMES_SERVICES_MENU_NAME, false) : _playGamesServicesItem.IsEnabled;
    }

    private static void PerformAction(BuildMenuItem buildItem)
    {
        buildItem.IsEnabled = !buildItem.IsEnabled;
        UpdateMenu(buildItem);
        SetBuildDirectives();
    }

    private static void SetBuildDirectives()
    {
        var activeBuildTargetGroup = EditorUserBuildSettings.selectedBuildTargetGroup;

#if UNITY_2021_3_OR_NEWER
        var activeBuildTarget = UnityEditor.Build.NamedBuildTarget.FromBuildTargetGroup(activeBuildTargetGroup);
        var currentDefinesArray = PlayerSettings.GetScriptingDefineSymbols(activeBuildTarget).Split(';');
#else
        currentDefinesArray = PlayerSettings.GetScriptingDefineSymbolsForGroup(activeBuildTargetGroup).Split(';');
#endif
        
        var defines = new HashSet<string>(
            currentDefinesArray
                .SelectMany(s => s.Split(';'))
                .Where(s => !string.IsNullOrEmpty(s))
        );

        defines.ExceptWith(AllManagedDirectives);

        foreach (BuildMenuItem buildItem in _buildItems)
        {
            if (buildItem == _recallAPI && !_playGamesServicesItem.IsEnabled)
            {
                continue;
            }
        
            string activeDirective = buildItem.GetDirective;
            if (!string.IsNullOrEmpty(activeDirective))
            {
                defines.Add(activeDirective);
            }
        }

        var newDefines = defines.ToArray();
    
#if UNITY_2021_3_OR_NEWER
        PlayerSettings.SetScriptingDefineSymbols(activeBuildTarget, newDefines);
#else
        PlayerSettings.SetScriptingDefineSymbolsForGroup(activeBuildTargetGroup, string.Join(";", newDefines));
#endif
    }
}