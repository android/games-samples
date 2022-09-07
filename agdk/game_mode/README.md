# ADPF (Android Device Performance Framework) sample

This is a basic sample app that demonstrates use of the ADPF API.
With ADPF APIs, an application can
- Monitor device's thermal throttling status and would be able to
  change it's task load dynamically based on the status.
  [a relative link](./src/main/cpp/adpf_manager.cpp#L88)
- Supply the system a performance hint of the process so that
  the system can allocate applications threads efficiently.
  [a relative link](./src/main/cpp/adpf_manager.cpp#L175)

The sample:

* Displays device's thermal status using ADPF API.
* Dynamically change workload based on the API's hint.

## Prerequisites

Before building in Android Studio the following prerequisites must be
performed:

## Requirements
Minimum API level of 30 (R) and supported devices (such as Pixel 4~) are
required for the thermal headroom APIs working.
Also minimum API level of 31 (S) and supported devices (such as Pixel 6~) are
required for the performance hint APIs working.

### 3rd part libraries

1. Open a terminal and set the working directory to `agdk/third_party/`
2. `git clone -b v1.80 https://github.com/ocornut/imgui`
3. `git clone https://github.com/bulletphysics/bullet3`


## Building

Once the prerequisites are complete, open the folder in Android Studio 4.2
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
