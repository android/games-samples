Input Capture Unity Package

Input capture for Google Play Games development with Unity.

![Preview](/Images/preview.gif)

### Features:
* Mouse input capture toggles with the Space key
* Mouse button state events
* Mouse scroll events
* Screen border detection

### Contents:

InputCapture.java - is the native side that calls Android API functions
InputCapture.cs - is the wrapper around java part
MouseCapture.cs - is the monobehaviour expected to be in the scene and receiving update calls
InputCaptureExample.cs - is the monobehaviour that listens to Space key to toggle the capture and prints values to GUI

### Supported Unity versions:
2017.4 - 2022.*

HPE 23.1.21+ kiwi

### Known issues

If capture will not work from the first time on the emulator, toggle the dev options->input->touchscreen/kiwi mouse

![Kiwi](/Images/kiwi.png)

### The Cursor

Put our own cursor image under Canvas->Cursor in the scene.

## Installation
Simply clone or download the repository at [games-samples](https://github.com/android/games-samples), then import  `input_capture_package` folder into your project's `Assets` folder.

### Credits
Licensed under the Apache License, Version 2.0 (the "License"); You may not use this file except in compliance with the License. You may obtain a copy of the License at [apache](https://www.apache.org/licenses/LICENSE-2.0).