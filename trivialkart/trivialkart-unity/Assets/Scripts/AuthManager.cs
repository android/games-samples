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
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Networking;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using TMPro;

#if PGS_V1 || PGS_V2
using Facebook.Unity;
using GooglePlayGames;
using GooglePlayGames.BasicApi;
#endif

#if PGS_V2
using Google; // From Google Sign-In plugin
using System.Threading.Tasks;
#endif

public class AuthManager : MonoBehaviour
{
#if PGS_V1 || PGS_V2
    private GameObject startPanel;
    private GameObject loginButtonsPanel;
    private GameObject gamePanel;
    private Button getStartedButton;
    private Button iAlreadyHaveButton;
    private Button signInWithGoogleButton;
    private Button signInWithFacebookButton;
    private Button signOutButton;
    private Button incButton;
    private Button unlockAchievementButton;
    private Button showAchievementButton;
    private TextMeshProUGUI statusText;
    private TextMeshProUGUI incText;
    private string customJwtToken;

#if PGS_V2
    // V2 (CredMan) Specific Variables
    private volatile bool googleTaskComplete = false;
    private string authCodeToExchange = null;
    private string credManError = null;
#endif

    public string serverUrl;
    public string webClientId;
    private string verify_and_link_google;
    private string exchange_authcode_and_link;
    private string verify_and_link_facebook;
    private string post_count;

    [Serializable]
    private class GoogleAuthRequest
    {
#if PGS_V1
        public string idToken;
#elif PGS_V2
        public string authCode;
#endif
    }

    [Serializable]
    private class FacebookAuthRequest
    {
        public string accessToken;
    }

    [Serializable]
    private class PostCountRequest
    {
        public int count;
    }

    [Serializable]
    private class LinkResponse
    {
        public string email;
        public string inGameAccountID;
        public int inGameCount;
        public string jwtToken;
    }

    private void Awake()
    {
        verify_and_link_google = serverUrl + "/verify_and_link_google";
        verify_and_link_facebook = serverUrl + "/verify_and_link_facebook";
        exchange_authcode_and_link = serverUrl + "/exchange_authcode_and_link";
        post_count = serverUrl + "/post_count";
        
        // UI setup
        startPanel = GameObject.Find("Canvas").transform.Find("StartPanel").gameObject;
        loginButtonsPanel = GameObject.Find("Canvas").transform.Find("LoginPanel").gameObject;
        gamePanel = GameObject.Find("Canvas").transform.Find("GamePanel").gameObject;
        statusText = GameObject.Find("Canvas").transform.Find("StatusText").GetComponent<TextMeshProUGUI>();

        getStartedButton = startPanel.transform.Find("GetStarted").GetComponent<Button>();
        iAlreadyHaveButton = startPanel.transform.Find("IAlreadyHave").GetComponent<Button>();
        signInWithGoogleButton = loginButtonsPanel.transform.Find("SIWG").GetComponent<Button>();
        signInWithFacebookButton = loginButtonsPanel.transform.Find("FB").GetComponent<Button>();
        signOutButton = gamePanel.transform.Find("SignOut").GetComponent<Button>();
        incText = gamePanel.transform.Find("IncText").GetComponent<TextMeshProUGUI>();
        incButton = gamePanel.transform.Find("Inc").GetComponent<Button>();
        unlockAchievementButton = gamePanel.transform.Find("UnlockAchievement").GetComponent<Button>();
        showAchievementButton = gamePanel.transform.Find("ShowAchievement").GetComponent<Button>();

#if PGS_V1
        // --- V1 INITIALIZATION ---
        statusText.text = "Initializing PGS v1...";
        var config = new PlayGamesClientConfiguration.Builder()
            .RequestEmail()
            .RequestIdToken()
            .Build();

        PlayGamesPlatform.InitializeInstance(config);
        PlayGamesPlatform.DebugLogEnabled = true;
        PlayGamesPlatform.Activate();
#elif PGS_V2
        // V2 Initialization
        statusText.text = "Initializing...";
        PlayGamesPlatform.DebugLogEnabled = true;
#endif

        // Facebook initialization
        if (!FB.IsInitialized)
        {
            FB.Init(OnInitComplete, OnHideUnity);
        }
        else
        {
            FB.ActivateApp();
        }

        // Button listeners
        getStartedButton.onClick.AddListener(GetStartedClicked);
        iAlreadyHaveButton.onClick.AddListener(IAlreadyHaveButtonClicked);
        signInWithGoogleButton.onClick.AddListener(OnSignInWithGoogleClicked);
        signInWithFacebookButton.onClick.AddListener(OnSignInWithFacebookClicked);
        signOutButton.onClick.AddListener(OnSignOutClicked);
        incButton.onClick.AddListener(OnIncButtonClicked);
        unlockAchievementButton.onClick.AddListener(OnAchievementUnlockButtonClicked);
        showAchievementButton.onClick.AddListener(OnShowAchievementsButtonClicked);

        statusText.text = "Checking credentials...";
#if PGS_V1
        PlayGamesPlatform.Instance.Authenticate(OnSilentSignInFinished, true);
#elif PGS_V2
        if (TryLoadSession())
        {
            Debug.Log("Valid session found. Skipping CredMan.");
            ShowGamePanel();
            SignInToPlayGamesServices(); // Achievements only
        }
        else if (PlayerPrefs.GetInt("UserSignedOut", 0) == 0)
        {
            Debug.Log("Attempting CredMan Silent Sign-In...");
            StartSignIn(false); // Silent Mode
        }
        else
        {
            ShowStartPanel();
        }
#endif
    }
    
    private void Update()
    {
#if PGS_V2
        // V2 Main Thread Dispatcher
        if (googleTaskComplete)
        {
            googleTaskComplete = false;
            if (!string.IsNullOrEmpty(credManError))
            {
                if (credManError == "SilentFailed")
                {
                    Debug.Log("CredMan Silent Sign-In failed.");
                    statusText.text = "Please sign in.";
                }
                else
                {
                    Debug.LogError("CredMan Error: " + credManError);
                    statusText.text = "Sign-in Failed.";
                }
                ShowStartPanel();
            }
            else if (!string.IsNullOrEmpty(authCodeToExchange))
            {
                Debug.Log("Got Auth Code. Exchanging...");
                statusText.text = "Connecting to server...";
                StartCoroutine(ExchangeAuthcodeAndLink(authCodeToExchange));
            }
            credManError = null;
            authCodeToExchange = null;
        }
#endif
    }


    private void OnShowAchievementsButtonClicked()
    {
        Debug.Log("Show achievement button");
        PlayGamesPlatform.Instance.ShowAchievementsUI();
    }

    private void OnAchievementUnlockButtonClicked()
    {
        if (!PlayGamesPlatform.Instance.IsAuthenticated())
        {
            Debug.LogWarning("Not authenticated with PGS. Cannot unlock achievement.");
            statusText.text = "Error: Not signed in to PGS.";
            //SignInToPlayGamesServices();
            return;
        }

        statusText.text = "Unlocking achievement...";

        PlayGamesPlatform.Instance.ReportProgress(
            GPGSIds.achievement_tk_achievement_rand,
            100f,
            (bool success) =>
            {
                if (success)
                {
                    Debug.Log("Achievement unlocked successfully!");
                    statusText.text = "Achievement Unlocked!";
                }
                else
                {
                    Debug.LogWarning("Failed to unlock achievement.");
                    statusText.text = "Failed to unlock achievement.";
                }
            });
    }

    private void OnIncButtonClicked()
    {
        var currNum = int.Parse(incText.text);
        currNum++;
        incText.text = currNum.ToString();

        StartCoroutine(PostScore());
    }

    // --- Facebook Methods (Unchanged) ---
    private void OnInitComplete()
    {
        if (FB.IsInitialized)
        {
            FB.ActivateApp();
            Debug.Log("Facebook SDK Initialized.");
        }
        else
        {
            Debug.LogError("Failed to Initialize the Facebook SDK.");
            statusText.text = "Facebook SDK failed to init.";
        }
    }

    private void OnHideUnity(bool isGameShown)
    {
        Time.timeScale = isGameShown ? 1 : 0;
    }

    
#if PGS_V1
    private void OnSilentSignInFinished(bool success)
    {
        if (success)
        {
            Debug.Log("PGS Silent sign-in successful. Verifying with server...");
            statusText.text = "Verifying with server...";
            ProcessAuthenticationResult(true);
        }
        else
        {
            Debug.Log("PGS Silent sign-in failed. Showing manual start panel.");
            statusText.text = "Please sign in.";
            ShowStartPanel();
        }
    }
    
    private void ProcessAuthenticationResult(bool success)
    {
        if (success)
        {
            statusText.text = "PGS Sign-in Successful! Getting Server Code...";
            string idToken = PlayGamesPlatform.Instance.GetIdToken();

            if (!string.IsNullOrEmpty(idToken))
            {
                Debug.Log($"PGS: Retrieved Server Auth Code. Sending to backend...");
                statusText.text = "Connecting to game server...";
                StartCoroutine(VerifyAndLinkGoogleAccount(idToken));
            }
            else
            {
                Debug.LogError("PGS: Failed to retrieve Server Auth Code.");
                statusText.text = "Failed to get server access. Please try again.";
                ShowStartPanel();
            }
        }
        else
        {
            Debug.LogError("PGS Sign-in failed or was cancelled.");
            statusText.text = "Sign-in failed or was cancelled.";
            ShowStartPanel();
        }
    }
    
    private IEnumerator VerifyAndLinkGoogleAccount(string idToken)
    {
        GoogleAuthRequest requestData = new GoogleAuthRequest { idToken = idToken };
        string jsonPayload = JsonUtility.ToJson(requestData);
        byte[] bodyRaw = Encoding.UTF8.GetBytes(jsonPayload);

        UnityWebRequest request = new UnityWebRequest(verify_and_link_google, "POST");
        request.uploadHandler = new UploadHandlerRaw(bodyRaw);
        request.downloadHandler = new DownloadHandlerBuffer();
        request.SetRequestHeader("Content-Type", "application/json");

        yield return request.SendWebRequest();

        if (request.result != UnityWebRequest.Result.Success)
        {
            Debug.LogError($"Backend Error: {request.error}");
            Debug.LogError($"Response: {request.downloadHandler.text}");
            statusText.text = "Failed to link account. Server error.";
            ShowStartPanel();
        }
        else
        {
            var jsonResponse = request.downloadHandler.text;
            var response = JsonUtility.FromJson<LinkResponse>(jsonResponse);

            Debug.Log($"Successfully linked! Email: {response.email}, In-Game ID: {response.inGameAccountID}");

            statusText.text = $"Signed in as: {response.email}\nIn-Game ID: {response.inGameAccountID}";
            incText.text = response.inGameCount.ToString("000");
            customJwtToken = response.jwtToken;

            ShowGamePanel();
        }
    }
#endif

#if PGS_V2
    private bool TryLoadSession()
    {
        string token = PlayerPrefs.GetString("Cached_JWT", null);
        if (string.IsNullOrEmpty(token)) return false;

        this.customJwtToken = token;
        string email = PlayerPrefs.GetString("Cached_Email", "");
        int count = PlayerPrefs.GetInt("Cached_Count", 0);

        statusText.text = $"Signed in as: {email}";
        incText.text = count.ToString("000");
        return true;
    }

    private void SaveSession(LinkResponse data)
    {
        PlayerPrefs.SetString("Cached_JWT", data.jwtToken);
        PlayerPrefs.SetString("Cached_Email", data.email);
        PlayerPrefs.SetString("Cached_ID", data.inGameAccountID);
        PlayerPrefs.SetInt("Cached_Count", data.inGameCount);
        PlayerPrefs.SetInt("UserSignedOut", 0);
        PlayerPrefs.Save();
        this.customJwtToken = data.jwtToken;
    }

    private void ClearSession()
    {
        PlayerPrefs.DeleteKey("Cached_JWT");
        PlayerPrefs.DeleteKey("Cached_Email");
        PlayerPrefs.DeleteKey("Cached_ID");
        PlayerPrefs.DeleteKey("Cached_Count");
        PlayerPrefs.SetInt("UserSignedOut", 1);
        PlayerPrefs.Save();
        this.customJwtToken = null;
    }

    public void StartSignIn(bool interactive)
    {
        AndroidJavaClass unityPlayer = new AndroidJavaClass("com.unity3d.player.UnityPlayer");
        AndroidJavaObject currentActivity = unityPlayer.GetStatic<AndroidJavaObject>("currentActivity");
        AndroidJavaClass bridge = new AndroidJavaClass("com.gamesamples.trivialkartunity.CredManBridge");
        
        string methodName = interactive ? "signInInteractive" : "signInSilent";
        bridge.CallStatic(methodName, currentActivity, this.webClientId);
    }
    
    public void OnSignInSuccess(string token) { authCodeToExchange = token; googleTaskComplete = true; }
    public void OnSignInError(string error) 
    { 
        credManError = error; 
        googleTaskComplete = true; 
    }
    
    private IEnumerator ExchangeAuthcodeAndLink(string serverAuthCode)
    {
        Debug.Log("Exchange Authcode And Link " + serverAuthCode);
        
        GoogleAuthRequest requestData = new GoogleAuthRequest { authCode = serverAuthCode };
        string jsonPayload = JsonUtility.ToJson(requestData);
        byte[] bodyRaw = Encoding.UTF8.GetBytes(jsonPayload);

        UnityWebRequest request = new UnityWebRequest(exchange_authcode_and_link, "POST");
        request.uploadHandler = new UploadHandlerRaw(bodyRaw);
        request.downloadHandler = new DownloadHandlerBuffer();
        request.SetRequestHeader("Content-Type", "application/json");

        yield return request.SendWebRequest();
        
        if (request.result != UnityWebRequest.Result.Success)
        {
            Debug.LogError($"Backend Error: {request.error}");
            Debug.LogError($"Response: {request.downloadHandler.text}");
            statusText.text = "Failed to link account. Server error.";
            ShowStartPanel();
        }
        else
        {
            var jsonResponse = request.downloadHandler.text;
            var response = JsonUtility.FromJson<LinkResponse>(jsonResponse);

            Debug.Log($"Successfully linked! Email: {response.email}, In-Game ID: {response.inGameAccountID}");

            statusText.text = $"Signed in as: {response.email}\nIn-Game ID: {response.inGameAccountID}";
            incText.text = response.inGameCount.ToString("000");
            customJwtToken = response.jwtToken;

            ShowGamePanel();
            
            SignInToPlayGamesServices();
        }
    }
    
    private void SignInToPlayGamesServices()
    {
        PlayGamesPlatform.Instance.Authenticate((SignInStatus status) => { Debug.Log("PGS Auth: " + status); });
    }
#endif

    // ---
    // == COMMON METHODS ==
    // ---
    private IEnumerator PostScore()
    {
        if (string.IsNullOrEmpty(customJwtToken))
        {
            Debug.LogError("Not logged in! (customJwtToken is null).");
            statusText.text = "Error: Not signed in. Cannot save.";
            yield break;
        }

        PostCountRequest requestData = new PostCountRequest
        {
            count = int.Parse(incText.text)
        };
        string jsonPayload = JsonUtility.ToJson(requestData);
        byte[] bodyRaw = Encoding.UTF8.GetBytes(jsonPayload);

        UnityWebRequest request = new UnityWebRequest(post_count, "POST");
        request.uploadHandler = new UploadHandlerRaw(bodyRaw);
        request.downloadHandler = new DownloadHandlerBuffer();
        
        request.SetRequestHeader("Content-Type", "application/json");
        request.SetRequestHeader("Authorization", "Bearer " + this.customJwtToken);

        yield return request.SendWebRequest();

        if (request.result != UnityWebRequest.Result.Success)
        {
            Debug.LogError($"Backend Error: {request.error}");
            Debug.LogError($"Response: {request.downloadHandler.text}");
            statusText.text = "Failed to post count. Server error.";

            if (request.responseCode == 401 || request.responseCode == 403)
            {
                statusText.text = "Session expired. Please sign out and in again.";
            }
        }
        else
        {
            var jsonResponse = request.downloadHandler.text;
            var response = JsonUtility.FromJson<LinkResponse>(jsonResponse);
            incText.text = response.inGameCount.ToString("000");
        }
    }

    private void IAlreadyHaveButtonClicked()
    {
        startPanel.SetActive(false);
        loginButtonsPanel.SetActive(true);
    }

    private void GetStartedClicked()
    {
        statusText.text = "Signing in with Google...";
        startPanel.SetActive(false);
#if PGS_V1
        PlayGamesPlatform.Instance.Authenticate(ProcessAuthenticationResult, false);
#elif PGS_V2
        StartSignIn(true);
#endif
    }

    private void OnSignInWithGoogleClicked()
    {
        statusText.text = "Signing in with Google...";
        loginButtonsPanel.SetActive(false);
#if PGS_V1
        PlayGamesPlatform.Instance.Authenticate(ProcessAuthenticationResult, false);
#elif PGS_V2
        StartSignIn(true);
#endif
    }

    // --- Facebook Methods ---
    private void OnSignInWithFacebookClicked()
    {
        if (!FB.IsInitialized)
        {
            statusText.text = "Facebook SDK not ready. Retrying init...";
            Debug.Log("FB SDK not ready. Calling Init...");
            FB.Init(OnInitComplete, OnHideUnity);
            return;
        }

        statusText.text = "Logging in with Facebook...";
        loginButtonsPanel.SetActive(false);

        var perms = new List<string>() { "public_profile", "email" };
        FB.LogInWithReadPermissions(perms, OnFacebookLoginComplete);
    }

    private void OnFacebookLoginComplete(ILoginResult result)
    {
        if (result.Error != null)
        {
            Debug.LogError($"Facebook Login Error: {result.Error}");
            statusText.text = "Facebook login failed.";
            ShowStartPanel();
            return;
        }

        if (result.Cancelled)
        {
            Debug.Log("Facebook Login Cancelled.");
            statusText.text = "Facebook login cancelled.";
            ShowStartPanel();
            return;
        }

        if (FB.IsLoggedIn)
        {
            var aToken = AccessToken.CurrentAccessToken;
            Debug.Log($"Facebook Access Token: {aToken.TokenString}");
            
            statusText.text = "Connecting to game server...";
            StartCoroutine(VerifyAndLinkFacebookAccount(aToken.TokenString));
        }
        else
        {
            Debug.LogWarning("Facebook login reported success, but FB.IsLoggedIn is false.");
            statusText.text = "Facebook login failed.";
            ShowStartPanel();
        }
    }

    private IEnumerator VerifyAndLinkFacebookAccount(string accessToken)
    {
        FacebookAuthRequest requestData = new FacebookAuthRequest { accessToken = accessToken };
        string jsonPayload = JsonUtility.ToJson(requestData);
        byte[] bodyRaw = Encoding.UTF8.GetBytes(jsonPayload);

        UnityWebRequest request = new UnityWebRequest(verify_and_link_facebook, "POST");
        request.uploadHandler = new UploadHandlerRaw(bodyRaw);
        request.downloadHandler = new DownloadHandlerBuffer();
        request.SetRequestHeader("Content-Type", "application/json");

        yield return request.SendWebRequest();

        if (request.result != UnityWebRequest.Result.Success)
        {
            Debug.LogError($"Backend Error: {request.error}");
            Debug.LogError($"Response: {request.downloadHandler.text}");
            statusText.text = "Failed to link FB account. Server error.";
            ShowStartPanel();
        }
        else
        {
            var jsonResponse = request.downloadHandler.text;
            var response = JsonUtility.FromJson<LinkResponse>(jsonResponse);

            Debug.Log($"Successfully linked! Email: {response.email}, In-Game ID: {response.inGameAccountID}");

            statusText.text = $"Signed in as: {response.email ?? "Facebook User"}\nIn-Game ID: {response.inGameAccountID}";
            incText.text = response.inGameCount.ToString("000");
            customJwtToken = response.jwtToken;

            ShowGamePanel();
        }
    }
    
    private void OnSignOutClicked()
    {
        statusText.text = "Signing out...";

#if PGS_V1
        if (PlayGamesPlatform.Instance.IsAuthenticated())
        {
            PlayGamesPlatform.Instance.SignOut();
        }
#elif PGS_V2
        ClearSession();
#endif

        if (FB.IsLoggedIn)
        {
            FB.LogOut();
        }
        
        customJwtToken = null;
        ShowStartPanel();
    }

    // --- UI Panel Helpers (Unchanged) ---
    private void ShowGamePanel()
    {
        gamePanel.SetActive(true);
        startPanel.SetActive(false);
        loginButtonsPanel.SetActive(false);
    }

    private void ShowStartPanel()
    {
        gamePanel.SetActive(false);
        startPanel.SetActive(true);
        loginButtonsPanel.SetActive(false);
    }
#endif // End of #if PGS_V1 || PGS_V2
}