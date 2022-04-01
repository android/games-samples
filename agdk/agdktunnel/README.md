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
