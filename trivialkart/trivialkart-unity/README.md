# TrivialKart

A sample game demonstrating use of Google Play technologies on Android
with the Unity engine.

This verson of TrivialKart demonstrates in-app purchases through Google Play
using the Unity IAP system.

## Pre-requisites

- Unity 2019 LTS or higher with Android build support

### Pre-requsities for enabling IAP

- Google Play developer account

## Getting started

Open the project in Unity and open the `TrivialKartScene` file
from `Assets/Scenes`.

By default, in-app purchase integration is disabled. The scene be run in-editor
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

1. Select `Window -> Unity IAP -> Receipt Validation Obfuscation` from the Unity
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

## Building for Google Play Games

TODO: Verify setup instructions across supported Unity versions and frame pacing guidance.

You need to enable x86 ABI architecture. This can be activated using `Player Settings > Other Settings > Target Architectures` and enabling both x86 (Chrome OS) and x86-64 (Chrome OS).

*Note:* You can only enable x86 support when using the IL2CPP Scripting Backend. This can be done from `Player Settings > Configuration > Scripting Backend > IL2CPP`.

## Support

If you've found any errors or bugs in this sample game, please
[file an issue](https://github.com/android/games-samples/issues).

This is not an officially supported Google product.

## Further reading

- [Unity IAP](https://docs.unity3d.com/Manual/UnityIAP.html)

## CHANGELOG

2022-01-27: 1.0.0 - Initial release.

## Graphical asset credits and license

The PNG and font graphic asset files used in TrivalKart are from
[Kenney's game assets](https://kenney.nl/). These files are licensed under
a [Creative Commons Zero](https://creativecommons.org/publicdomain/zero/1.0/)
license.
