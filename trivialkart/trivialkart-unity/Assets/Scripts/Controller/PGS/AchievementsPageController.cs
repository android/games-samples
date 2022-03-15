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
using UnityEngine.UI;

public class AchievementsPageController : MonoBehaviour
{
    public GameObject achievementText;
    private PGSController _pgsController;

    void Awake()
    {
        _pgsController = FindObjectOfType<PGSController>();
    }

    // Refresh the page when it becomes active
    private void OnEnable()
    {
        RefreshPage();
    }

#if PLAY_GAMES_SERVICES
    private string GenerateAchievementString(PGSAchievementManager.TrivialKartAchievements id)
    {
        var achievementManager = _pgsController.AchievementManager;
        bool unlocked = achievementManager.GetAchievementUnlocked(id);
        string achievementName = achievementManager.GetAchievementName(id);
        string achievementString = achievementName + " " + (unlocked ? "unlocked\n" : "locked\n");
        return achievementString;
    }
#endif

    private void RefreshPage()
    {
#if PLAY_GAMES_SERVICES
        string achievementString = "Achievements\n";
        achievementString += GenerateAchievementString(
            PGSAchievementManager.TrivialKartAchievements.Tk_Achievement_Distance);
        achievementString += GenerateAchievementString(
            PGSAchievementManager.TrivialKartAchievements.Tk_Achievement_Truck);
        Text achievementText = this.achievementText.GetComponent<Text>();
        achievementText.text = achievementString;
#endif
    }
}
