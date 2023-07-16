# ADPF (Android Device Performance Framework) sample

This sample demonstrates use of the [Adaptive performance APIs](https://developer.android.com/games/optimize/adpf).

Using these APIs, a game can: 

- Monitor the device's thermal throttling status and [dynamically change its workload in response
- Give the OS performance hints about critical thead workloads to permit scheduling optimization

The sample:

- Displays the device's thermal status using the Thermal API.
- Reduces the target framerate when the device thermal state changes
- Provides performance hints about the sample's primary thread work

## Prerequisites

Before building in Android Studio the following prerequisites must be
performed:

## Requirements

### Thermal API

- Minimum API level of 30 (Android R) and a supported device (such as Pixel 4~)

### Performance Hint Manager API

- Minimum API level of 31 (Android S) and a supported device (such as Pixel 6~)

### 3rd party libraries

1. Open a terminal and set the working directory to `agdk/third_party/`
2. `git clone https://github.com/ocornut/imgui`
3. `git clone https://github.com/epezent/implot`
4. `git clone https://github.com/bulletphysics/bullet3`

## Building

Once the prerequisites are complete, open the folder in Android Studio 2021.2
or higher. You can then build and run the samples from Android Studio.

## Reference

https://developer.android.com/reference/android/os/PerformanceHintManager

https://developer.android.com/reference/android/os/PowerManager

https://developer.android.com/ndk/reference/group/thermal

## License

Copyright 2022 The Android Open Source Project

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
