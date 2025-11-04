# PGS Recall API with a Unity Client & Node.js Server Demo

This guide will walk you through setting up a complete, end-to-end demonstration of the Google Play Games Services (PGS) Recall API. It has been updated to include a persistent progress-saving mechanism. The goal is to show how a player can start playing on one device, save their progress in near real-time, and then seamlessly continue with that exact progress on a new device without needing a traditional login screen. We'll use a local Node.js server to act as your game's backend and a Unity client to simulate the game.

Note : This example uses the server to store and retrieve data to demonstrate the Recall API's functionality, and is not intended to replace Cloud Save. You can find more information about Cloud Save and the Recall API here:

* **[Cloud Save](https://developer.android.com/games/pgs/savedgames)**
* **[Recall](https://developer.android.com/games/pgs/recall)**
 
## Repo

* **[Unity Client](/trivialkart/trivialkart-unity)**
* **[Local Server](/trivialkart/trivialkart-recall-server)**

## 📋 Prerequisites

Before you begin, make sure you have the following:

* **Node.js and npm:** Installed on your computer to run the backend server.
* **Unity Hub and Unity Editor:** To run the game client.
* **Google Cloud Project:** A project with billing enabled and the Google Play Games Services API enabled.
* **Two Android Devices:** Both logged into Google Play accounts for testing.
* **A configured PGS Project:** Your game should be set up in the Google Play Console with the Recall API enabled for your project.
* **Shared Wi-Fi Network:** Your computer (running the server) and both Android devices must be connected to the same local network.

## 💻 Part 1: Backend Server Setup

First, we'll configure and launch the Node.js server. This server will handle the logic for creating and retrieving player accounts linked via the Recall API.

### Step 1: Configure Google Cloud Service Account

The server needs credentials to securely communicate with Google's APIs.

1.  **Go to the Google Cloud Console:** Navigate to "IAM & Admin" \> "Service Accounts".
2.  **Create Service Account:**
    * Click "+ CREATE SERVICE ACCOUNT".
    * Give it a name (e.g., `recall-api-server`).
    * Click "CREATE AND CONTINUE". For permissions, granting a specific role like **"Play Games Services Admin"** is a good practice, though for the purposes of this demo, simply having the API enabled in your project may be sufficient.
    * Click "Done".
3.  **Generate a Key:**
    * Find your newly created service account in the list and click on it.
    * Go to the **"KEYS"** tab.
    * Click "ADD KEY" \> "Create new key".
    * Select **JSON** as the key type and click "CREATE".
    * A JSON file will be downloaded. **Treat this file like a password\!**

### Step 2: Set Up the Server Project

1.  **Create a Project Folder:** Create a new folder on your computer named `trivialkart-recall-server`.

2.  **Add Server File:** Place the provided `server.js` file inside this folder.

3.  **Add Key File:** Move the downloaded JSON key file into the `trivialkart-recall-server` folder.

4.  **Create `.env` file:** In the same folder, create a new file named `.env`. This file will store your secret key path. Open it and add the following line, replacing `your-key-file-name.json` with the actual name of your key file:

    ``` 
    KEY_FILE_PATH='your-key-file-name.json'
    ```

5.  **Install Dependencies:** Open a terminal or command prompt, navigate into your `trivialkart-recall-server` folder, and run:

    ``` bash
    npm install express cors google-auth-library axios uuid dotenv
    ```

    This command downloads all the necessary packages for the server to run.

### Step 3: Start the Server

Now, you're ready to launch the server.

1.  In your terminal, while still in the project folder, run the command:

    ``` bash
    npm start
    ```

2.  If successful, you should see output similar to this:

    ``` 
    Successfully obtained Google API access token.
    Server listening at http://localhost:3000
    ```

    Your backend is now live and waiting for requests from your Unity game\! ✅

## 🎮 Part 2: Unity Client Setup

Next, we'll configure the Unity project to communicate with our local server.

### Step 1: Find Your Local IP Address

Your mobile devices need a specific IP address to find the server on your network.

* **On macOS:** Open a terminal and run `ipconfig getifaddr en0`. (en0 is typically the Wi-Fi interface on modern Macs).
    * Alternatively: `networksetup -getinfo Wi-Fi | grep 'IP address:'`
* **On Windows:** Open Command Prompt and run `ipconfig`. Look for the "IPv4 Address" under your active Wi-Fi or Ethernet adapter.

### Step 2: Configure the Unity Script

1.  **Add the Script:** Place the `PGSRecallManager.cs` file into your Unity project's `Scripts` folder.

2.  **Update the Server URL:** Open the script and find the `SERVER_BASE_URL` constant. **Replace** the placeholder IP with the one you found in the previous step, making sure to keep the `:3000` port.

    ``` csharp
    private const string SERVER_BASE_URL = "http://YOUR_COMPUTER_IP:3000";
    ```

3.  **Enable UI:**

    * Enable ‘DummyLoginPanel’ `gameObject`
    * Uncomment ‘RecallManager.TryRestorePlayerSession() in PGSController

### Step 3: Configure Build Settings for Local Testing

For your Unity client to communicate with the local `http://` server, you need to configure two settings that work together:

1.  **Enable Development Build:** In your **Build Settings** (File \> Build Settings...), check the **Development Build** box.
2.  **Allow HTTP Downloads:** In your **Player Settings** (Edit \> Project Settings \> Player), find the "Other Settings" tab. Make sure that the **Allow downloads over HTTP**\* option is set to **Allowed in development builds**. This setting, which is usually the default, is activated when you enable a development build.

**Note**: *These settings are only necessary for testing with a local, non-secure (**http://**) server. A live game should always use a secure production server with **https://**, which does not require these changes.*

## 📱 Part 3: Running the Demo

Now for the exciting part\! This flow is based on the provided code which uses a dummy login for automatic account creation.

### Device 1: Creating the Account & Saving Progress

1.  **Build and Run:** Build your Unity project and run it on your first Android device (Phone 1).

2.  **Login to Create Account:** Enter a username and press your "Login" button.

3.  **Check Server Logs:** Your Node.js server will log the account creation process, link a new token with Google, save the initial player data, and respond with `AccountCreated`.

    ``` 
    Received a request on /create-account
    Attempting to link persona '...'...
    Successfully obtained Google API access token.
    Successfully linked persona '...'.
    Successfully created and linked account for YourUsername.
    ```

4.  **Play the Game & Sync Progress:** On Phone 1, play the game to increase the `distanceTraveled` value stored in `PlayerPrefs`. Watch the server logs again. Every 5 seconds, the Unity client will check for changes. When it detects one, it will automatically call the `/update-progress` endpoint. You will see new logs like this:

    ``` 
    Received a request on /update-progress
    Updated distance for YourUsername to 123.45
    ```

### Device 2: Recalling the Account with Synced Progress

1.  **Build and Run:** Now, run the same Unity project on your second Android device (Phone 2).

2.  **Authenticate with PGS:** Trigger the real PGS login, which should call the `TryRestorePlayerSession` method.

3.  **Observe the Magic ✨:** The client will send the `recallSessionId` to the `/recall-session` endpoint. The server will find the token linked on Phone 1, look it up in its database, and find the player data, including the **most recently updated `distanceTraveled` value**.

    ``` 
    Received a request on /recall-session
    Successfully obtained Google API access token.
    Found recall token: ...
    Player found in database. Sending data to client.
    ```

4.  **Progress Restored:** Your Unity client on Phone 2 receives the `AccountFound` status and the latest player data. The UI will update, and the local `PlayerPrefs` will be populated with the progress that was just saved from Phone 1. 
5. You have now seamlessly restored the latest game state without a login screen\!
