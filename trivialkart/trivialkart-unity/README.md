# TrivialKart

A sample game demonstrating use of Google Play technologies on Android
with the Unity engine.

This verson of TrivialKart demonstrates:

* In-app purchases through Google Play using the Unity IAP system
* Play Games Services, for signin, achievements, leaderboards, friends and
cloud save
* Play Integrity for receiving integrity signals about device integrity
and Play license status
* The Input SDK for Google Play Games for PC

## Pre-requisites

* Unity 2020 LTS or higher with Android build support (Play plugins are
compatible with earlier versions of Unity, but the TrivialKart project is
built using 2020 LTS)

### Pre-requsities for enabling Google Play features

* Google Play developer account

## Getting started

Open the project in Unity and open the `TrivialKartScene` file
from `Assets/Scenes`.

By default, Google Play featues are disabled. The scene be run in-editor
or exported to device and run. With in-app purchase integration disabled, all
prices are placeholder and purchasing in-game items automatically succeeds.

## Overview

In TrivialKart, the player has a vehicle which they can drive by tapping on it.
Driving the car uses gas. When the car runs out of gas, to continue driving,
more must be purchased using in-game currency. If the player runs out of
in-game currency, they may buy more via in-app purchases.
In-game currency is an example of a consumable purchase that may be repeated.

The game has different cars available for unlock. One car is purchasable using
in-game currency; other cars require unlocking via in-app purchase.
These unlocks are permanent one-time purchases.

The game also has subscriptions available for in-app purchase. One subscription
unlocks a different travel background. A second, more expensive subscription
unlocks the travel background and adds a 40% discount off the cost of purchasing
gas or unlocks using in-game currency. The features of a subscription are only
available if the subscription is active.

## Enabling in-app purchases

### Creating a project in the Google Play Developer Console

1. Go to the
[Google Play Developer Console](https://play.google.com/apps/publish)
and create a new application.
2. Select the `Monetization setup` page for your new application.
Copy the Base64-encoded public key text in the `Licensing` area. You will need
to use this key when configuring Unity IAP.

### Updating the package name in Unity

1. In Unity, if you haven't already, select `File -> Build Settings...`
from the Unity menu bar and switch the platform to `Android`.
2. From the `Build Settings` window, select the `Player Settings...` button.
3. In the `Player Settings` window, expand the `Other Settings` section and
enter the application package identifer you specified in the Play Console in
the `Package Name` field.

### Enabling Unity IAP

Follow the instructions at
[Setting up Unity IAP](https://docs.unity3d.com/2020.3/Documentation/Manual/UnityIAPSettingUp.html)

This sample was tested using version 4.1.2 of the Unity In App Purchasing
package installed through the Package Manager. We recommend this as a minimum
version for this sample. Use of earlier versions, or the older version of
Unity IAP available on the Unity Asset Store is not supported.

### Setting up receipt obfuscation

**NOTE** If the Services menu does not appear in your Unity menu bar, try
uninstalling and reinstalling the In App Purchasing package from the
Unity Package Manager. 

1. Select `Services -> Unity IAP -> Receipt Validation Obfuscation` from the Unity
menu bar.
2. Paste the Base64-encoded public key you retrieved from the Google Play
Developer console into the text box.
3. Click the `Obfuscate Google Play License Key` button.
4. Close the popup window.

### Turning on IAP in the project

1. From the Unity menu bar, select
`TrivialKart -> BuildOptions -> Build with IAP`

This menu item acts as a global toggle, if unchecked `NO_IAP` is defined in
the `Scripting Define Symbols`. If checked `USE_IAP` is defined in the
`Scripting Define Symbols`.

### Building and uploading a signed build

1. In the Unity `Build Settings` window, make sure the
`TrivialKart/Scenes/playScene` is added to the list of `Scenes In Build`.
2. Configure the project for signing by creating a new key store at
`File > Build Settings > Player Settings > Publishing Settings`. Additional
instructions on configurating signing are available.
[on the Unity site](https://answers.unity.com/questions/326812/signing-android-application.html).
3. Build an App Bundle.
4. Use the Google Play Developer Console to upload your build to Google Play.
You can upload to the Internal test track.

### Setting up the purchase items in Google Play

*Note:* you must upload a build to Google Play to be able to create your
in-app purchasing items in the Google Play Developer Console Console.

1. Return to the Google Play Developer Console.
2. Under `Monetize -> Products -> In-app products`, create the following
in-app products:
      | Product ID   |  Price|
      | :---:        | :---: |
      | car_offroad  | $2.99 |
      | car_kart     | $4.99 |
      | five_coins   | $0.99 |
      | ten_coins    | $1.99 |
      |twenty_coints | $2.49 |
      |fifty_coins   | $4.99 |

3. Under `Monetize -> Products -> Subscriptions`, create subscriptions with
these IDs and prices (Fill out the other fields. Set them to "Active"):
     | Product ID   |  Price|
     | :---:        | :---: |
     | silver_subscription   | $1.99 |
     | golden_subscription    | $4.99 |

4. Publish your build to the testing channel. It may take up to a few hours to
process the build. Running a build before processing complete can result in
errors such as Google Play reporting that "this version of the application is
not enabled for in-app billing".
5. Add tester accounts to your game. This will allow you to test purchases and
subscriptions without being charged. Test accounts also have greatly reduced
subscription periods, allowing for easier testing of subscription features.

## Enabling Play Games Services

### Enable Play Games Services in Play Console

Follow the
[instructions](https://developers.google.com/games/services/console/enabling)
for setting up Play Games Services for your app in the Play Console.

You will also need to
[enable save games](https://developers.google.com/games/services/console/configuring#enabling_saved_games)
since TrivialKart uses the Cloud Save feature.

### Defining the leaderboard and achievements

In the Play Games Services section of your Play Console entry
for your app, access the **Achievements** and **Leaderboard**
sections to add the following achievements and leaderboard:

**Achievements**
Name: `tk_achievement_drive`
Description: `Drive a while`
Initial State: `Revealed`
Incremental: `Unchecked`
Points: `5`

Name: `tk_achievement_truck`
Description: `Unlock the truck`
Initial State: `Revealed`
Incremental: `Unchecked`
Points: `5`

**Leaderboards**
Name: `tk_leaderboard_distance`
Format: `Number`
Number of decimals `2`
Sort order: `Largest first`

Make sure to publish your achievements and leaderboard after creation.

### Setup the Unity plugin

1. Download version 11.0 or later of the Play Games Services for Unity plugin
from its [GitHub releases page](https://github.com/playgameservices/play-games-plugin-for-unity/releases).
2. Extract the `.zip` file.
3. Install the `.unitypackage` file located in the `current-build` directory of
the extracted archive using `Assets > Import Package > Custom Package`.

### Generate Google Play Games constants

1. From the Play Console entry for your app, select `Play Games Services -> Setup and management -> Achievements`.
2. Find the **Get resources** button and click it.
3. Copy the text from the `Android (XML)` tab.
4. From the Unity menu bar, select `Window -> Google Play Games -> Setup -> Android Setup...`.
5. Paste the resource text into the **Resources Definition** field.
6. Click the **Setup** button


### Configure Proguard

1. Go to **File > Build Settings > Player Settings** and click **Publishing Settings** section.
2. Choose **Proguard for Minify > Release**. Then, enable User Proguard File.
3. If you want the plugin to be proguarded for debug apks as well, you can repeat for **Proguard for Minify > Debug.**
4. Copy the contents of the proguard configuration file at
`Assets/GooglePlaygames/com.google.play.games/Proguard/games.txt` into `Assets/Plugins/Android/proguard-user.txt`.

### Enable in the Play Games Services feature in the Unity Editor

From the Unity menu bar, select
`TrivialKart -> BuildOptions -> Build with Google Play Games Services`

### Adding the upload key for local build testing

Builds published for testing or distribution on Google Play are resigned
by Google. When you configure Play Games Services, these keys are used for
authentication. If you run a locally built APK signed with your upload key,
Play Games Services won't be able to authenticate. To fix this, you can
add the upload key to a new OAuth client for your local builds. To do this:

1. In Play Console, go to **Setup->App Integrity**
2. Select the **App Signing** tab
3. Find the **Upload key certificate** section and copy the **SHA-1 certificate fingerprint**
4. Go to **Grow->Play Games Services->Setup** and **Management->Configuration**
5. Find the **Credentials** section and click on **Add Credentials**
6. Give the credential a different name from the existing name
7. Click **Create OAuth Client** and follow the instructions, using the SHA-1 certificate
fingerprint from the Upload key
8. Click **Refresh OAuth** clients until the new client is listed in the **OAuth client list**
9. Select the new client from the list and click **Save changes**
10. Return to the **Configuration** screen and click **Review and publish**
11. On the next screen, click **Publish**

## Enabling Play Integrity

### Play Console setup

See the [Play Integrity guide](https://developer.android.com/google/play/integrity/setup)
for information on configuring Play Integrity in the Google Play Console.

### Downloading the plugin

You will need to download the `unitypackage` file for Play Integrity from the
[Google Play Unity plugins releases page](https://github.com/google/play-unity-plugins/releases).

After downloading the plugin, install it using using `Assets > Import Package > Custom Package`.

### Server specifications

The Play Integrity code requires communication with a backend server to:

* Generate a nonce
* Decrypt the integrity verdict

You can find implementation details for these tasks in the
[Work with integrity verdicts](https://developer.android.com/google/play/integrity/verdict) guide.

A server implementation is not included with this sample. You can build and
deploy a server using the framework and platform of your choice. TrivialKart expects the
server to define two endpoints accessed via HTTP requests:

A `getNonce` endpoint, accessed via a GET request. This URL returns a json payload containing
the nonce value. The JSON payload containts a single key/value pair with the key being `nonce`.

A `processToken` endpoint, accessed via a POST request. The URL expects a JSON payload containing
the encrypted integrity token returned by the Play Integrity plugin. The JSON payload
contains a single key/value pair with the key being `tokenString`. The POST request returns a
JSON payload containing the integrity verdict information. This is serialized into the structures
defined in the `IntegrityVerdict.cs` source file.

### Configuring server URLs

You will need to customize the following lines in `PlayIntegrityController.cs`
to point to the URLs for your server:

`private readonly string URL_GETNONCE = "https://your-play-integrity-server.com/getNonce";`
`private readonly string URL_PROCESSTOKEN = "https://your-play-integrity-server.com/processToken";`

### Enable the Play Integrity feature in the Unity Editor

From the Unity menu bar, select
`TrivialKart -> BuildOptions -> Build with Play Integrity`

### Verdict display in game

An integrity verdict is requested when opening the store page. A verdict will only be requested
once per play session. The summary results string is displayed near the top of the store page.

Note that in a real game, you would not directly return the decrypted verdict information,
but communicate the results in a way that makes sense for your game. For simplicity, this
sample expects the raw verdict json.

## Building for Google Play Games

1. Enable x86 ABI architecture. This can be activated using `Player Settings > Other Settings > Target Architectures` and enabling both x86 (Chrome OS) and x86-64 (Chrome OS).
*Note:* You can only enable x86 support when using the IL2CPP Scripting Backend. This can be done from `Player Settings > Configuration > Scripting Backend > IL2CPP`.
2. Disable unsupported android features and permissions. This can be done by adding custom build logic to your `AndroidManifest.xml`. To do this, open `Player Settings > Publishing Settings` and find the `Build` section. Then, enable the `Custom Main Manifest` or corresponding manifest for the feature or permission you need to disable.
At this point, you can find that manifest and add `android:required="false"` to the `<uses-feature>` or `uses-permission` declaration for all features that Google Play Games does not support.
For more information, see the [Unity documention on Android Manifests](https://docs.unity3d.com/Manual/android-manifest.html).
You should also disable any lines requesting permissions in your Unity code. See the Unity documentation on [requestion permissions](https://docs.unity3d.com/Manual/android-RequestingPermissions.html) for more details.
3. Install the Input SDK Unity package using `Assets > Import Package > Custom Package`.
4. From the Unity menu bar, select
`TrivialKart > BuildOptions > Build for Google Play Games PC` to enable the
appropriate scripting defines.
5. Create a build using the normal Unity build process.

## Support

If you've found any errors or bugs in this sample game, please
[file an issue](https://github.com/android/games-samples/issues).

This is not an officially supported Google product.

### Build version workarounds

If you forget to select the version you want to build before closing Unity, you may be unable to see the `TrivialKart > BuildOptions` menu.
To fix it, delete the directives under `Edit > Project Settings > Player > Other Settings > Script Compilation` and restart Unity.

## Further reading

* [Unity IAP](https://docs.unity3d.com/Manual/UnityIAP.html)

## CHANGELOG

2022-04-01: 1.1.0 - Added Play Games Services, Play Integrity and Google
Play Games for PC features.
2022-01-27: 1.0.0 - Initial release.

## Graphical asset credits and license

The PNG and font graphic asset files used in TrivalKart are from
[Kenney's game assets](https://kenney.nl/). These files are licensed under
a [Creative Commons Zero](https://creativecommons.org/publicdomain/zero/1.0/)
license.
