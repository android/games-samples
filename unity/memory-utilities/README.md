# Memory Utilities

This folder contains utility scripts for Unity developers to monitor and track the memory footprint and Android process state (OOM Score Adjustment) of their application.

## Contents

* **`AndroidProcessStats.cs`**: Demonstrates how to use JNI (Java Native Interface) within Unity to link into the Android `ActivityManager` APIs. It provides functionality to fetch the running app processes, their importance, and OS-level state. It also contains lightweight methods to read memory stats directly from the Linux `/proc` filesystem.
* **`GameMemoryMonitor.cs`**: Shows how to hook this information up for continuous monitoring and debugging. It demonstrates how to poll the Android APIs on a background thread to prevent frame drops, and how to hook into lifecycle events (e.g., `OnApplicationPause`) to track memory changes when the app is backgrounded or resumed.
