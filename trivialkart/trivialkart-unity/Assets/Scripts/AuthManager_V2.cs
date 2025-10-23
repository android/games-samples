using UnityEngine;
using UnityEngine.UI;
using GooglePlayGames;
using GooglePlayGames.BasicApi;
using TMPro;
using Facebook.Unity;
using System.Collections.Generic;

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
        
        statusText.text = "Attempting auto sign-in...";
        
        getStartedButton.onClick.AddListener(GetStartedClicked);
        iAlreadyHaveButton.onClick.AddListener(IAlreadyHaveButtonClicked);
        signInWithGoogleButton.onClick.AddListener(OnSignInWithGoogleClicked);
        signInWithFacebookButton.onClick.AddListener(OnSignInWithFacebookClicked);
        signOutButton.onClick.AddListener(OnSignOutClicked);
        incButton.onClick.AddListener(OnIncButtonClicked);
        
        // --- Facebook SDK Initialization --- ADDED
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
        
        // This attempts the automatic (silent) sign-in
        PlayGamesPlatform.Instance.Authenticate(ProcessAuthentication);
    }

    private void OnIncButtonClicked()
    {
        var currNum = int.Parse(incText.text);
        currNum++;
        incText.text = currNum.ToString();
    }

    private void ProcessAuthentication(SignInStatus status)
    {
        if (status == SignInStatus.Success)
        {
            statusText.text = "PGS Sign-in Successful!";
            // Auto-sign-in worked, now get server auth code
            RequestSpecialScopes();
        }
        else
        {
            // Auto-sign-in failed (e.g., user not signed in, no network, etc.)
            Debug.LogError($"PGS Sign-in failed with status: {status}");
            statusText.text = "Auto sign-in failed. Please sign in.";
            
            // Show the start panel so the user can manually sign in
            ShowStartPanel();
        }
    }

    private void RequestSpecialScopes()
    {
        var scopes = new System.Collections.Generic.List<AuthScope>
        {
            AuthScope.EMAIL,
            AuthScope.PROFILE
        };

        PlayGamesPlatform.Instance.RequestServerSideAccess(
            false,
            scopes,
            (authResponse) =>
            {
                if (authResponse.GetAuthCode() != null)
                {
                    var authCode = authResponse.GetAuthCode();
                    Debug.Log($"Successfully retrieved Server Auth Code: {authCode}");
                    
                    if (authResponse.GetGrantedScopes().Contains(AuthScope.EMAIL))
                    {
                        Debug.Log("Email scope was granted!");
                        // statusText.text = $"Signed in as: {PlayGamesPlatform.Instance.UserEmail}";
                        
                        // Show the main game panel
                        ShowGamePanel();
                    }
                    else
                    {
                        Debug.LogWarning("Email scope was NOT granted.");
                        statusText.text = "Email scope not granted. Please try again.";
                        // Show start panel to allow retry
                        ShowStartPanel();
                    }
                }
                else
                {
                    Debug.LogError("Failed to retrieve Server Auth Code (Inactive Session).");
                    statusText.text = "Failed to get server access. Please try again.";
                    
                    // Show start panel so user can retry the manual sign-in
                    ShowStartPanel();
                }
            });
    }
    
    private void IAlreadyHaveButtonClicked()
    {
        startPanel.SetActive(false);
        loginButtonsPanel.SetActive(true);
    }

    private void GetStartedClicked()
    {
        // This method should trigger the manual sign-in process,
        // which is what shows the account picker popup.
        statusText.text = "Showing account picker...";
        startPanel.SetActive(false); // Hide this panel
        
        // This is the call that shows the popup to select from multiple IDs
        PlayGamesPlatform.Instance.ManuallyAuthenticate(ProcessAuthentication);
    }
    
    private void OnSignInWithGoogleClicked()
    {
        statusText.text = "Signing in with Google...";
        loginButtonsPanel.SetActive(false);
        
        // This also shows the account picker popup
        PlayGamesPlatform.Instance.ManuallyAuthenticate(ProcessAuthentication);
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
    }
    
    private void OnSignOutClicked()
    {
        // This is the crucial call to "unlink" or sign out the user
        statusText.text = "Signing out...";
        // PlayGamesPlatform.Instance.SignOut();
        
        // ADDED: Sign out of Facebook
        if (FB.IsLoggedIn)
        {
            FB.LogOut();
        }
        
        // Now reset the UI to the beginning of the flow
        ShowStartPanel();
    }
    
    // --- Facebook SDK Helper Methods --- ADDED
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
        string name =
 result.ResultDictionary.ContainsKey("name") ? result.ResultDictionary["name"].ToString() : "FB User";
        string email =
 result.ResultDictionary.ContainsKey("email") ? result.ResultDictionary["email"].ToString() : "No Email";

        Debug.Log($"Facebook Name: {name}, Email: {email}");
        statusText.text = $"Signed in as: {name} ({email})";
        ShowGamePanel();
    }
    
    // --- UI Helper Methods --- ADDED
    
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