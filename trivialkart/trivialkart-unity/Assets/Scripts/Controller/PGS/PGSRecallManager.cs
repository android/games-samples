#if PLAY_GAMES_SERVICES
using GooglePlayGames;
#endif
using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Networking;
using UnityEngine.UI;

public class PGSRecallManager : MonoBehaviour
{
#if RECALL_API
    // --- Server Configuration ---
    private const string SERVER_BASE_URL = "http://192.168.0.103:3000"; // Update with your IP
    private const string RECALL_SESSION_ENDPOINT = SERVER_BASE_URL + "/recall-session";
    private const string CREATE_ACCOUNT_ENDPOINT = SERVER_BASE_URL + "/create-account";
    private const string UPDATE_PROGRESS_ENDPOINT = SERVER_BASE_URL + "/update-progress";
    
    private string _currentRecallSessionId;
    private string _playerRecallToken;
    private Coroutine _distanceUpdateCoroutine;
    private float _lastDistanceSentToServer = -1f;
    private const float UPDATE_INTERVAL_SECONDS = 5.0f;
    
    private Transform _dummyLoginPanel;
    private InputField _usernameInputField;
    private Button _loginButton;
    private Text _usernameText;
    
    [Serializable]
    public class ServerResponse
    {
        public string status;
        public PlayerData playerData;
        public string playerRecallToken;
    }

    [Serializable]
    public class PlayerData
    {
        public string username;
        public int coinsOwned;
        public float distanceTraveled;
    }

    [Serializable]
    private class RecallSessionPayload { public string token; }
    
    [Serializable]
    private class CreateAccountPayload
    {
        public string recallSessionId;
        public string username;
        public int coinsOwned;
        public float distanceTraveled;
    }

    [Serializable]
    private class UpdateProgressPayload
    {
        public string playerRecallToken;
        public float distanceTraveled;
    }
    
    private void Start()
    {
        var playboardCanvas = GameObject.Find("PlayBoardCanvas");
        _dummyLoginPanel = playboardCanvas.transform.Find("DummyLoginPanel");
        _usernameText = playboardCanvas.transform.Find("UsernameLabel").GetComponent<Text>();
        _usernameInputField = _dummyLoginPanel.transform.Find("Username").GetComponent<InputField>();
        _loginButton = _dummyLoginPanel.transform.Find("Login").GetComponent<Button>();
        
        _loginButton.onClick.AddListener(DummyLogin);
    }

    /// <summary>
    /// Starts the process of restoring a player's session.
    /// </summary>
    public void TryRestorePlayerSession()
    {
#if PLAY_GAMES_SERVICES
        if (!PlayGamesPlatform.Instance.IsAuthenticated())
        {
            Debug.LogError("[PGSRecallManager] User is not authenticated with Play Games.");
            return;
        }
        
        Debug.Log("[PGSRecallManager] Requesting Recall Session ID from Google...");
        
        PlayGamesPlatform.Instance.RequestRecallAccess(recallAccess =>
        {
            if (recallAccess != null)
            {
                _currentRecallSessionId = recallAccess.sessionId;
                Debug.Log($"[PGSRecallManager] Success! Received Session ID: {_currentRecallSessionId[..20]}...");
                StartCoroutine(ValidateRecallSession(_currentRecallSessionId));
            }
            else
            {
                Debug.LogError("[PGSRecallManager] Failed to get Recall Session ID. The recallAccess object was null.");
            }
        });
#endif
    }

    private void DummyLogin()
    {
        if (string.IsNullOrEmpty(_usernameInputField.text))
        {
            Debug.LogError("[PGSRecallManager] Username is required.");
            return;
        }

        _usernameText.text = _usernameInputField.text;
        CreateNewAccount(_usernameInputField.text);
    }
    
    /// <summary>
    /// Call this after the player enters their new username in the UI.
    /// </summary>
    private void CreateNewAccount(string username)
    {
#if PLAY_GAMES_SERVICES
        if (string.IsNullOrEmpty(_currentRecallSessionId))
        {
            PlayGamesPlatform.Instance.RequestRecallAccess(recallAccess =>
            {
                if (recallAccess != null)
                {
                    _currentRecallSessionId = recallAccess.sessionId;
                    Debug.Log($"[PGSRecallManager] Success! Received Session ID: {_currentRecallSessionId.Substring(0, 20)}...");
                    StartCoroutine(SendCreateAccountRequest(username));
                }
                else
                {
                    Debug.LogError("[PGSRecallManager] Failed to get Recall Session ID. The recallAccess object was null.");
                }
            });
            return;
        }
        
        StartCoroutine(SendCreateAccountRequest(username));
#endif
    }

    private IEnumerator ValidateRecallSession(string sessionId)
    {
        var payload = new RecallSessionPayload { token = sessionId };
        var jsonPayload = JsonUtility.ToJson(payload);

        yield return PostRequest(RECALL_SESSION_ENDPOINT, jsonPayload, (responseJson) =>
        {
            var response = JsonUtility.FromJson<ServerResponse>(responseJson);

            if (response.status == "AccountFound")
            {
                _dummyLoginPanel.gameObject.SetActive(false);
                Debug.Log($"[PGSRecallManager] Welcome back, {response.playerData.username}!");
                
                _playerRecallToken = response.playerRecallToken;
                StartProgressUpdateLoop();

                _usernameText.text = response.playerData.username;
                PlayerPrefs.SetInt("coinsOwned", response.playerData.coinsOwned);
                PlayerPrefs.SetFloat("dist", response.playerData.distanceTraveled);
            }
            else if (response.status == "NewPlayer")
            {
                Debug.Log("[PGSRecallManager] This is a new player. Showing account creation UI...");
                _dummyLoginPanel.gameObject.SetActive(true);
            }
        });
    }

    private IEnumerator SendCreateAccountRequest(string username)
    {
        var payload = new CreateAccountPayload
        {
            recallSessionId = _currentRecallSessionId,
            username = username,
            coinsOwned = PlayerPrefs.GetInt("coinsOwned"),
            distanceTraveled = PlayerPrefs.GetFloat("dist")
        };
        var jsonPayload = JsonUtility.ToJson(payload);

        yield return PostRequest(CREATE_ACCOUNT_ENDPOINT, jsonPayload, (responseJson) =>
        {
            var response = JsonUtility.FromJson<ServerResponse>(responseJson);
            if(response.status == "AccountCreated")
            {
                _dummyLoginPanel.gameObject.SetActive(false);
                Debug.Log($"[PGSRecallManager] New account created for {response.playerData.username}! Welcome!");
                
                // ***MODIFIED***: Store the token and start the update loop
                _playerRecallToken = response.playerRecallToken;
                StartProgressUpdateLoop();
            }
        });
    }
    
    private void StartProgressUpdateLoop()
    {
        if (_distanceUpdateCoroutine != null)
        {
            StopCoroutine(_distanceUpdateCoroutine);
        }
        _distanceUpdateCoroutine = StartCoroutine(PeriodicDistanceUpdate());
    }
    
    private IEnumerator PeriodicDistanceUpdate()
    {
        Debug.Log("[PGSRecallManager] Starting periodic distance updates to the server.");
        while (true)
        {
            yield return new WaitForSeconds(UPDATE_INTERVAL_SECONDS);

            float currentDistance = PlayerPrefs.GetFloat("dist", 0f);

            // Only send an update if the value has changed meaningfully
            if (Mathf.Abs(currentDistance - _lastDistanceSentToServer) > 0.1f)
            {
                StartCoroutine(SendDistanceUpdate(currentDistance));
            }
        }
    }
    
    private IEnumerator SendDistanceUpdate(float distance)
    {
        if (string.IsNullOrEmpty(_playerRecallToken))
        {
            Debug.LogWarning("[PGSRecallManager] Cannot update distance, player token is missing.");
            yield break;
        }

        var payload = new UpdateProgressPayload
        {
            playerRecallToken = _playerRecallToken,
            distanceTraveled = distance
        };
        string jsonPayload = JsonUtility.ToJson(payload);
        
        yield return PostRequest(UPDATE_PROGRESS_ENDPOINT, jsonPayload, (responseJson) =>
        {
            _lastDistanceSentToServer = distance;
            Debug.Log($"[PGSRecallManager] Successfully updated distance on server to {distance}.");
        });
    }

    private static IEnumerator PostRequest(string url, string jsonPayload, Action<string> onSuccess)
    {
        using var www = new UnityWebRequest(url, "POST");
        var bodyRaw = System.Text.Encoding.UTF8.GetBytes(jsonPayload);
        www.uploadHandler = new UploadHandlerRaw(bodyRaw);
        www.downloadHandler = new DownloadHandlerBuffer();
        www.SetRequestHeader("Content-Type", "application/json");
        Debug.Log($"[PGSRecallManager] Sending POST to {url}");
        yield return www.SendWebRequest();
        if (www.result != UnityWebRequest.Result.Success)
        {
            Debug.LogError($"[PGSRecallManager] Server Error: {www.error}. Response: {www.downloadHandler.text}");
        }
        else
        {
            Debug.Log($"[PGSRecallManager] Server Response: {www.downloadHandler.text}");
            onSuccess?.Invoke(www.downloadHandler.text);
        }
    }
#endif
}