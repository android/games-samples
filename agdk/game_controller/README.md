# Game Controller library sample

This is a basic sample app that demonstrates use of the Game Controller
library. The sample:

* Displays graphical representation of control inputs from the active controller
* Displays information about connected controllers
* Provides an interface to test optional extended controller features,
such as vibration

There are two versions of the sample. One uses NativeActivity, the other
uses GameActivity.

## Prerequisites

Before building in Android Studio the following prerequisites must be
performed:

### ImGui

1. Open a terminal and set the working directory to `agdk/third_party/`
2. `git clone -b v1.80 https://github.com/ocornut/imgui`

## Building

Once the prerequisites are complete, open the `nativeactivity` or
`gameactivity` directories in Android Studio 4.2 or higher. You can
then build and run the samples from Android Studio.

## License

Copyright 2021 The Android Open Source Project

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
