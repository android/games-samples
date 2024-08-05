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
  * **[agdk/agdktunnel](agdk/agdktunnel)** (C/C++) - This is a sample game that uses multiple AGDK libraries.
  * **[agdk/adpf](agdk/adpf)** (C/C++) - This sample demonstrates the use of the [Adaptive performance APIs](https://developer.android.com/games/optimize/adpf).
  * **[agdk/game_mode](agdk/game_mode)** (C/C++) - This sample demonstrates use of the [Game Mode API](https://developer.android.com/games/gamemode/gamemode-api) to adjust the game's graphic fidelity according to the user's preference.

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

## Unity engine samples

The **[unity/](unity)** directory contains samples demonstrating best practices
when targeting Android with the Unity engine.

* **[unity/power_efficiency_demo](unity/power_efficiency_demo)**

Sample project implementing best practices for power efficiency on Android
when using the Unity engine for game development.

## Codelabs

The **[codelabs/](codelabs)** directory contains two codelabs on learning how
to use Play Asset Delivery for games written using C++ or the Unity engine.
