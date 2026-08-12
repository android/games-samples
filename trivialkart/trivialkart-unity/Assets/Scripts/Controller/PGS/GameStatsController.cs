// Copyright 2026 Google LLC
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
using System.Globalization;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

#if PLAY_GAMES_SERVICES
using GooglePlayGames;
using GooglePlayGames.BasicApi;
#endif

/// <summary>
/// Controller for managing Google Play Games Services Game Stats.
/// Reads inputs from the GameStats scene UI and records PlayerGameEvent
/// instances on button presses.
/// </summary>
public class GameStatsController : MonoBehaviour
{
    [Header("Race Completed Event UI")]
    [SerializeField] private TMP_Dropdown carUsedDropdown;
    [SerializeField] private TMP_Dropdown rankDropdown;
    [SerializeField] private TMP_Dropdown usedNOSDropdown;
    [SerializeField] private TMP_Dropdown raceTimeDropdown;
    [SerializeField] private Button raceCompletedButton;

    [Header("Car Upgraded Event UI")]
    [SerializeField] private TMP_Dropdown carCharacteristicDropdown;
    [SerializeField] private TMP_Dropdown oldValueDropdown;
    [SerializeField] private TMP_Dropdown newValueDropdown;
    [SerializeField] private TMP_Dropdown garageLevelDropdown;
    [SerializeField] private Button carUpgradedButton;

    private void Awake()
    {
        BindUIElementsIfNull();
    }

    private void Start()
    {
        InitializePlayGamesPlatform();
        RegisterButtonListeners();
    }

    /// <summary>
    /// Finds and binds UI elements automatically if they were not assigned in the inspector.
    /// </summary>
    private void BindUIElementsIfNull()
    {
        if (carUsedDropdown == null)
            carUsedDropdown = FindDropdownByName("carUsedDropdown");

        if (rankDropdown == null)
            rankDropdown = FindDropdownByName("rankDropdown");

        if (usedNOSDropdown == null)
            usedNOSDropdown = FindDropdownByName("usedNOSDropdown");

        if (raceTimeDropdown == null)
            raceTimeDropdown = FindDropdownByName("raceTimeDropdown");

        if (raceCompletedButton == null)
            raceCompletedButton = FindButtonByName("raceCompletedButton");

        if (carCharacteristicDropdown == null)
            carCharacteristicDropdown = FindDropdownByName("carCharacteristicDropdown");

        if (oldValueDropdown == null)
            oldValueDropdown = FindDropdownByName("oldValueDropdown");

        if (newValueDropdown == null)
            newValueDropdown = FindDropdownByName("newValueDropdown");

        if (garageLevelDropdown == null)
            garageLevelDropdown = FindDropdownByName("garageLevelDropdown");

        if (carUpgradedButton == null)
            carUpgradedButton = FindButtonByName("carUpgradedButton");
    }

    private TMP_Dropdown FindDropdownByName(string name)
    {
        Transform child = transform.Find(name);
        if (child != null && child.TryGetComponent<TMP_Dropdown>(out var dropdown))
            return dropdown;

        GameObject go = GameObject.Find(name);
        return go != null ? go.GetComponent<TMP_Dropdown>() : null;
    }

    private Button FindButtonByName(string name)
    {
        Transform child = transform.Find(name);
        if (child != null && child.TryGetComponent<Button>(out var button))
            return button;

        GameObject go = GameObject.Find(name);
        return go != null ? go.GetComponent<Button>() : null;
    }

    /// <summary>
    /// Registers click listeners on the UI buttons.
    /// </summary>
    private void RegisterButtonListeners()
    {
        if (raceCompletedButton != null)
        {
            raceCompletedButton.onClick.RemoveListener(OnRaceCompletedButtonClicked);
            raceCompletedButton.onClick.AddListener(OnRaceCompletedButtonClicked);
        }
        else
        {
            Debug.LogWarning("[GameStatsController] raceCompletedButton is not assigned!");
        }

        if (carUpgradedButton != null)
        {
            carUpgradedButton.onClick.RemoveListener(OnCarUpgradedButtonClicked);
            carUpgradedButton.onClick.AddListener(OnCarUpgradedButtonClicked);
        }
        else
        {
            Debug.LogWarning("[GameStatsController] carUpgradedButton is not assigned!");
        }
    }

    /// <summary>
    /// Initializes Google Play Games Platform and handles authentication if needed.
    /// </summary>
    private void InitializePlayGamesPlatform()
    {
#if PLAY_GAMES_SERVICES
        PlayGamesPlatform.DebugLogEnabled = true;
        PlayGamesPlatform.Activate();

        if (!PlayGamesPlatform.Instance.IsAuthenticated())
        {
            PlayGamesPlatform.Instance.Authenticate(status =>
            {
                Debug.Log($"[GameStatsController] PGS Authenticate status: {status}");
            });
        }
        else
        {
            Debug.Log("[GameStatsController] PGS already authenticated.");
        }
#endif
    }

    #region Event Handlers

    /// <summary>
    /// Called when the 'raceCompleted' button is clicked.
    /// Collects values from carUsed, rank, usedNOS, raceTime dropdowns and records the event.
    /// </summary>
    public void OnRaceCompletedButtonClicked()
    {
        string carUsed = GetDropdownText(carUsedDropdown);
        string rankStr = GetDropdownText(rankDropdown);
        string usedNOSStr = GetDropdownText(usedNOSDropdown);
        string raceTimeStr = GetDropdownText(raceTimeDropdown);

        long rank = ParseLong(rankStr, 1);
        bool usedNOS = usedNOSStr.Equals("Yes", StringComparison.OrdinalIgnoreCase);
        double raceTime = ParseTimeToSeconds(raceTimeStr);

        SendRaceCompletedEvent(carUsed, rank, usedNOS, raceTime);
    }

    /// <summary>
    /// Constructs and records the 'raceCompleted' PlayerGameEvent.
    /// </summary>
    public void SendRaceCompletedEvent(string carUsed, long rank, bool usedNOS, double raceTime)
    {
        Debug.Log($"[GameStatsController] Sending 'raceCompleted' event: carUsed='{carUsed}', rank={rank}, usedNOS={usedNOS}, raceTime={raceTime}s");

#if PLAY_GAMES_SERVICES
        try
        {
            // Note: Filter for stat #3 ("Races finished with Mustang") is carUsed = "mustang" (lowercase).
            string carUsedValue = !string.IsNullOrEmpty(carUsed) ? carUsed.ToLowerInvariant() : "mustang";

            PlayerGameEvent raceCompletedEvent = new PlayerGameEvent.Builder("raceCompleted")
                .AddProperty("carUsed", carUsedValue)
                .AddProperty("rank", rank)
                .AddProperty("usedNOS", usedNOS)
                .AddProperty("raceTime", raceTime)
                .Build();

            PlayGamesPlatform.Instance.RecordEvent(raceCompletedEvent);
            Debug.Log("[GameStatsController] Successfully recorded 'raceCompleted' PlayerGameEvent.");
        }
        catch (Exception ex)
        {
            Debug.LogError($"[GameStatsController] Failed to record 'raceCompleted' event: {ex}");
        }
#else
        Debug.Log("[GameStatsController] (PLAY_GAMES_SERVICES not enabled) 'raceCompleted' event simulated.");
#endif
    }

    /// <summary>
    /// Called when the 'carUpgraded' button is clicked.
    /// Collects values from carCharacteristic, oldValue, newValue, garageLevel dropdowns and records the event.
    /// </summary>
    public void OnCarUpgradedButtonClicked()
    {
        string carCharacteristic = GetDropdownText(carCharacteristicDropdown);
        string oldValueStr = GetDropdownText(oldValueDropdown);
        string newValueStr = GetDropdownText(newValueDropdown);
        string garageLevelStr = GetDropdownText(garageLevelDropdown);

        long oldValue = ParseLong(oldValueStr, 1);
        long newValue = ParseLong(newValueStr, 2);
        long garageLevel = ParseLong(garageLevelStr, 1);

        SendCarUpgradedEvent(carCharacteristic, oldValue, newValue, garageLevel);
    }

    /// <summary>
    /// Constructs and records the 'carUpgraded' PlayerGameEvent.
    /// </summary>
    public void SendCarUpgradedEvent(string carCharacteristic, long oldValue, long newValue, long garageLevel)
    {
        Debug.Log($"[GameStatsController] Sending 'carUpgraded' event: carCharacteristic='{carCharacteristic}', oldValue={oldValue}, newValue={newValue}, garageLevel={garageLevel}");

#if PLAY_GAMES_SERVICES
        try
        {
            PlayerGameEvent carUpgradedEvent = new PlayerGameEvent.Builder("carUpgraded")
                .AddProperty("carCharacteristic", carCharacteristic)
                .AddProperty("oldValue", oldValue)
                .AddProperty("newValue", newValue)
                .AddProperty("garageLevel", garageLevel)
                .Build();

            PlayGamesPlatform.Instance.RecordEvent(carUpgradedEvent);
            Debug.Log("[GameStatsController] Successfully recorded 'carUpgraded' PlayerGameEvent.");
        }
        catch (Exception ex)
        {
            Debug.LogError($"[GameStatsController] Failed to record 'carUpgraded' event: {ex}");
        }
#else
        Debug.Log("[GameStatsController] (PLAY_GAMES_SERVICES not enabled) 'carUpgraded' event simulated.");
#endif
    }

    /// <summary>
    /// Records a progression stat event (such as Level / XP progression).
    /// </summary>
    /// <param name="eventName">Event name.</param>
    /// <param name="propertyName">Property name, e.g. 'currentProgress'.</param>
    /// <param name="progressValue">Numeric progression value.</param>
    public void SendProgressionStatEvent(string eventName, string propertyName, long progressValue)
    {
        Debug.Log($"[GameStatsController] Sending progression stat event '{eventName}': {propertyName}={progressValue}");

#if PLAY_GAMES_SERVICES
        try
        {
            PlayerGameEvent progressionEvent = new PlayerGameEvent.Builder(eventName)
                .AddProperty(propertyName, progressValue)
                .Build();

            PlayGamesPlatform.Instance.RecordEvent(progressionEvent);
            Debug.Log($"[GameStatsController] Successfully recorded progression event '{eventName}'.");
        }
        catch (Exception ex)
        {
            Debug.LogError($"[GameStatsController] Failed to record progression event '{eventName}': {ex}");
        }
#else
        Debug.Log("[GameStatsController] (PLAY_GAMES_SERVICES not enabled) Progression stat event '{eventName}' simulated.");
#endif
    }

    /// <summary>
    /// Flushes and requests an immediate upload of any pending player game events to Google Play Games Services.
    /// </summary>
    public void RequestEventsUpload()
    {
#if PLAY_GAMES_SERVICES
        try
        {
            PlayGamesPlatform.Instance.RequestEventsUpload();
            Debug.Log("[GameStatsController] Requested events upload to Play Games Services.");
        }
        catch (Exception ex)
        {
            Debug.LogError($"[GameStatsController] Failed to request events upload: {ex}");
        }
#else
        Debug.Log("[GameStatsController] (PLAY_GAMES_SERVICES not enabled) RequestEventsUpload simulated.");
#endif
    }

    #endregion

    #region Helper Methods

    private static string GetDropdownText(TMP_Dropdown dropdown)
    {
        if (dropdown == null || dropdown.options == null || dropdown.options.Count == 0)
            return string.Empty;

        int index = Mathf.Clamp(dropdown.value, 0, dropdown.options.Count - 1);
        return dropdown.options[index].text;
    }

    private static long ParseLong(string text, long defaultValue)
    {
        if (string.IsNullOrWhiteSpace(text))
            return defaultValue;

        if (long.TryParse(text.Trim(), NumberStyles.Integer, CultureInfo.InvariantCulture, out long result))
            return result;

        return defaultValue;
    }

    /// <summary>
    /// Parses formatted time string (e.g. "0:02:30" or "0:01:02" or "150") to total seconds.
    /// </summary>
    private static double ParseTimeToSeconds(string timeStr)
    {
        if (string.IsNullOrWhiteSpace(timeStr))
            return 0.0;

        timeStr = timeStr.Trim();

        if (TimeSpan.TryParse(timeStr, CultureInfo.InvariantCulture, out TimeSpan ts))
        {
            return ts.TotalSeconds;
        }

        if (double.TryParse(timeStr, NumberStyles.Float, CultureInfo.InvariantCulture, out double seconds))
        {
            return seconds;
        }

        // Fallback manual parse for mm:ss or hh:mm:ss format
        string[] parts = timeStr.Split(':');
        if (parts.Length == 3)
        {
            if (double.TryParse(parts[0], out double h) &&
                double.TryParse(parts[1], out double m) &&
                double.TryParse(parts[2], out double s))
            {
                return (h * 3600) + (m * 60) + s;
            }
        }
        else if (parts.Length == 2)
        {
            if (double.TryParse(parts[0], out double m) &&
                double.TryParse(parts[1], out double s))
            {
                return (m * 60) + s;
            }
        }

        return 0.0;
    }

    #endregion
}
