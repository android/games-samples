# Android Games Samples Repository

This repository contains sample projects demonstrating various aspects of
developing games for Android. A description of the high-level directories
in this repo follows:

## Android Game Development Kit samples
See the [agdk/README.md](agdk/README.md) file for descriptions of individual samples.

* **[agdk/](agdk)** (C/C++) - These samples demonstrate integration and use of
the [Android Game Development Kit libraries](https://developer.android.com/games/agdk/libraries-overview).
* **[agdk/agde](agdk/agde)** (C/C++) - These samples demonstrate use of the
[Android Game Development Extension](https://developer.android.com/games/agde).
* **[agdk/adpf](agdk/adpf)** (C/C++) - This sample demonstrate the use of [Thermal API](https://developer.android.com/ndk/reference/group/thermal) to limit CPU/GPU usage to prevent device from overheating which may trigger hardware throttling. Checkout [Thermal Mitigation](https://source.android.com/docs/core/architecture/hidl/thermal-mitigation) for more information.
* **[agdk/game_mode](agdk/game_mode)** (C/C++) - This sample demonstrate the use of [Game Mode API](https://developer.android.com/games/gamemode/gamemode-api) to adjust game's graphic fidelity according to user's preference.

## TrivialKart sample

TrivialKart is a sample game demonstrating use of Google Play technologies
on Android.

TrivialKart is currently available for the
[Godot](https://www.godotengine.org) game engine and the
[Unity](https://www.unity.com) engine.

* **[trivialkart/trivialkart-godot](trivialkart/trivialkart-godot)**

Demonstrates use of Google Play Billing for in-app purchases.

* **[trivialkart/trivialkart-unity](trivialkart/trivialkart-unity)**

Demonstrates use of multiple Google Play technologies:

* Google Play Billing for in-app purchases using Unity IAP
* Google Play Games Services
* Google Play Games for PC
* Play Integrity

## Google Play Games for PC samples

The **[googleplaygamesforpc/](googleplaygamesforpc)** directory contains
samples and tools specific to Google Play Games for PC.

* **[googleplaygamesforpc/unity_projects/platform_utils_package](googleplaygamesforpc/unity_projects/platform_utils_package)**

Example tooling to help automate configuring a Unity engine project to target
Google Play Games for PC.
