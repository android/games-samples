# **OpenID Account Linking Sample: PGS v1 to v2 Migration**

This sample project demonstrates how to implement a robust account linking system that survives the migration from **Google Play Games Services (PGS) v1** to **v2**.

It specifically addresses the scenario where you need to authenticate users against your own backend server while upgrading your Unity client from the deprecated v1 authentication flow to the new v2 flow.

## **🔑 The Core Concept: OpenID Consistency**

The most critical challenge in migrating from PGS v1 to v2 is ensuring that players do not lose their account progress. This sample solves this by relying on the **OpenID Connect `sub` (Subject) claim**.

### **How it works:**

1.  **In PGS v1**: The client obtains an **ID Token** directly and sends it to the server. The server verifies it and extracts the `sub` (OpenID).
2.  **In PGS v2**: The client obtains a **Server Auth Code**, sends it to the server, and the server exchanges it for an ID Token. The server extracts the `sub` (OpenID).

**Crucially, the `sub` value for a specific Google user is identical in both scenarios.**

By using this `sub` value as the key in your database (e.g., `google-123456789`), a user can start playing on v1, upgrade the app to v2, and your server will recognize them as the exact same user.

## **📂 Files Overview**

* **[server.js](/trivialkart/trivialkart-auth-server/server.js)**: A Node.js/Express backend.
    * **Endpoint `/verify_and_link_google`**: Handles the v1 flow (verifies ID Token directly).
    * **Endpoint `/exchange_authcode_and_link`**: Handles the v2 flow (exchanges Auth Code for Token).
    * **Logic**: Both endpoints resolve to the same `userDatabase` entry based on the `sub` claim.
* **[AuthManager.cs](/trivialkart/trivialkart-unity/Assets/Scripts/AuthManager.cs)**: A Unity C# script that manages the UI and Authentication logic.
    * Uses Preprocessor Directives (`#if PGS_V1` / `#if PGS_V2`) to compile different logic for the different SDK versions.

## **🛠️ Prerequisites**

* **Unity 2020.3+**
* **Node.js** (v14+)
* **Google Cloud Console Project** with OAuth 2.0 Web Client Credentials.
* **Facebook App** (Optional: only required if testing Facebook linking features).

## **🚀 Backend Setup**

1.  **Install Dependencies**:

    ```bash
    npm install express google-auth-library jsonwebtoken dotenv axios
    ```

2.  **Configure Environment**: Create a `.env` file in the same directory as `server.js`:

    ```env
    WEB_CLIENT_ID=your-google-web-client-id.apps.googleusercontent.com
    WEB_CLIENT_SECRET=your-google-web-client-secret
    FB_APP_ID=your-facebook-app-id
    FB_APP_SECRET=your-facebook-app-secret
    JWT_SECRET=super-secret-key-for-your-game-jwt
    ```

3.  **Note:** The `WEB_CLIENT_ID` must be the "Web application" Client ID from Google Cloud Console, **not** the Android Client ID.
4.  **Run the Server**:

    ```bash
    node server.js
    ```

## **🎮 Unity Client Setup**

This sample uses **Scripting Define Symbols** to switch between the v1 and v2 implementation without changing the code manually.

### **🧹 Clean Removal (Important)**

**⚠️ Backup your project before proceeding.**

When switching between versions (e.g., v1 to v2) or if you encounter dependency resolution errors, you **must** cleanly remove the installed plugin artifacts before importing the new package.

Delete the following folders from your Unity project's `Assets` directory:

1.  `Assets/ExternalDependencyManager`
2.  `Assets/GeneratedLocalRepo`
3.  `Assets/GooglePlayGames`

*(Note: You may also need to check `Assets/Plugins` for residual Android/iOS libraries if errors persist.)*

### **Dependencies**

* **Common**
    * **[Facebook SDK for Unity](https://developers.facebook.com/docs/unity/downloads)** (Optional)
* **For v1**:
    * **[Google Play Games Plugin for Unity **v0.10.15**](https://github.com/playgameservices/play-games-plugin-for-unity/releases)**.
* **For v2**:
    * **[Google Play Games Plugin for Unity **v2.1.0**](https://github.com/playgameservices/play-games-plugin-for-unity/releases)**.
    * **[Google Sign-In Unity Plugin](https://github.com/googlesamples/google-signin-unity/releases)**.
      *(Note: PGS v2 removes authentication methods, so the separate Google Sign-In plugin is required to retrieve Auth Codes.)*

### **Configuration Steps**

1.  **Working scene**: Open the **[MockSignIn](/trivialkart/trivialkart-unity/Assets/Scenes/MockSignIn.unity)** Scene
2.  **Allow HTTP Downloads:** In your **Player Settings** (*Edit > Project Settings > Player*), find the "Other Settings" tab. Ensure **Allow downloads over HTTP** is set to **Allowed in development builds**.
    * *Note: This is only for testing with a local `http://` server. Production apps must use `https://`.*
3.  **Set Server URL**: Find the `AuthManager` GameObject in the scene. Set the `Server Url` field in the Inspector to your local IP (e.g., `http://192.168.1.5:3000`).

### **Switching Versions**

To switch the active code path, go to **Project Settings > Player > Other Settings > Scripting Define Symbols**.

#### **Mode A: Simulating PGS v1 (Legacy)**

Add the symbol: `PGS_V1`

* **Behavior**: The script uses `PlayGamesPlatform.Instance.GetIdToken()` to get a token.
* **Server Endpoint**: Calls `/verify_and_link_google`.

#### **Mode B: Simulating PGS v2 (Migration Target)**

Add the symbol: `PGS_V2`

* **Behavior**: The script uses `GoogleSignIn.DefaultInstance.SignIn()` to get an **Auth Code**.
* **Server Endpoint**: Calls `/exchange_authcode_and_link`.

## **🧪 Testing the Migration**

1.  **Start with PGS_V1**:
    * Build the app with the `PGS_V1` symbol.
    * (Use "Development Build" to enable the HTTP permissions set earlier).
    * Login with Google.
    * Note the "In-Game ID" displayed (e.g., `ingame-1001`).
    * Increment the counter to save some data.
2.  **Upgrade to PGS_V2**:
    * Change the symbol to `PGS_V2`.
    * (Ensure you have the Google Sign-In plugin installed and older PGS v1 files removed).
    * Build and run the app again (simulating an app update).
    * Login with Google.
3.  **Verify**:
    * The server logs will show `(PGS v2) Verified... Existing user linked to OpenID`.
    * The client should receive the **exact same** In-Game ID (`ingame-1001`) and your previous count.

## **⚠️ Important Note on Client IDs**

For the **v2 Flow (Auth Code)** to work, you must configure the Google Sign-In plugin in Unity with your **Web Client ID**, not your Android Client ID.

```csharp
// Inside AuthManager.cs (v2 block)
GoogleSignIn.Configuration = new GoogleSignInConfiguration
{
    WebClientId = "YOUR_WEB_CLIENT_ID_HERE", // Must match .env WEB_CLIENT_ID exactly
    RequestAuthCode = true, // Critical for v2 backend linking
    // ...
};