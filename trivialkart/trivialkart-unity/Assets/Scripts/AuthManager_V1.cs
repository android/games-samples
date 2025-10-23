using UnityEngine;
using UnityEngine.UI;
using GooglePlayGames;
using GooglePlayGames.BasicApi;
using TMPro;
using Facebook.Unity;
using System.Collections.Generic;

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
        
        
        statusText.text = "Initializing PGS v1...";
        var config = new PlayGamesClientConfiguration.Builder()
            .RequestEmail()
            .RequestServerAuthCode(false) 
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
            Debug.Log("PGS Silent sign-in successful.");
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
            statusText.text = "PGS Sign-in Successful!";
            string authCode = PlayGamesPlatform.Instance.GetServerAuthCode();
            string email = PlayGamesPlatform.Instance.GetUserEmail(); 

            if (!string.IsNullOrEmpty(authCode))
            {
                Debug.Log($"PGS: Retrieved Server Auth Code: {authCode}");
                
                if (!string.IsNullOrEmpty(email))
                {
                    Debug.Log("PGS: Email scope was granted!");
                    statusText.text = $"Signed in as: {email}";
                    ShowGamePanel();
                }
                else
                {
                    Debug.LogWarning("PGS: Email scope was NOT granted.");
                    statusText.text = "Email scope not granted. Please try again.";
                    ShowStartPanel();
                }
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
    
    // --- Button Click Handlers ---
    
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