using UnityEngine;
using UnityEngine.UI;
using GooglePlayGames;
using GooglePlayGames.BasicApi;
using TMPro;
using Facebook.Unity;
using System.Collections.Generic;

public class AuthManager : MonoBehaviour
{
    public GameObject startPanel;
    public GameObject loginButtonsPanel;
    public GameObject gamePanel;
    public Button getStartedButton;
    public Button iAlreadyHaveButton;
    public Button signInWithGoogleButton;
    public Button signInWithFacebookButton;
    public Button signOutButton;
    
    // CHANGED: Corrected the type name
    public TextMeshProUGUI statusText;

    private void Awake()
    {
        // --- Google Play Games Initialization ---
        statusText.text = "Initializing PGS v1...";
        PlayGamesClientConfiguration config = new PlayGamesClientConfiguration.Builder()
            .RequestEmail()
            .RequestServerAuthCode(false) 
            .RequestIdToken() 
            .Build();

        PlayGamesPlatform.InitializeInstance(config);
        PlayGamesPlatform.DebugLogEnabled = true;
        PlayGamesPlatform.Activate();
        
        // --- Facebook SDK Initialization ---
        if (!FB.IsInitialized)
        {
            // Initialize the Facebook SDK
            FB.Init(OnInitComplete, OnHideUnity);
        }
        else
        {
            // Already initialized, signal an app activation event
            FB.ActivateApp();
        }

        // --- Button Listeners ---
        getStartedButton.onClick.AddListener(GetStartedClicked);
        iAlreadyHaveButton.onClick.AddListener(IAlreadyHaveButtonClicked);
        signInWithGoogleButton.onClick.AddListener(OnSignInWithGoogleClicked);
        signInWithFacebookButton.onClick.AddListener(OnSignInWithFacebookClicked);
        signOutButton.onClick.AddListener(OnSignOutClicked);
        
        // --- Auto Sign-In Logic ---
        statusText.text = "Checking credentials...";
        PlayGamesPlatform.Instance.Authenticate(OnSilentSignInFinished, true);
    }
    
    // --- Facebook SDK Helper Methods ---

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
        // Pause the game time if the Facebook UI is overlaying
        Time.timeScale = isGameShown ? 1 : 0;
    }

    // --- Google Play Games Callbacks ---

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
    
    // CHANGED: Implemented Facebook Login
    private void OnSignInWithFacebookClicked()
    {
        if (!FB.IsInitialized)
        {
            statusText.text = "Facebook SDK not ready. Retrying init...";
            Debug.Log("FB SDK not ready. Calling Init...");
            // Try to init again
            FB.Init(OnInitComplete, OnHideUnity);
            return;
        }

        statusText.text = "Logging in with Facebook...";
        loginButtonsPanel.SetActive(false);
        
        // Request "public_profile" and "email" permissions
        var perms = new List<string>() { "public_profile", "email" };
        FB.LogInWithReadPermissions(perms, OnFacebookLoginComplete);
        // FB.LogInWithReadPermissionsAsync(perms, OnFacebookLoginComplete);
    }

    // NEW: Callback for Facebook Login attempt
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

        // If we get here, login was successful
        if (FB.IsLoggedIn)
        {
            var aToken = AccessToken.CurrentAccessToken;
            Debug.Log($"Facebook User ID: {aToken.UserId}");
            Debug.Log($"Facebook Access Token: {aToken.TokenString}");
            
            // You can now get the user's email/profile info
            FB.API("/me?fields=name,email", HttpMethod.GET, OnFacebookGraphResult);
        }
        else
        {
            Debug.LogWarning("Facebook login reported success, but FB.IsLoggedIn is false.");
            statusText.text = "Facebook login failed.";
            ShowStartPanel();
        }
    }

    // NEW: Callback for getting Facebook user data
    private void OnFacebookGraphResult(IGraphResult result)
    {
        if (result.Error != null)
        {
            Debug.LogError($"Facebook Graph Error: {result.Error}");
            statusText.text = "FB login error (Graph).";
            ShowStartPanel();
            return;
        }

        // Get user's name and email
        string name = result.ResultDictionary.ContainsKey("name") ? result.ResultDictionary["name"].ToString() : "FB User";
        string email = result.ResultDictionary.ContainsKey("email") ? result.ResultDictionary["email"].ToString() : "No Email";

        Debug.Log($"Facebook Name: {name}, Email: {email}");
        statusText.text = $"Signed in as: {name} ({email})";
        ShowGamePanel();
    }
    
    private void OnSignOutClicked()
    {
        statusText.text = "Signing out...";
        
        // Sign out of both services
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
    
    // --- UI Helper Methods ---
    
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
}