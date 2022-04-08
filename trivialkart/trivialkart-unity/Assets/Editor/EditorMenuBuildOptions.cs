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
using UnityEngine;
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

    private static BuildMenuItem _iapItem;
    private static BuildMenuItem _playGamesPCItem;
    private static BuildMenuItem _playGamesServicesItem;
    private static BuildMenuItem _playIntegrityItem;

    private static IList<BuildMenuItem> _buildItems;

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

        _buildItems = new List<BuildMenuItem>()
        {
            _iapItem,
            _playGamesPCItem,
            _playGamesServicesItem,
            _playIntegrityItem
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
        PerformAction(_playGamesServicesItem);
    }

    [MenuItem(PLAY_INTEGRITY_MENU_NAME)]
    private static void TogglePlayIntegrityAction()
    {
        // Toggling action
        PerformAction(_playIntegrityItem);
    }

    private static void PerformAction(BuildMenuItem buildItem)
    {
        buildItem.IsEnabled = !buildItem.IsEnabled;
        UpdateMenu(buildItem);
        SetBuildDirectives();
    }

    private static void SetBuildDirectives()
    {
        int defineCount = 0;
        string defineString = "";
        foreach (BuildMenuItem buildItem in _buildItems)
        {
            if (!string.IsNullOrEmpty(buildItem.GetDirective))
            {
                if (defineCount > 0)
                {
                    defineString += ";";
                }
                Debug.Log(buildItem.GetDirective);
                defineString += buildItem.GetDirective;
                ++defineCount;
            }
        }
        PlayerSettings.SetScriptingDefineSymbolsForGroup(BuildTargetGroup.Android,
            defineString);
    }
}
