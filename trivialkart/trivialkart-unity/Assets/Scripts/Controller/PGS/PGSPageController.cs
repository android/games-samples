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

using System.Collections;
using System.Collections.Generic;
using UnityEngine;

/// <summary>
/// Controller for the tab/page switch in the play games services UI.
/// It switches pages and tabs when different tabs are clicked;
/// </summary>
public class PGSPageController : MonoBehaviour
{
    public GameObject tab;
    public GameObject signinPage;
    public GameObject achievementPage;
    public GameObject leaderboardPage;
    public GameObject friendsPage;

    private const int UnselectedTabIndex = 0;
    private const int SelectedTabIndex = 1;
    private GameObject[] _tabs;
    private List<GameObject> _pgsPages;
    private int _currentTabIndex;

    private void Awake()
    {
        _currentTabIndex = 0;
        _pgsPages = new List<GameObject>()
            {signinPage, achievementPage, leaderboardPage, friendsPage};

        var tabsCount = tab.transform.childCount;
        _tabs = new GameObject[tabsCount];
        for (var tabIndex = 0; tabIndex < tabsCount; tabIndex++)
        {
            _tabs[tabIndex] = tab.transform.GetChild(tabIndex).gameObject;
        }
    }

    private void Update()
    {
        // Use tab key to switch between pages
        if (Input.GetKeyUp(KeyCode.Tab))
        {
            OnSwitchPageTabClicked((_currentTabIndex + 1) % _pgsPages.Count);
        }
    }

    private void OnEnable()
    {
        RefreshPage();
    }

    private void RefreshPage()
    {
        OnSwitchPageTabClicked(_currentTabIndex);
    }

    public void OnSwitchPageTabClicked(int tabIndex)
    {
        _currentTabIndex = tabIndex;
        SetPage(_pgsPages[tabIndex]);
        SetTab(tabIndex);
    }

    private void SetPage(GameObject targetPage)
    {
        foreach (var page in _pgsPages)
        {
            page.SetActive(page.Equals(targetPage));
        }
    }

    private void SetTab(int targetTabIndex)
    {
        for (var tabIndex = 0; tabIndex < _tabs.Length; tabIndex++)
        {
            var isTabSelected = tabIndex == targetTabIndex;
            _tabs[tabIndex].transform.GetChild(UnselectedTabIndex)
                .gameObject.SetActive(!isTabSelected);
            _tabs[tabIndex].transform.GetChild(SelectedTabIndex)
                .gameObject.SetActive(isTabSelected);
        }
    }}
