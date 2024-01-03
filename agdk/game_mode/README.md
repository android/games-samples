## Game Mode API Sample

This is a basic sample that demonstrates the use of the [Game Mode API](https://developer.android.com/games/gamemode/gamemode-api). With the Game Mode API, an application can retrieve the user's optimization preference for the game and act accordingly.

The sample will adjust behavior according to user selected Game Mode:

*   [STANDARD](https://developer.android.com/reference/android/app/GameManager#GAME_MODE_STANDARD): run the game at 30 fps and half the device’s maximum resolution
*   [PERFORMANCE](https://developer.android.com/reference/android/app/GameManager#GAME_MODE_PERFORMANCE): run the game at 60 fps and maximum resolution
*   [BATTERY](https://developer.android.com/reference/android/app/GameManager#GAME_MODE_BATTERY): run the game at 30 fps and a quarter of the maximum resolution
*   [UNSUPPORTED](https://developer.android.com/reference/android/app/GameManager#GAME_MODE_UNSUPPORTED): this sample treats it the same way as STANDARD (you may opt to differ)

Note that this sample is just demonstrating the easiest and most distinguishable way to optimize for either PERFORMANCE or BATTERY mode. Each game may have more ways of optimization and may further differentiate between the modes such as loading different assets, online/offline mode (minimizing network calls), etc.

## Prerequisites

Before building in Android Studio (2022.3 or higher), the following prerequisites must be met:

### Requirements

Minimum API level of 33 (Android 13) and supported devices (such as Pixel 6~) are required for the Game Mode API. This sample is set to a minApi of 24 as it
demonstrates how to check for availability of the Game Mode API. It will run on earlier versions of Android, but none of the
game mode functionality will be available.

### 3rd Party Libraries

This sample utilizes 3rd party libraries such as Dear Imgui and Bullet physics. Follow these steps to setup the required libraries:

1. Open a terminal and set the working directory to `agdk/third_party/`
2. Run: `git clone https://github.com/ocornut/imgui`
3. Run: `git clone https://github.com/bulletphysics/bullet3`

## Building

Once the prerequisites are complete, open the folder in Android Studio 2021.2 or higher. You can then build and run the sample from Android Studio

## Running

To switch between the game modes, you can use the Game Dashboard (Available on Pixel devices) or similar applications provided by other OEMs (such as Game Space or Game Booster).

If you are using Game Dashboard and the icon is not showing upon launching the game, you may need to upload your application to Google Play Console and install it via Play Store (you can start an [internal test track](https://support.google.com/googleplay/android-developer/answer/9844679?hl=en) and allowlist your own account to access it)

![Game Dashboard Activity](/agdk/game_mode/docs/gamedashboardactivity.png?raw=true "Game Dashboard Activity")

During development, if you are using device without Game Dashboard and the manufacturer do not provide any way to set Game Mode for each application, you can change the Game Mode status via [adb](https://developer.android.com/studio/command-line/adb)

```
adb shell cmd game mode [standard|performance|battery] <PACKAGE_NAME>
```

## References

https://developer.android.com/games/gamemode/gamemode-api

https://developer.android.com/reference/android/app/GameManager

## License

Copyright 2022 The Android Open Source Project

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

```
https://www.apache.org/licenses/LICENSE-2.0
```

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
