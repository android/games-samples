using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Networking;
using Random = UnityEngine.Random;

// using GooglePlayGames;

public class PGSRecallManager : MonoBehaviour
{
    // --- Server Configuration ---
    private const string SERVER_BASE_URL = "http://192.168.0.101:3000"; // Update with your IP
    private const string RECALL_SESSION_ENDPOINT = SERVER_BASE_URL + "/recall-session";
    private const string CREATE_ACCOUNT_ENDPOINT = SERVER_BASE_URL + "/create-account";

    private string _currentRecallSessionId;

    // --- JSON Helper Classes ---
    [Serializable]
    public class ServerResponse
    {
        public string status;
        public PlayerData playerData;
    }

    [Serializable]
    public class PlayerData
    {
        public string username;
        public int coinsOwned;
        public float distanceTraveled;
    }

    /// <summary>
    /// Starts the process of restoring a player's session.
    /// </summary>
    public void RestorePlayerSession()
    {
        // if (!PlayGamesPlatform.Instance.IsAuthenticated())
        // {
        //     Debug.LogError("[PGSRecallManager] User is not authenticated with Play Games.");
        //     return;
        // }
        //
        // Debug.Log("[PGSRecallManager] Requesting Recall Session ID from Google...");
        //
        // // The callback receives one parameter. We check if it's null to handle errors.
        // PlayGamesPlatform.Instance.RequestRecallAccess(recallAccess =>
        // {
        //     // If the object is not null, the call was successful.
        //     if (recallAccess != null)
        //     {
        //         _currentRecallSessionId = recallAccess.sessionId;
        //         Debug.Log($"[PGSRecallManager] Success! Received Session ID: {_currentRecallSessionId.Substring(0, 20)}...");
        //         StartCoroutine(ValidateRecallSession(_currentRecallSessionId));
        //     }
        //     else
        //     {
        //         // If the object is null, the call failed.
        //         Debug.LogError("[PGSRecallManager] Failed to get Recall Session ID. The recallAccess object was null.");
        //     }
        // });
    }

    public void DummyLogin()
    {
        CreateNewAccount("DummyLoginUser" + Random.Range(100, 999));
    }
    
    /// <summary>
    /// Call this after the player enters their new username in the UI.
    /// </summary>
    private void CreateNewAccount(string username)
    {
        // if (string.IsNullOrEmpty(_currentRecallSessionId))
        // {
        //     PlayGamesPlatform.Instance.RequestRecallAccess(recallAccess =>
        //     {
        //         // If the object is not null, the call was successful.
        //         if (recallAccess != null)
        //         {
        //             _currentRecallSessionId = recallAccess.sessionId;
        //             Debug.Log($"[PGSRecallManager] Success! Received Session ID: {_currentRecallSessionId.Substring(0, 20)}...");
        //             StartCoroutine(SendCreateAccountRequest(username));
        //         }
        //         else
        //         {
        //             // If the object is null, the call failed.
        //             Debug.LogError("[PGSRecallManager] Failed to get Recall Session ID. The recallAccess object was null.");
        //         }
        //     });
        //     return;
        // }
        //
        // StartCoroutine(SendCreateAccountRequest(username));
    }

    private IEnumerator ValidateRecallSession(string sessionId)
    {
        var jsonPayload = $"{{\"token\":\"{sessionId}\"}}";
        yield return PostRequest(RECALL_SESSION_ENDPOINT, jsonPayload, (responseJson) =>
        {
            var response = JsonUtility.FromJson<ServerResponse>(responseJson);

            if (response.status == "AccountFound")
            {
                Debug.Log($"[PGSRecallManager] Welcome back, {response.playerData.username}!");
                PlayerPrefs.SetInt("coinsOwned", response.playerData.coinsOwned);
                PlayerPrefs.SetFloat("dist", response.playerData.distanceTraveled);
            }
            else if (response.status == "NewPlayer")
            {
                Debug.Log("[PGSRecallManager] This is a new player. Showing account creation UI...");
                CreateNewAccount("UnityPlayer" + Random.Range(100, 999));
            }
        });
    }

    private IEnumerator SendCreateAccountRequest(string username)
    {
        var jsonPayload = $"{{\"recallSessionId\":\"{_currentRecallSessionId}\"," +
                          $"\"username\":\"{username}\",\"coinsOwned\":\"{PlayerPrefs.GetInt("coinsOwned")}\"," +
                          $"\"distanceTraveled\":\"{PlayerPrefs.GetFloat("dist")}\"}}";
        yield return PostRequest(CREATE_ACCOUNT_ENDPOINT, jsonPayload, (responseJson) =>
        {
            var response = JsonUtility.FromJson<ServerResponse>(responseJson);
            if(response.status == "AccountCreated")
            {
                Debug.Log($"[PGSRecallManager] New account created for {response.playerData.username}! Welcome!");
            }
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
}