using Facebook.Unity;
#if PGS_V2
using GooglePlayGames;
using GooglePlayGames.BasicApi;
#endif
using System.Collections.Generic;
using System.Collections;
using System.Text;
using TMPro;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Networking;

public class AuthManager_V2 : MonoBehaviour
{
#if PGS_V2
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
    
    private const string verify_and_link_google = "http://192.168.0.102:3000/verify_and_link_google";
    private const string verify_and_link_facebook = "http://192.168.0.102:3000/verify_and_link_facebook";
    private const string post_count = "http://192.168.0.102:3000/post_count";
    
    [System.Serializable]
    private class GoogleAuthRequest
    {
        public string idToken;
        public string playerID;
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
        showAchievementButton =  gamePanel.transform.Find("ShowAchievement").GetComponent<Button>();
        
        
        statusText.text = "Initializing PGS v1...";
        var config = new PlayGamesClientConfiguration.Builder()
            .RequestEmail()
            // .RequestServerAuthCode(false)
            .RequestIdToken() 
            .Build();
        
        PlayGamesPlatform.InitializeInstance(config);
        PlayGamesPlatform.DebugLogEnabled = true;
        PlayGamesPlatform.Activate();
        
        
        if (!FB.IsInitialized)
        {
            FB.Init(OnInitComplete, OnHideUnity);
        }
        else
        {
            FB.ActivateApp();
        }
        
        getStartedButton.onClick.AddListener(GetStartedClicked);
        iAlreadyHaveButton.onClick.AddListener(IAlreadyHaveButtonClicked);
        signInWithGoogleButton.onClick.AddListener(OnSignInWithGoogleClicked);
        signInWithFacebookButton.onClick.AddListener(OnSignInWithFacebookClicked);
        signOutButton.onClick.AddListener(OnSignOutClicked);
        incButton.onClick.AddListener(OnIncButtonClicked);
        unlockAchievementButton.onClick.AddListener(OnAchievementUnlockButtonClicked);
        showAchievementButton.onClick.AddListener(OnShowAchievementsButtonClicked);
        
        statusText.text = "Checking credentials...";
        PlayGamesPlatform.Instance.Authenticate(OnSilentSignInFinished, true);
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
        
        // Report 100% progress to unlock the achievement
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
    
    private void ProcessAuthenticationResult(bool success)
    {
        if (success)
        {
            statusText.text = "PGS Sign-in Successful! Getting Server Code...";
            string idToken = PlayGamesPlatform.Instance.GetIdToken();
            string playerID = PlayGamesPlatform.Instance.GetUserId();

            if (!string.IsNullOrEmpty(idToken))
            {
                Debug.Log($"PGS: Retrieved Server Auth Code. Sending to backend...");
                statusText.text = "Connecting to game server...";
                // Send the code to the server for verification and linking
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
    
    private IEnumerator PostScore()
    {
        if (string.IsNullOrEmpty(customJwtToken))
        {
            Debug.LogError("Not logged in! (customJwtToken is null).");
            statusText.text = "Error: Not signed in. Cannot save.";
            yield break;
        }

        // 1. Create the new, simpler payload
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
        
        // 3. --- NEW: Add the JWT as an Authorization header ---
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
                // Here you should force the user to sign out and log in again
                // OnSignOutClicked();
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
        PlayGamesPlatform.Instance.Authenticate(ProcessAuthenticationResult, false);
    }
    
    private void OnSignInWithGoogleClicked()
    {
        statusText.text = "Signing in with Google...";
        loginButtonsPanel.SetActive(false);
        PlayGamesPlatform.Instance.Authenticate(ProcessAuthenticationResult, false);
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
            
            // --- REMOVED THIS LINE ---
            // FB.API("/me?fields=name,email", HttpMethod.GET, OnFacebookGraphResult);

            // --- ADD THIS LINE ---
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

        // 4. Handle the response (this is identical to VerifyAndLinkAccount)
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
            
            // Note: response.email might be null if the user didn't grant permission
            statusText.text = $"Signed in as: {response.email ?? "Facebook User"}\nIn-Game ID: {response.inGameAccountID}";
            incText.text = response.inGameCount.ToString("000");
            
            customJwtToken = response.jwtToken;
            
            ShowGamePanel();
        }
    }
    
    private void OnSignOutClicked()
    {
        statusText.text = "Signing out...";
        
        if (PlayGamesPlatform.Instance.IsAuthenticated())
        {
            PlayGamesPlatform.Instance.SignOut();
        }
        if (FB.IsLoggedIn)
        {
            FB.LogOut();
        }
        
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
#endif
}