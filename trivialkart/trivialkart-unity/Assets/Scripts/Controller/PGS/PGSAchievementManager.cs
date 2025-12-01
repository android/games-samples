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


#if PGS
using GooglePlayGames;
using GooglePlayGames.BasicApi;
using UnityEngine.SocialPlatforms;
#endif

public class PGSAchievementManager : MonoBehaviour
{
    public enum TrivialKartAchievements
    {
        // Achievement for driving more than a certain distance
        Tk_Achievement_Distance = 0,
        // Achievement running ot of gas
        Tk_Achievement_Gas = 1,
        // Achievement playing the game      
        Tk_Achievement_Play = 2
    }

    // Internal state object for achievement status and description
    private class AchievementInfo
    {
        public string achievementId;
        public string achievementName;
        public bool achievementUnlocked;

        public AchievementInfo(string achId)
        {
            achievementId = achId;
            achievementName = "";
            achievementUnlocked = false;
        }
    }

    private AchievementInfo[] _achievementInfo;

    void Awake()
    {
        // _achievementInfo = new AchievementInfo[]
        //     {
        //         new AchievementInfo(GPGSIds.achievement_tk_achievement_drive),
        //         new AchievementInfo(GPGSIds.achievement_tk_achievement_gas),
        //         new AchievementInfo(GPGSIds.achievement_tk_achievement_play)
        //     };
    }

    // Loads the achievements from Play Games Services, and updates
    // internal description and unlock status using the achievement data.
    public void LoadAchievements()
    {
        if (_achievementInfo == null)
            return;

        #if PGS
        PlayGamesPlatform.Instance.LoadAchievements((achievements) =>
        {
            foreach (var achievement in achievements)
            {
                foreach (var achievementInfo in _achievementInfo)
                {
                    if (achievement.id.Equals(achievementInfo.achievementId))
                    {
                        achievementInfo.achievementUnlocked = achievement.completed;
                    }
                }
            }
        });

        PlayGamesPlatform.Instance.LoadAchievementDescriptions(achievementDescriptions =>
        {
            foreach (var description in achievementDescriptions)
            {
                foreach (var achievementInfo in _achievementInfo)
                {
                    if (description.id.Equals(achievementInfo.achievementId))
                    {
                        achievementInfo.achievementName = description.unachievedDescription;
                    }
                }
            }
        });
        #endif
    }

    public bool GetAchievementUnlocked(TrivialKartAchievements achievementId)
    {
        if (_achievementInfo == null)
            return false;

        return _achievementInfo[(int)achievementId].achievementUnlocked;
    }

    public string GetAchievementName(TrivialKartAchievements achievementId)
    {
        if (_achievementInfo == null)
            return "";

        return _achievementInfo[(int)achievementId].achievementName;
    }

    public void UnlockAchievement(TrivialKartAchievements achievementId)
    {
        if(_achievementInfo == null)
            return;

        Social.ReportProgress(_achievementInfo[(int)achievementId].achievementId,
            100.0f, (bool success) =>
            {
                if (success)
                {
                    _achievementInfo[(int)achievementId].achievementUnlocked = true;
                }
                else
                {
                    Debug.Log("Unlock achievement failed for " +
                              _achievementInfo[(int)achievementId].achievementId);
                }
            });
    }

    public void IncrementAchievement(TrivialKartAchievements achievementId, int progress)
    {
        if(_achievementInfo == null)
            return;
            
        PlayGamesPlatform.Instance.IncrementAchievement(
            _achievementInfo[(int)achievementId].achievementId, progress, (bool success) => {
                    if (!success)
            {
                Debug.Log("Increment achievement failed for " +
                            _achievementInfo[(int)achievementId].achievementId);
            }
        });
    }

    public static void ShowPGSAchievements() => Social.ShowAchievementsUI();
}
