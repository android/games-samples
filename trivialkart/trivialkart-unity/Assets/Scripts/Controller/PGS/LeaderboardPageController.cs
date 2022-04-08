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

using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using UnityEngine.UI;

#if PLAY_GAMES_SERVICES
using GooglePlayGames;
using GooglePlayGames.BasicApi;
using UnityEngine.SocialPlatforms;
#endif

public class LeaderboardPageController : MonoBehaviour
{
    public GameObject leaderboardText;

    // Don't update the leaderboard more than once every five minutes,
    // unless we go to the background
    private const float MinimumLeaderboardReportInterval = 300.0f;

    // True if the distance game data has been initialized and is
    // valid for leaderboard reporting
    private bool _distanceInitialized = false;
    // The previously reported distance for the user
    private float _previousReportedDistance = 0f;
    // Delta time (in seconds) since the last leaderboard report
    private float _timeSinceLastLeaderboardUpdate = 0f;
    // String holding parsed results of a leaderboard data query
    private string _leaderboardString;

    // If distance is valid, has changed since our last report, and the minimum
    // time internal since the last report has passed, report the current
    // distance to the leaderboard.
    public void UpdateLeaderboard()
    {
        if (_distanceInitialized)
        {
            _timeSinceLastLeaderboardUpdate += Time.deltaTime;
            if (_previousReportedDistance < GameDataController.GetGameData().distanceTraveled)
            {
                if (_timeSinceLastLeaderboardUpdate > MinimumLeaderboardReportInterval)
                {
                    ReportDistanceLeaderboard();
                }
            }
        }
    }

    // Called after the save data for distance traveled has been loaded
    // or initialized, and a user is signed in.
    public void EnableLeaderboardReporting()
    {
        _distanceInitialized = true;
        _previousReportedDistance = GameDataController.GetGameData().distanceTraveled;
    }

    public void OnApplicationFocus(bool hasFocus)
    {
        // Immediately report our distance if we go to the background and it is out of
        // sync with the previously reported distance
        if (!hasFocus && _distanceInitialized)
        {
            if (_previousReportedDistance < GameDataController.GetGameData().distanceTraveled)
            {
                ReportDistanceLeaderboard();
            }
        }
    }

    // Report a distance traveled to the leaderboard
    private void ReportDistanceLeaderboard()
    {
#if PLAY_GAMES_SERVICES
        Debug.Log("ReportDistanceLeaderboard");
        // Reset the timer to make sure we don't immediately attempt reporting after
        // an error condition
        _timeSinceLastLeaderboardUpdate = 0f;
        long reportedDistance = (long)GameDataController.GetGameData().distanceTraveled;
        Social.ReportScore(reportedDistance, GPGSIds.leaderboard_tk_leaderboard_distance,
            (bool success) =>
        {
            if (success)
            {
                Debug.Log("Reported distance score of: " + reportedDistance);
                _previousReportedDistance = GameDataController.GetGameData().distanceTraveled;
            }
            else
            {
                Debug.Log("Failed to report leaderboard score");
            }
        });
#endif
    }

    // Refresh the page when it becomes active
    private void OnEnable()
    {
        RefreshPage();
    }

    private void RefreshPage()
    {
#if PLAY_GAMES_SERVICES
        if (_distanceInitialized)
        {
            LoadLeaderboardScores();
        }
#endif
    }

#if PLAY_GAMES_SERVICES
    // Loads 5 scores from the leaderboard, with our player's standing
    // centered in the list and formats them into a string for display
    // in the UI.
    private void LoadLeaderboardScores()
    {
        var lbText = leaderboardText.GetComponent<Text>();
        _leaderboardString = "Loading leaderboard";
        lbText.text = _leaderboardString;
        PlayGamesPlatform.Instance.LoadScores(
            GPGSIds.leaderboard_tk_leaderboard_distance,
            LeaderboardStart.PlayerCentered,
            5,
            LeaderboardCollection.Public,
            LeaderboardTimeSpan.AllTime,
            (data) =>
            {
                if (data.Valid)
                {
                    if (data.Scores.Length == 0)
                    {
                        lbText.text = "Empty leaderboard";
                    }
                    else
                    {
                        _leaderboardString = data.Title + "\n";
                        GenerateScoreStrings(lbText, data);
                    }
                }
                else
                {
                    Debug.Log("Invalid scores");
                }
            });
    }

    // Generate the standings strings, with a line for
    // each score, with placement, distance traveled, and user name (if available)
    private void GenerateScoreStrings(Text lbText, LeaderboardScoreData data)
    {
        IList<string> userIds = new List<string>();
        // Create list of userIds to use with LoadUsers to
        // try and retrieve user names
        foreach (IScore scores in data.Scores)
        {
            userIds.Add(scores.userID);
        }

        Social.LoadUsers(userIds.ToArray(), (users) =>
        {
            foreach(IScore score in data.Scores) {
                IUserProfile user = FindUser(users, score.userID);
                string userName = (user != null) ? user.userName : "unknown";
                string scoreString = String.Format("#{0} - {1} by {2}\n",
                    score.rank.ToString(), score.value.ToString(), userName);
                _leaderboardString += scoreString;
            }

            lbText.text = _leaderboardString;
        });
    }

    private static IUserProfile FindUser(IUserProfile[] users, string userID)
    {
        foreach (IUserProfile user in users)
        {
            if (user.id.Equals(userID))
            {
                return user;
            }
        }
        return null;
    }
#endif
}
