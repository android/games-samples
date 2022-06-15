# AGDKTunnel

A sample project demonstrating use of the Android Game Development Kit libraries.
AGDKTunnel is derived from the NDK sample Endless Tunnel.

AGDKTunnel uses the following AGDK libraries:

* Android Performance Tuner
* Frame Pacing
* GameActivity
* GameController
* GameTextInput
* Memory Advice
* Oboe

AGDKTunnel can optionally use the following Play libraries:

* Play Games Services for Play identity and cloud save
* Play Asset Delivery (via Play core libraries)
* Input SDK for Google Play Games for PC

## Building

Open the `agdktunnel' directory in Android Studio 4.2 or higher.

## Memory Advice

AGDKTunnel integrates the Memory Advice library to receive hints when memory usage is becoming
critical and risks being terminated by the OS due to low available memory.

Memory consumption is artificially inflated using the MemoryConsumer class, which also displays
status information onscreen. This is off by default. The **Consume Memory** button in the title
screen can be used to enable memory consumption.

When active, the Memory Consumer allocates at random intervals blocks of variable sizes and
places them into two memory pools, dubbed 'General' and 'Cache'. If the Memory Advice library
signals a critical memory situation, the Memory Consumer frees all allocations from the Cache
pool. After the first critical memory warning, the Memory Consumer ceases to allocate into the
General pool and only allocates into the Cache pool.

When active, the Memory Consumer displays statistics on screen in the UI and Play scenes. The
information is formatted as follows:

`(status) (crit warning count) (percent available) (General pool size) (Cache pool size)`

**(status)**
`OK|WARN|CRIT`
Memory State reported by the Memory Advice library
**(crit warning count)**
The number of times Memory Advice has reported a critical memory shortage
**(percent available)**
The estimated percentage of available memory reported by Memory Advice, this
is updated every five seconds
**(General pool size)**
Total allocation size of the General pool in megabytes
**(Cache pool size)**
Total allocation size of the Cache pool in megabytes

## Android Performance Tuner (APT)

Android Performance Tuner is disabled by default. To enable it, perform the following steps:

1. Ensure APT build prerequisites are met
2. Add an APT API key
3. Enable the APT option in gradle.properties

These steps are described in more detail in the following subsections.

### APT Prerequisites

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

#### Note for macOS developers

The runtime data files for Android Performance Tuner are compiled using the
`protoc` compiler located in `third_party/protobuf-3.0.0/install/mac/bin/protoc`.
This executable is not codesigned or notarized. You may need to allow execution of the relevant
files using the **System Preferences -> Security & Privacy** control panel, adjust your
Gatekeeper settings or compile your own protoc from the [protobuf](https://github.com/protocolbuffers) repo.

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
