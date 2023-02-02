# TrivialKart Godot game sample

## Overview

TrivialKart demonstrates integration and use of Google Play
libraries and services. The current version implements in-app purchases
and subscriptions using the Google Play Billing Library.

## Getting started

The project was built in Godot Engine 3.4. To try it out in the editor,
open and run `Main.tscn`. Purchases will automatically succeed.

The project can be exported for Android and run on device with or without
the Google Play Billing Library plugin installed. If the plugin is not
installed, behavior will match the editor: purchases automatically succeed.

## Setting up the plugin

### Obtaining the plugin

The Google Play Billing Library plugin for Godot is available on the
[Play Billing Llibrary plugin GitHub repo](https://github.com/godotengine/godot-google-play-billing)

**NOTE**: The version of the plugin currently available on the releases page
is 1.0.1. The minimum version required by TrivialKart is 1.1.2. To build 1.1.2,
clone the plugin repo and follow the build instructions in the README.

Installing the plugin requires the following files:

`GodotGooglePlayBilling.1.1.2.release.aar`

`GodotGooglePlayBilling.gdap`

### Installing the plugin

1. Make sure [Android custom builds](https://docs.godotengine.org/en/stable/getting_started/workflow/export/android_custom_build.html#doc-android-custom-build)
are enabled for the project and the Android build template for your version
of Godot is installed.
2. Download and copy the `godot-lib-$VERSION-release.aar` file for your version
of Godot into `res://android/build/plugins`.
3. Install the GodotGooglePlayBillng plugin files into
`res://android/build/plugins`.
4. Ensure the `Godot Google Play Billing` plugin is enabled in the Export settings.

## Setting up in-app purchases

### Exporting the app

You will need to create an app entry in the Google Play Developer Console
and upload a build in order to configure in-app purchasing.

1. Go to the [Google Play Developer Console](https://play.google.com/apps/publish)
and create a new application.
2. Generate an [upload key and keystore](https://developer.android.com/studio/publish/app-signing#generate-key).
3. Configure the Godot project with the new package name and signing key and
export an app bundle. For more information on exporting Godot projects to
Android, see [this guide](https://developer.android.com/games/engines/godot/godot-export)
4. Upload the app bundle to Google Play. You will probably want to upload
to an internal test track.

### Setting up the purchase items in Google Play

1. Return to the Google Play Developer Console.
2. Under `Monetize -> Products -> In-app products`, create the following
in-app products:
      | Product ID    |  Price|
      | :---:         | :---: |
      | car_offroad   | $2.99 |
      | car_kart      | $4.99 |
      | five_coins    | $0.99 |
      | ten_coins     | $1.99 |
      | twenty_coints | $2.49 |
      | fifty_coins   | $4.99 |

3. Under `Monetize -> Products -> Subscriptions`, create subscriptions with
these IDs and prices (Fill out the other fields. Set them to "Active"):
     | Product ID   |  Price|
     | :---:        | :---: |
     | silver_subscription   | $1.99 |
     | golden_subscription    | $4.99 |

4. Publish your build to the testing channel. It may take up to a few hours to
process the build. Running a build before processing complete can result in
errors such as Google Play reporting that "this version of the application is not enabled for in-app billing".
5. Add tester accounts to your game. This will allow you to test purchases
and subscriptions without being charged. Test accounts also have greatly reduced
subscription periods, allowing for easier testing of subscription features.

## Release history

12/10/2021 - 1.0.0 - Initial release.

## Graphical asset credits and license

The PNG and font graphic asset files used in TrivalKart are from
[Kenney's game assets](https://kenney.nl/). These files are licensed under
a [Creative Commons Zero](https://creativecommons.org/publicdomain/zero/1.0/) license.
