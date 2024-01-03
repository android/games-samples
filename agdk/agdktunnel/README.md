# AGDKTunnel

A sample project demonstrating use of the Android Game Development Kit libraries.
AGDKTunnel is derived from the NDK sample Endless Tunnel.

AGDKTunnel uses the following AGDK libraries:

* Android Performance Tuner
* Frame Pacing
* GameActivity
* GameController
* GameTextInput
* Oboe

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

Open the `agdktunnel' directory in Android Studio 2022.3 or higher.

## Android Performance Tuner (APT)

Android Performance Tuner is disabled by default. To enable it, perform the following steps:

1. Ensure APT build prerequisites are met
2. Add an APT API key
3. Enable the APT option in gradle.properties

These steps are described in more detail in the following subsections.

### APT Prerequisites

#### Installing the protobuf compiler

APT needs to run the `protoc` compiler to compile the protobuf files. You have
two options for installing `protoc`:

1. Follow the instructions in `third_party/protobuf-3.0.0/src/README.md` to compile
your own protoc.
2. Download precompiled binaries from [the protobuf releases page](https://github.com/protocolbuffers/protobuf/releases/tag/v3.0.0).

The build scripts expect the following layout in the `third_party/protobuf-3.0.0/install` directory:

```

./linux-x86/bin:
protoc

./linux-x86/lib:
(shared libraries)

./mac/bin:
protoc

./mac/.libs:
(shared libraries)

./win/bin:
protoc.exe

```

You can ignore subdirectories for OSes you aren't using.

##### Note for macOS developers

Precompiled versions of the `protoc` compiler arenot codesigned or notarized.
You may need to allow execution of the relevant
files using the **System Preferences -> Security & Privacy** control panel, adjust your
Gatekeeper settings or compile your own protoc.

#### Python

Python is expected to be available in your `PATH`. The `protobuf` package is
expected to have been installed via `pip`. In some cases, Android Studio may be using an internal
install of Python rather than a system install. If this is the case, you may need to run
`pip install protobuf` from the Terminal tab in Android Studio.

#### APT API Key

The APT functionality requires a valid API key to operate. This is not
necessary to run the sample. For information on how to configure an API key
for APT, see the **Get an API key for the Android Performance Tuner**
section of the Codelab [Integrating Android Performance Tuner into your native Android game](https://developer.android.com/codelabs/android-performance-tuner-native#1).

#### Enable the APT option

To enable building the runtime APT assets and use the library at runtime, edit the
`gradle.properties` file and change: `APTEnabled=false` to `APTEnabled=true`. When switching
configurations, it is recommended to sync the gradle file, and run
**Build -> Refresh Linked C++ Projects** and **Build -> Clean Project** before rebuilding.

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

1.0.4 - Merged Play integrations from AOSP. Play Asset Delivery,
        Input SDK, PGS sign in.
