# Power efficiency demo

This is a sample project for Unity engine (2021 LTS and 2022 LTS),
demonstrating best practices for maximizing energy efficiency and
reducing power consumption on Android.

You will need to import one prerequisite package in Unity before the demo will run. See the
Prerequisties section for instructions.

Some features have minimum Android version requirements and will not be in effect on earlier
versions of Android.

## Features

A summary of included features, see the details section of each for more information:

* Vulkan graphics API with [VkQuality](https://developer.android.com/games/engines/unity/unity-vkquality) plugin integration
* Adaptive performance plugin with Android adapter
* Android [Game Mode](https://developer.android.com/games/optimize/adpf/gamemode/gamemode-api) API query for retrieving user performance/battery
preferences
* Adjusting [display panel refresh rate](https://developer.android.com/media/optimize/performance/frame-rate) to match game frame rate target

## Prerequsities

The VkQuality .aar library file could not be directly included in the project as it contains binary
runtime library files. To install it, perform the following steps:

1. Download the latest .unitypackage from the VkQuality GitHub [releases page](https://github.com/android/vkquality/releases)
2. Open the demo project in the Unity editor
3. Import the downloaded .unitypackage, but when doing so *only import the VkQuality aar file*. Importing other files such as the Activity java file or the sample scene from the unitypackage will break the demo!

Changing the Game Mode at runtime using the default demo package name may not be possible.
See the Game Mode section for details.

## Vulkan

Vulkan is a more efficient API than OpenGL ES. Use of Vulkan can reduce the CPU overhead
of graphics rendering in your game, and save power. This demo integrates the
[VkQuality](https://developer.android.com/games/engines/unity/unity-vkquality)
plugin, which restricts Vulkan to a smaller set of modern devices with recent drivers, falling
back to OpenGL ES on all other devices. VkQuality allows you to take advantage of Vulkan within
a constrained testing and support space.

*Note:* For maximum efficiency when using Vulkan, you must have the
`Apply display rotation during rendering` option enabled in the Android player settings.
Compatibility with this feature may require changes to certain custom shaders. See the
[Unity documentation](https://docs.unity3d.com/Manual/vulkan-swapchain-pre-rotation.html)
of this feature for more details. Without the feature enabled, you may experience negative
performance effects.

## Adaptive performance

Unity offers a [Adaptive performance plugin](https://docs.unity3d.com/Packages/com.unity.adaptiveperformance@5.1/manual/index.html) for the Unity engine that works with 'providers' of
platform specific adaptive performance features. An 
[Android provider](https://docs.unity3d.com/Packages/com.unity.adaptiveperformance.google.android@1.0/manual/index.html) is available that
exposes Android specific adaptive performance functionality through the adaptive performance
plugin.

This demo demonstrates including a later version of the Adaptive performance plugin (5.1.0) and
the Android provider (1.3.1) than are available through the Package Manager in Unity 2021 and
Unity 2022. They are references directly from the `packages/manifest.json` file:

```
  "dependencies": {
    "com.unity.adaptiveperformance": "5.1.0",
    "com.unity.adaptiveperformance.google.android": "1.3.1",
```

The demo does not demonstrate the dynamic features available from the plugin and Android provider. However,
having the plugin active on Android 13 or higher will enable it to use the
[Performance Hint Manager](https://developer.android.com/games/optimize/adpf/performance-hint-api) to
help optimize the Unity engine CPU performnace at runtime, which can make your game run more
efficiently. 

## Game Mode

The Android [Game Mode](https://developer.android.com/games/optimize/adpf/gamemode/gamemode-api)
API is a method for querying performance preferences the user may have for your game. The user
can set the Game Mode using vendor specific tools like the Game Dashboard on Pixel or the
Gaming Hub/Game Booster on Samsung devices. The demo will query for the current game state and
adjust the game frame rate and display panel refresh rate depending on whether the game mode is
set to a battery optimized, neutral, or performance optimized preference.

Game Mode API is available on Android 13 and higher. Identifying that your game supports Game
Mode requires adding some metadata to your application bundle. The custom manifest file
located in `Assets/Plugins/Android/AndroidManifest.xml` references this metadata, which is
contained in an XML file stored in the
`Assets/Plugins/Android/game-mode-xml.aar` archive file.

The Game Dashboard or Game Booster may not recognize a game and show Game Mode setting options
if it is not recognized as a game downloaded through the Play Store. You may need to upload
to the Google Play Console, create an internal test track, and install it via the Play Store.

If you already have a game on the Play Store, you could also rename the package name of the demo
to one of your existing games and 'spoof' it to enable the Game Mode settings. You can use
a debug signing key for this, it does not need to be signed for upload to the Play Store.

## Display panel refresh rate

Premium Android devices include display panels that can operate at high refresh rates, often
of 120Hz or more. Operating at a high display refresh rate consumes additional power. If your
game is designed to run at 30 or 60 frames per second, running the display panel at a higher
refresh rate wastes power.

One method for changing the display panel rate to a more optimal match to the game frame rate
is enabling the `Optimize Frame Pacing` option in the Android player settings. In Unity 2021 LTS
and higher this option will change the display panel rate.

Some developers have observed frame rate reductions or other undesriable behavior when
Optimize Frame Pacing is enabled. This demo includes an alternate method of changing the
display panel refresh rate, by directly calling the Android [setFraneRate](https://developer.android.com/reference/android/view/Surface#setFrameRate(float,%20int)) API. This API is only available on
API 30 (Android 11) and higher, it will have no effect on earlier versions of Android.

## Unity 6 notes

Some features of this demo are integrated into Unity 6. In Unity 6 you can directly access the
Game Mode API through the Android Adaptive Performance provider. Unity 6 also has its own
customizable Vulkan allow-list system that is similar to VkQuality but does not require the
addition of a custom Activity.

## Issues

Report any problems using the GitHub issues page of the games-samples repository. This demo
is not an officially supported Google product.

## Release history

1.0.0 - (20240719) - Initial release, derived from VkQuality sample project
