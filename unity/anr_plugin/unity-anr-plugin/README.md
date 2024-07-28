# Unity ANR Plugin

The Unity ANR Plugin provides the access to the following APIs from Android SDK:

1. [ApplicationExitInfo](https://developer.android.com/reference/android/app/ApplicationExitInfo) to diagnose crashes and ANRs for Unity 2022 and older
2. [StrictMode](https://developer.android.com/topic/performance/vitals/anr#strict_mode) to identify and prevent ANRs/deadlocks caused by I/O operations on the Main Thread
3. Polling the memory stats for Unity 2021 and older

This will help you to diagnose your ANRs, on Unity versions 2022 and less. From Unity 2023, this functionality is already integrated into Unity.

# How to use
1. **Build** the project in Android Studio by open the project folders in Android Studio and hitting "Make Project"

2. **Grab** the `UnityANR-Plugin.aar` file (compiled plugin) from the `Plugin/build/outputs/aar/` folder

3. **Import** the `UnityANR-Plugin.aar` file into your Unity Editor

4. **Call** the APIs!

# License
**unity-anr-plugin** is distributed under the terms of the Apache License (Version 2.0). See the
[license](LICENSE.txt) for more information.
