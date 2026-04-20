# TrivialKart Unreal

A sample game demonstrating the use of Google Play technologies on Android with **Unreal Engine**.

This version of TrivialKart demonstrates:

* **Play Games Services (v2)**: For sign-in, achievements.
* **In-App Purchases (IAP)**.

---

## Pre-requisites

* **Unreal Engine**: Version 5.4 or higher (Recommended).
* **Android Studio**: Properly configured for the specific version of Unreal Engine being used.
* **Google Play Games Unreal Plugin**: Ensure the official plugin is enabled.

### Pre-requisites for enabling Google Play features
* Google Play developer account.
* A configured project in the [Google Play Console](https://play.google.com/apps/publish).

---

## Getting started

1.  Clone the repository and navigate to `trivialkart/trivialkart_unreal`.
2.  Right-click `TrivialKart.uproject` and select **Generate Visual Studio project files**.
3.  Open the `.sln` file in Visual Studio (or the `.uproject` in Unreal Editor).
4.  Open the `L_TrivialKart` found in `Content/Maps`.

By default, Google Play features are simulated or disabled unless the Google Play plugin is active and configured with a valid App ID.

---

## Overview

In TrivialKart, the player has a vehicle which they can drive by tapping on it.
Driving the car uses gas. When the car runs out of gas, to continue driving,
more must be purchased using in-game currency. If the player runs out of
in-game currency, they may buy more via in-app purchases.
In-game currency is an example of a consumable purchase that may be repeated.

The game has different cars available for unlock. One car is purchasable using
in-game currency; other cars require unlocking via in-app purchase.
These unlocks are permanent one-time purchases.

The game features:
* **Consumable Purchases**: Coin packs.

---

## Enabling In-App Purchases

### 1. Google Play Console Setup
1. Go to the [Google Play Developer Console](https://play.google.com/apps/publish)
and create a new application.
2. Navigate to `Monetise with Play -> Monetisation Setup`
Copy the Base64-encoded public key text in the `Licensing` area. You will need
to use this key when configuring IAP.

### 2. Project Configuration
1. In Unreal Editor, go to **Project Settings > Platforms > Android**.
2. Under **Google Play Services**:
    * Enable **Support Google Play Services**.
    * Copy your **License Key** from the Play Console into the **Google Play License Key** field.
3. Under **Advanced APK Packaging**, ensure your **Package Name** matches the Console.

### Setting up the purchase items in Google Play

*Note:* you must upload a build to Google Play to be able to create your
in-app purchasing items in the Google Play Developer Console Console.

1. Return to the Google Play Developer Console.
2. Under `Monetize -> Products -> In-app products`, create the following
in-app products:
      | Product ID   |  Price|
      | :---:        | :---: |
      | five_coins   | $0.99 |
      | ten_coins    | $1.99 |
      |twenty_coints | $2.49 |
      |fifty_coins   | $4.99 |

4. Publish your build to the testing channel. It may take up to a few hours to
process the build. Running a build before processing complete can result in
errors such as Google Play reporting that "this version of the application is
not enabled for in-app billing".
5. Add tester accounts to your game. This will allow you to test purchases and
subscriptions without being charged. Test accounts also have greatly reduced
subscription periods, allowing for easier testing of subscription features.

---

## Enabling Play Games Services

### 1. Set Up LogIn Credentioals
1. Go to the [Google Play Developer Console](https://play.google.com/apps/publish)
and create a new application.
2. Navigate to `Grow Users -> Play Games Services -> Setup and Management -> Configurations`
3. Generate a Game server credentials by making a credential of type Web Application.
5. Go to **Google Play Services** in **Unreal Engine** via `Project Settings -> Android -> Google Play Services`
6. Copy your **Crednetials** from the Play Console into the **Google Play Games Oauth Client ID Register for Game Server** field.



### 2. Define Achievements
In the Play Games Services section of your Play Console entry
for your app, access the **Achievements** sections 
to add the following achievements:

**Achievements**
Name: `tk_achievement_drive`
Description: `Drive a while`
Initial State: `Revealed`
Incremental: `Unchecked`
Points: `5`

Name: `tk_achievement_fuel`
Description: `Empty the fuel tank`
Initial State: `Revealed`
Incremental: `Unchecked`
Points: `5`

Name: `tk_achievement_login`
Description: `LogIn 5 times`
Initial State: `Revealed`
Incremental: `Checked`
Points: `5`

---

## Support

If you've found any errors or bugs in this sample game, please
[file an issue](https://github.com/android/games-samples/issues).

---

## Graphical Asset Credits
Assets provided by [Kenney.nl](https://kenney.nl/) under [Creative Commons Zero](https://creativecommons.org/publicdomain/zero/1.0/).
