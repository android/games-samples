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

public class AuthManagerV1V2 : MonoBehaviour
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

#if PGS_V1
    private const string verify_and_link_google = "http://192.168.0.102:3000/verify_and_link_google";
    private const string verify_and_link_facebook = "http://192.168.0.102:3000/verify_and_link_facebook";
    private const string post_count = "http://192.168.0.102:3000/post_count";
#elif PGS_V2
    private const string exchange_authcode_and_link = "http://192.168.0.105:3000/exchange_authcode_and_link";
    private const string verify_and_link_facebook = "http://192.168.0.105:3000/verify_and_link_facebook";
    private const string post_count = "http://192.168.0.105:3000/post_count";
#endif

    [System.Serializable]
    private class GoogleAuthRequest
    {
#if PGS_V1
        public string idToken;
        public string playerID;
#elif PGS_V2
        public string authCode;
#endif
    }

    [System.Serializable]
    private class FacebookAuthRequest
    {
        public string accessToken;
    }

    [System.Serializable]
    private class PostCountRequest
    {
        public int count;
    }

    [System.Serializable]
    private class LinkResponse
    {
        public string email;
        public string inGameAccountID;
        public int inGameCount;
        public string jwtToken;
    }

    private void Awake()
    {
        // UI setup is identical in both versions
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
        statusText.text = "Initializing PGS v1...";
        var config = new PlayGamesClientConfiguration.Builder()
            .RequestEmail()
            // .RequestServerAuthCode(false) // Not needed if RequestIdToken is used
            .RequestIdToken() // v1 specific: We ask for the ID Token directly
            .Build();

        PlayGamesPlatform.InitializeInstance(config);
        PlayGamesPlatform.DebugLogEnabled = true;
        PlayGamesPlatform.Activate();
#elif PGS_V2
        PlayGamesPlatform.DebugLogEnabled = true;
#endif

        // Facebook initialization is identical
        if (!FB.IsInitialized)
        {
            FB.Init(OnInitComplete, OnHideUnity);
        }
        else
        {
            FB.ActivateApp();
        }

        // Button listeners are identical
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
        PlayGamesPlatform.Instance.Authenticate(OnAuthenticationFinished);
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
            Debug.LogWarning("Not authenticated. Cannot unlock achievement.");
            statusText.text = "Error: Not signed in.";
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

    // ---
    // Facebook Methods - Identical in both versions
    // ---
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
            // We successfully signed in, now process the result (which will call the backend)
            ProcessAuthenticationResult(true);
        }
        else
        {
            Debug.Log("PGS Silent sign-in failed. Showing manual start panel.");
            statusText.text = "Please sign in.";
            startPanel.SetActive(true);
            loginButtonsPanel.SetActive(false);
            gamePanel.SetActive(false);
        }
    }

    // v1: Main auth processor, called by both silent and interactive sign-in
    private void ProcessAuthenticationResult(bool success)
    {
        if (success)
        {
            statusText.text = "PGS Sign-in Successful! Getting Server Code...";
            // v1: Get ID Token and User ID directly from the client
            string idToken = PlayGamesPlatform.Instance.GetIdToken();
            string playerID = PlayGamesPlatform.Instance.GetUserId();

            if (!string.IsNullOrEmpty(idToken))
            {
                Debug.Log($"PGS: Retrieved Server Auth Code. Sending to backend...");
                statusText.text = "Connecting to game server...";
                // Send the insecure (client-provided) data to the server
                StartCoroutine(VerifyAndLinkGoogleAccount(idToken, playerID));
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

    // v1: Coroutine to send ID Token and Player ID to the server
    private IEnumerator VerifyAndLinkGoogleAccount(string idToken, string playerID)
    {
        // 1. Create the request payload
        GoogleAuthRequest requestData = new GoogleAuthRequest { idToken = idToken, playerID = playerID };
        string jsonPayload = JsonUtility.ToJson(requestData);
        byte[] bodyRaw = Encoding.UTF8.GetBytes(jsonPayload);

        // 2. Create the UnityWebRequest
        UnityWebRequest request = new UnityWebRequest(verify_and_link_google, "POST");
        request.uploadHandler = new UploadHandlerRaw(bodyRaw);
        request.downloadHandler = new DownloadHandlerBuffer();
        request.SetRequestHeader("Content-Type", "application/json");

        // 3. Send the request and wait for a response
        yield return request.SendWebRequest();

        // 4. Handle the response
        if (request.result != UnityWebRequest.Result.Success)
        {
            Debug.LogError($"Backend Error: {request.error}");
            Debug.LogError($"Response: {request.downloadHandler.text}");
            statusText.text = "Failed to link account. Server error.";
            ShowStartPanel();
        }
        else
        {
            // 5. Success! Parse the response from the server
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
    // v2: Main callback for all authentication attempts
    private void OnAuthenticationFinished(SignInStatus status)
    {
        if (status == SignInStatus.Success)
        {
            Debug.Log("PGS Sign-in successful.");
            statusText.text = "Sign-in successful. Getting Server Code...";

            var scopes = new List<AuthScope>
            {
                AuthScope.EMAIL,
                AuthScope.OPEN_ID,
            };

            PlayGamesPlatform.Instance.RequestServerSideAccess(false, scopes, OnServerAuthCodeReceived);
        }
        else
        {
            Debug.LogError("PGS Sign-in failed: " + status);
            statusText.text = "Sign-in failed. Please try again.";
            ShowStartPanel();
        }
    }

    private void OnServerAuthCodeReceived(AuthResponse authResponse)
    {
        if (string.IsNullOrEmpty(authResponse.GetAuthCode()))
        {
            Debug.LogError("PGS: Failed to retrieve Server Auth Code.");
            statusText.text = "Failed to get server access. Please try again.";
            ShowStartPanel();
        }
        else
        {
            Debug.Log($"PGS: Retrieved Server Auth Code. Sending to backend...");
            statusText.text = "Connecting to game server...";

            StartCoroutine(ExchangeAuthcodeAndLink(authResponse.GetAuthCode()));
        }
    }

    // v2: Coroutine to send the one-time Auth Code to the server
    private IEnumerator ExchangeAuthcodeAndLink(string serverAuthCode)
    {
        Debug.Log("Exchange Authcode And Link " + serverAuthCode);
        // 1. Create the request payload
        GoogleAuthRequest requestData = new GoogleAuthRequest { authCode = serverAuthCode };
        string jsonPayload = JsonUtility.ToJson(requestData);
        byte[] bodyRaw = Encoding.UTF8.GetBytes(jsonPayload);

        // 2. Create the UnityWebRequest
        // v2: Use the new endpoint
        UnityWebRequest request = new UnityWebRequest(exchange_authcode_and_link, "POST");
        request.uploadHandler = new UploadHandlerRaw(bodyRaw);
        request.downloadHandler = new DownloadHandlerBuffer();
        request.SetRequestHeader("Content-Type", "application/json");

        // 3. Send the request and wait for a response
        yield return request.SendWebRequest();

        // 4. Handle the response (identical logic to v1, but different endpoint)
        if (request.result != UnityWebRequest.Result.Success)
        {
            Debug.LogError($"Backend Error: {request.error}");
            Debug.LogError($"Response: {request.downloadHandler.text}");
            statusText.text = "Failed to link account. Server error.";
            ShowStartPanel();
        }
        else
        {
            // 5. Success! Parse the response from the server
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
    
    private IEnumerator PostScore()
    {
        if (string.IsNullOrEmpty(customJwtToken))
        {
            Debug.LogError("Not logged in! (customJwtToken is null).");
            statusText.text = "Error: Not signed in. Cannot save.";
            yield break;
        }

        // 1. Create the payload
        PostCountRequest requestData = new PostCountRequest
        {
            count = int.Parse(incText.text)
        };
        string jsonPayload = JsonUtility.ToJson(requestData);
        byte[] bodyRaw = Encoding.UTF8.GetBytes(jsonPayload);

        // 2. Create the UnityWebRequest
        UnityWebRequest request = new UnityWebRequest(post_count, "POST");
        request.uploadHandler = new UploadHandlerRaw(bodyRaw);
        request.downloadHandler = new DownloadHandlerBuffer();

        // 3. Add the JWT as an Authorization header
        request.SetRequestHeader("Content-Type", "application/json");
        request.SetRequestHeader("Authorization", "Bearer " + this.customJwtToken);

        // 4. Send the request
        yield return request.SendWebRequest();

        // 5. Handle response
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
            // 6. Success!
            var jsonResponse = request.downloadHandler.text;
            var response = JsonUtility.FromJson<LinkResponse>(jsonResponse);

            // Update UI (text is already correct, but this confirms)
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
        PlayGamesPlatform.Instance.Authenticate(OnAuthenticationFinished);
#endif
    }

    private void OnSignInWithGoogleClicked()
    {
        statusText.text = "Signing in with Google...";
        loginButtonsPanel.SetActive(false);
#if PGS_V1
        PlayGamesPlatform.Instance.Authenticate(ProcessAuthenticationResult, false);
#elif PGS_V2
        PlayGamesPlatform.Instance.Authenticate(OnAuthenticationFinished);
#endif
    }
    
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

            // Send the token to our server for secure verification
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
        // 1. Create the request payload
        FacebookAuthRequest requestData = new FacebookAuthRequest { accessToken = accessToken };
        string jsonPayload = JsonUtility.ToJson(requestData);
        byte[] bodyRaw = Encoding.UTF8.GetBytes(jsonPayload);

        // 2. Create the UnityWebRequest
        UnityWebRequest request = new UnityWebRequest(verify_and_link_facebook, "POST");
        request.uploadHandler = new UploadHandlerRaw(bodyRaw);
        request.downloadHandler = new DownloadHandlerBuffer();
        request.SetRequestHeader("Content-Type", "application/json");

        // 3. Send the request
        yield return request.SendWebRequest();

        // 4. Handle the response
        if (request.result != UnityWebRequest.Result.Success)
        {
            Debug.LogError($"Backend Error: {request.error}");
            Debug.LogError($"Response: {request.downloadHandler.text}");
            statusText.text = "Failed to link FB account. Server error.";
            ShowStartPanel();
        }
        else
        {
            // 5. Success! Parse the response
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
        // v1 explicitly signs out of the PGS platform
        if (PlayGamesPlatform.Instance.IsAuthenticated())
        {
            PlayGamesPlatform.Instance.SignOut();
        }
#endif

        // FB sign out is the same
        if (FB.IsLoggedIn)
        {
            FB.LogOut();
        }

#if PGS_V2
        // v2 code clears the custom JWT to end the server session
        // but does not explicitly call PlayGamesPlatform.Instance.SignOut()
        customJwtToken = null;
#endif

        ShowStartPanel();
    }
    
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