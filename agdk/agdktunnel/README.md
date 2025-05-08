# AGDKTunnel

A sample project demonstrating use of the Android Game Development Kit libraries.
AGDKTunnel is derived from the NDK sample Endless Tunnel.

AGDKTunnel uses the following AGDK libraries:

* Frame Pacing 120 
* GameActivity 120
* GameController 120
* GameTextInput 120
* Oboe 120

AGDKTunnel can optionally use the following Play libraries:

* Play Games Services for Play identity and cloud save
* Play Asset Delivery (via Play core libraries)
* Input SDK for Google Play Games for PC

AGDKTunnel uses the [Library Wrapper](https://developer.android.com/games/develop/custom/wrapper)
tool to generate interface files to call the Vibrator API in order to provide haptic
feedback on in-game collisions.

## Prerequisites

1. cd to `agdk/third_party`
2. git clone https://github.com/zeux/volk.git
3. git clone https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git

## Building

Open the `agdktunnel' directory in Android Studio 2024.3.1 Patch 1 (Meerkat) or higher.

## Library Wrapper notes

The configuration file used to generate the wrapped Android API is located in
`agdktunnel/library_wrapper/config.json`. The generated files are located in
`agdktunnel/app/src/main/cpp/native_wrappers`.

The generation was done by copying the Library Wrapper `lw.jar` file into the root `agdk` directory
and running the following command from a terminal with `adgk` as the working directory:

`java -jar lw.jar -o "agdktunnel/app/src/main/cpp/native_wrappers" -c "agdktunnel/library_wrapper/config.json"`

For more information on integrating generated wrapper code into your game, see the
[guide page](https://developer.android.com/games/develop/custom/wrapper-guide).

## Google Play Games for PC (optional)

Build variants are used to differentiate between the default (mobile) platform
and the PC platform. To build AGDKTunnel to run in Google Play Games for PC follow these steps:

1. Go to **Build > Select Build Variant** and select the `playGamesPC` [build variant](https://developer.android.com/studio/build/build-variants).
2. (Optional) Enable Play Games Services to turn-on cloud save on mobile and PC.
3. (Optional) Enable Play Asset Delivery API to delivery DXT1 compressed texture assets.

## Google Play Games Services (optional)

Play Games Services (PGS) is used for sign-in and cloud save. To enable these features, follow these steps:

1. Rename the package of AGDK Tunnel to a name of your choosing.
2. Create an application on the [Google Play Console](https://play.google.com/console/about/?) and follow the steps to set up Play Games Services using your package name.
3. Replace the **game_services_project_id** string value in `app/src/main/res/values/strings.xml` with the id of your project in the Google Play Console.

## Google Play Asset Delivery (optional)

The Play Asset Delivery (PAD) API with Texture Compression Format Targeting (TCFT) is used to
deliver optimal compressed textures (ETC2 by default and DXT1 for Google Play Games for PC). To
enable PAD, follow these steps:

1. Edit the `gradle.properties` file and change: `PADEnabled=false` to `PADEnabled=true`.
2. [Download the Play Core API into the project](https://developer.android.com/guide/playcore#native)
   and copy the `play-core-native-sdk` directory into the `apps/libs` directory.
3. Play Asset Delivery requires building an Android App Bundle instead of an APK. Bundletool will
   help you to test your Android App Bundle locally, download bundletool by visiting the
   [bundletool releases page](https://github.com/google/bundletool/releases) and install it in the
   root of the project.
4. Using Android Studio build an App Bundle **Build > Build bundle(s)/APK(s) > Build bundle(s)**
5. Install from the Android App Bundle file to your device using bundletool:

For the mobile variant in debug:

```
   java -jar bundletool-all-1.9.1.jar build-apks
      --bundle=app/build/outputs/bundle/mobileDebug/app-mobile-debug.aab
      --output=agdktunnel.apks
      --local-testing
   
   java -jar bundletool-all-1.9.1.jar install-apks --apks=agdktunnel.apks
```

For the Google Play Games for PC variant in debug:

```
   java -jar bundletool-all-1.9.1.jar build-apks
      --bundle=app/build/outputs/bundle/playGamesPCDebug/app-playGamesPC-debug.aab
      --output=agdktunnel.apks
      --local-testing
   
   java -jar bundletool-all-1.9.1.jar install-apks --apks=agdktunnel.apks
```

For more information see the codelab: [Using Play Asset Delivery in native games](https://developer.android.com/codelabs/native-gamepad#0)

## Version history

1.2.1 - Updated to current AGDK/NDK/AGP versions, deprecated Android Performance Tuner integration
1.2.0 - Major refactor to add basegameframework support, Vulkan/GLES simplerenderer, updating AGDK versions
1.0.4 - Merged Play integrations from AOSP. Play Asset Delivery,
        Input SDK, PGS sign in.
