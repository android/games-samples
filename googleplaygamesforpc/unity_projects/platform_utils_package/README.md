# Google Play Games Unity Package
A tool for your unity project to automate routines for Google Play Games development with Unity.

![Preview](https://raw.githubusercontent.com/natetrost/games-samples/main/googleplaygamesforpc/unity_projects/platform_utils_package/Images/preview.jpg)

### Features:
 * GPG platform define script, that will add `UNITY_ANDROID_x86_64` define to the list of custom defines withing Player settings
 * Asset importer script, that will set every imported texture to a desired texture compression - currently DXTC by default
 * Setting window for the options available under `Tools->GPG Settings`

### Contents:
Plugin follows recommended Unity structure:

root\
  ├── package.json\
  ├── README.md\
  ├── Editor\
  │   ├── GeneralSettings.cs\
  │   └── SettingsWindow.cs\
  ├── Runtime\
  │   ├── AssetImporter.cs\
  │   └── HPEDefineSymbolAdder.cs\


## Installation
Simply clone or download the repository at [games-samples](https://github.com/android/games-samples), then import  `platform_utils_package` folder into your project's `Assets` folder. If everything's fine, you should see a new item under the "Tools" dropdown menu at the top of Unity editor called `GPG Settings`.

### Credits
Licensed under the Apache License, Version 2.0 (the "License"); You may not use this file except in compliance with the License. You may obtain a copy of the License at [apache](https://www.apache.org/licenses/LICENSE-2.0).