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
    private const string PLAY_GAMES_SERVICES_MENU_NAME =
        "TrivialKart/Enable Google Play Games Services";

    private static BuildMenuItem _playGamesServicesItem;

    private static IList<BuildMenuItem> _buildItems;

    // InitializeOnLoad attribute means this is called on load
    static EditorMenuBuildOptions()
    {
        _playGamesServicesItem = new BuildMenuItem(PLAY_GAMES_SERVICES_MENU_NAME,
            EditorPrefs.GetBool(PLAY_GAMES_SERVICES_MENU_NAME, false), "PGS");

        _buildItems = new List<BuildMenuItem>()
        {
            _playGamesServicesItem,
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

    [MenuItem(PLAY_GAMES_SERVICES_MENU_NAME)]
    private static void TogglePlayGamesServicesAction()
    {
        // Toggling action
        PerformAction(_playGamesServicesItem);
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
