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

using UnityEngine;

/// <summary>
/// Controller for background garage page.
/// It listens to the background switch button click events;
/// It controls availability and selected status of background garage items.
/// </summary>
public class BackgroundGaragePageController : MonoBehaviour
{
    private void Update()
    {
        // Use the keyboard arrows to select the background
        if (Input.GetKeyUp(KeyCode.LeftArrow))
        {
            SwitchToPreviousBackground();
        }
        if (Input.GetKeyUp(KeyCode.RightArrow))
        {
            SwitchToNextBackground();
        }
    }

    private void OnEnable()
    {
        RefreshPage();
    }

    public void OnBackgroundGarageItemClicked(int backgroundIndex)
    {
        SwitchToTargetBackground(BackgroundList.GetBackgroundByIndex(backgroundIndex));
    }

    private void SwitchToTargetBackground(BackgroundList.Background targetBackground)
    {
        GameDataController.GetGameData().UpdateBackgroundInUse(targetBackground);
        SetBackgroundInUseStatus();
    }

    private void RefreshPage()
    {
        SetBackgroundAvailability();
        SetBackgroundInUseStatus();
    }

    // Check if player owns the background, and set availabiliy accordingly
    private void SetBackgroundAvailability()
    {
        foreach (var background in BackgroundList.List)
        {
            var isBackgroundOwned = GameDataController.GetGameData().CheckBackgroundOwnership(background);
            background.GarageItemGameObj.SetActive(isBackgroundOwned);
        }
    }

    private void SetBackgroundInUseStatus()
    {
        foreach (var background in BackgroundList.List)
        {
            background.GarageItemGameObj.transform.Find("statusText").gameObject.SetActive(false);
        }

        GameDataController.GetGameData().BackgroundInUseObj.GarageItemGameObj.transform.Find("statusText").gameObject
            .SetActive(true);
    }

    // Select the next unlocked background from the current index
    private void SwitchToNextBackground()
    {
        // Current background index
        int backgroundIndex = BackgroundList
            .List.IndexOf(GameDataController.GetGameData().BackgroundInUseObj);
        int totalBackgrounds = BackgroundList.List.Count;

        // Search for the next unlocked background to the right
        do
        {
            backgroundIndex = (backgroundIndex + 1) % totalBackgrounds;
        } while (!GameDataController.GetGameData()
            .CheckBackgroundOwnership(BackgroundList.GetBackgroundByIndex(backgroundIndex)));
        SwitchToTargetBackground(BackgroundList.GetBackgroundByIndex(backgroundIndex));
    }

    // Select the previous unlocked background from the current index
    private void SwitchToPreviousBackground()
    {
        // Current background index
        int backgroundIndex = BackgroundList
            .List.IndexOf(GameDataController.GetGameData().BackgroundInUseObj);
        int totalBackgrounds = BackgroundList.List.Count;

        // Search for the next unlocked background to the left
        do
        {
            backgroundIndex = (backgroundIndex - 1 + totalBackgrounds) % totalBackgrounds;
        } while (!GameDataController.GetGameData()
            .CheckBackgroundOwnership(BackgroundList.GetBackgroundByIndex(backgroundIndex)));
        SwitchToTargetBackground(BackgroundList.GetBackgroundByIndex(backgroundIndex));
    }
}