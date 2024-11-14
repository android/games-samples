# Unity ANR Plugin
The plugin aims to expose Android OS API functions that help you debug your code and profile.

### Supported Unity Versions
- [x] Unity 2022
- [x] Unity 2021
- [ ] Unity 2023 (not tested)
- [ ] Unity 2023 with GameActivity (not tested)

## Setup
- Add the _Unity ANR Plugin_ prefab into your first scene.
  - This _GameObject_ does not destroy after scene unloading.
- The _UnityANR_ script attached to this _GameObject_ will initialize the Java Plugin in an Awake method.
- The script will receive messages from the Java Plugin. Therefore, it is crucial not to rename it after the game runs.
- After initializing, you can safely call the _PluginBinder_ methods.
![setup](https://media.github.cds.internal.unity3d.com/user/6263/files/4508e6a8-a171-496b-a89b-884eee4a83c4)



> [!IMPORTANT]
> Once the _UnityANR_ script receives the message from the plugin, the data parsing happens on the Thread Pool for optimization purposes. 
> Check the [Unity Thread Solutions](#unity-thread-solutions) for more information on running code in the Unity Main Thread.


## Application Exit Info
Describes the information of an [application process's death](https://developer.android.com/reference/android/app/ApplicationExitInfo).

### Use Case Example
- If this call returns an ANR with a trace file, you could submit an issue to your Reporting Tool and attach a log file and other relevant information.
- Or knowing that a previous run had a problem and it will likely repeat, your game code could take actions to enhance logging or prevent the ANR from happening by:
  - Decreasing graphic quality.
  - Enabling more logs in your game to capture more information.
  - Turning on [Memory Stats](#memory-stats) to enhance your system resources comprehension.

### Usage
- Call the _PluginBinder.CheckAppExitInfo_ method.
- The plugin runs on a separate Thread to not impact UnityMain or Main thread.
- Once data is fetched, the plugin uses the UnitySendMessage to communicate with the _UnityANR_ script.
- Listen to the _PluginEvents.ApplicationExitInfoFetched_ event to handle the data.
  - Remember that this call does not happen on the Unity Main Thread 

Sample response:
```
{
    "entries": [
        {
            "Description": "user request after error: Input dispatching timed out (21b6f76 App/com.google.unity.plugin.UnityActivity (server) is not responding. Waited 10001ms for MotionEvent)",
            "FilePath": "ANR_Traces/AppExitInfo_88d33aa0-1604-45d4-be24-c1255e583af5.trace",
            "ID": "88d33aa0-1604-45d4-be24-c1255e583af5",
            "Reason": "ANR",
            "ReasonCode": 6
        }
    ]
}
```

To access the file:
```
var path = Path.Combine(Application.persistentDataPath, lastExitInfo.FilePath);
```

## Strict Mode
It helps you [find accidental I/O operations](https://developer.android.com/topic/performance/vitals/anr#strict_mode) on the main thread.

### Use Case Example
- During development, you can enable it to identify Main Thread blocks and refactor your code to other threads.
- Identify issues with Third Party plugins.

### Usage
- Call the _PluginBinder.EnableStrictMode_ method.
- The plugin calls the Android API with the following settings:
  - _detectDiskReads_, _detectDiskWrites_, _detectNetwork_, _detectCustomSlowCalls_

## Memory Stats
Return general information about the [memory state of the system](https://developer.android.com/reference/android/app/ActivityManager#getMemoryInfo(android.app.ActivityManager.MemoryInfo))

### Use Case Example
- This can be used to track memory information and include it in the ANR report by adding logs or breadcrumbs
- Manage your game memory usage, though note that polling is not recommended and [Memory Warning](#Memory Warning) is the preferred way to do this

### Usage
- The _UnityANR_ script starts the memory stats retriever in the plugin.
- The plugin uses a _ScheduledThreadPoolExecutor_ to run the memory stats polling every X seconds.
  - The interval can be set in _Unity ANR Plugin_ prefab.
- The _UnityANR_ script also invokes a function to get the last memory stats.
  - You can also call it manually using _PluginBinder.GetMemoryInfo_.
- Then, the plugin sends a message to the _UnityANR_ script, which parses the data into _MemoryData_ struct.

Sample response:
```
{
    "AvailableHeap": "248",
    "AvailableMem": "1311",
    "MaxHeap": "256",
    "PercentAvailable": "36.7",
    "Threshold": "141",
    "Timestamp": "15:19:06:196",
    "TotalMem": "3570",
    "UsedHeap": "8"
}
```

## Memory Warning
Returns the result defined in the [onTrimMemory](https://developer.android.com/reference/android/content/ComponentCallbacks2#onTrimMemory(int)). Similar to the [Application.memoryUsageChanged](https://docs.unity3d.com/2021.3/Documentation/ScriptReference/Application-memoryUsageChanged.html).

### Use Case Example
- Unload unused content when memory is pressured
- Track how many sessions the user device received a severe memory warning. If significant, you could decrease the game quality since the device may constantly struggle with memory.

### Usage
- Override the Custom Main Manifest in the Unity Project Settings
- Change the activity to `android:name="com.google.unity.plugin.UnityActivity"`
- The _UnityANR_ receives the messages from the plugin.
- Listen to the _PluginEvents.LowMemoryReceived_ event and use the _TrimMemoryData_ struct.

## Debug
The plugin also has several functions that force crashes and ANRs for debugging purposes.

### Forcing ANR
The Plugin calls the _ActivityManager.appNotResponding_ method to tell the system that the app's wedged and would like to trigger an ANR.
From any Unity script, just call the _PluginBinder.ForceANR_

### Forcing Thread Sleep
There are 2 options when calling the _PluginBinder.ForceThreadSleep_
- If the argument is _true_ the plugin puts the Unity Main thread to sleep
- Else, the plugin accesses the UI Thread/Main Thread and puts it to sleep
- The plugin sets the default wait time to 15000 ms

## Unity Thread Solutions
To run Unity-specific operations, you may need to switch from a Thread Pool to Unity Thread. There are a couple of solutions to accomplish this:
- [Await Support](https://docs.unity3d.com/2023.2/Documentation/Manual/AwaitSupport.html)
- [UniTask]( https://github.com/Cysharp/UniTask)
- [UnityMainThreadDispatcher](https://github.com/PimDeWitte/UnityMainThreadDispatcher) 
