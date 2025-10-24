using UnityEngine;
using UnityEngine.UI;
using GooglePlayGames;
using GooglePlayGames.BasicApi;
using TMPro;
using Facebook.Unity;
using System.Collections.Generic;
using System.Collections;
using UnityEngine.Networking;
using System.Text;

public class AuthManager_V1 : MonoBehaviour
{
#if PGS_V1
    private GameObject startPanel;
    private GameObject loginButtonsPanel;
    private GameObject gamePanel;
    private Button getStartedButton;
    private Button iAlreadyHaveButton;
    private Button signInWithGoogleButton;
    private Button signInWithFacebookButton;
    private Button signOutButton;
    private Button incButton;
    private TextMeshProUGUI statusText;
    private TextMeshProUGUI incText;

    // We'll store the backend URL here
    private string backendUrl = "http://192.168.0.101:3000/verify_and_link";

    // Helper classes for JSON serialization
    [System.Serializable]
    private class AuthRequest
    {
        public string authCode;
    }

    [System.Serializable]
    private class LinkResponse
    {
        public string email;
        public string inGameAccountId;
    }


    private void Awake()
    {
        // ... (all your existing Awake() code up to the point of config is fine)
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
        
        
        statusText.text = "Initializing PGS v1...";
        var config = new PlayGamesClientConfiguration.Builder()
            .RequestEmail()
            .RequestServerAuthCode(false) 
            .RequestIdToken() 
            .Build();

        // ... (the rest of your Awake() method is fine)
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
        
        statusText.text = "Checking credentials...";
        PlayGamesPlatform.Instance.Authenticate(OnSilentSignInFinished, true);
    }

    private void OnIncButtonClicked()
    {
        var currNum = int.Parse(incText.text);
        currNum++;
        incText.text = currNum.ToString();
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


    // --- THIS IS THE MAINLY EDITED METHOD ---
    private void ProcessAuthenticationResult(bool success)
    {
        if (success)
        {
            statusText.text = "PGS Sign-in Successful! Getting Server Code...";
            string authCode = PlayGamesPlatform.Instance.GetServerAuthCode();

            if (!string.IsNullOrEmpty(authCode))
            {
                Debug.Log($"PGS: Retrieved Server Auth Code. Sending to backend...");
                statusText.text = "Connecting to game server...";
                // Send the code to the server for verification and linking
                StartCoroutine(VerifyAndLinkAccount(authCode));
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

    // --- THIS IS THE NEW COROUTINE ---
    private IEnumerator VerifyAndLinkAccount(string authCode)
    {
        // 1. Create the request payload
        AuthRequest requestData = new AuthRequest { authCode = authCode };
        string jsonPayload = JsonUtility.ToJson(requestData);
        byte[] bodyRaw = Encoding.UTF8.GetBytes(jsonPayload);

        // 2. Create the UnityWebRequest
        UnityWebRequest request = new UnityWebRequest(backendUrl, "POST");
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
            string jsonResponse = request.downloadHandler.text;
            LinkResponse response = JsonUtility.FromJson<LinkResponse>(jsonResponse);

            Debug.Log($"Successfully linked! Email: {response.email}, In-Game ID: {response.inGameAccountId}");
            
            // Store the in-game ID, update the UI, and show the game
            // You might want to save response.inGameAccountId to a static class or PlayerPrefs
            
            statusText.text = $"Signed in as: {response.email}\nIn-Game ID: {response.inGameAccountId}";
            ShowGamePanel();
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
            Debug.Log($"Facebook User ID: {aToken.UserId}");
            Debug.Log($"Facebook Access Token: {aToken.TokenString}");
            
            FB.API("/me?fields=name,email", HttpMethod.GET, OnFacebookGraphResult);
        }
        else
        {
            Debug.LogWarning("Facebook login reported success, but FB.IsLoggedIn is false.");
            statusText.text = "Facebook login failed.";
            ShowStartPanel();
        }
    }
    
    private void OnFacebookGraphResult(IGraphResult result)
    {
        if (result.Error != null)
        {
            Debug.LogError($"Facebook Graph Error: {result.Error}");
            statusText.text = "FB login error (Graph).";
            ShowStartPanel();
            return;
        }
        
        var name =
 result.ResultDictionary.TryGetValue("name", out var value) ? value.ToString() : "FB User";
        var email =
 result.ResultDictionary.TryGetValue("email", out var value1) ? value1.ToString() : "No Email";

        // TODO: You would also send the FB Access Token to your server
        // to verify and link the FB account, just like you do for Google.
        Debug.Log($"Facebook Name: {name}, Email: {email}");
        statusText.text = $"Signed in as: {name} ({email})";
        ShowGamePanel();
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