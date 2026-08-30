**Input Capture Plugin Documentation**

![](https://github.com/vlad-unity/games-samples/blob/main/googleplaygamesforpc/unity_projects/input_capture_package/Images/preview.gif)

**High level plugin description**

Unity Input capture plugin provides the ability to use custom mouse pointer as well as tracking raw mouse input on the platforms such as Android and Google Play Games on PC.  
  
**Key Features**

- Mouse input capture toggles with the Space key
- Mouse button state events
- Mouse scroll events
- Screen border detection
- Manual pointer position set
- Customizable cursor graphics

**Distribution**

  
Plugin represented in a form of the unitypackage archive that can be obtained from the following repository: [input_capture_package](https://github.com/android/games-samples/tree/main/googleplaygamesforpc/unity_projects/input_capture_package) 
Upon download and import it into the Unity project, plugin with expand the following contents:

  
  
  


**Plugin contents**

![](https://github.com/vlad-unity/games-samples/blob/main/googleplaygamesforpc/unity_projects/input_capture_package/Images/structure.png)

  


- Example - Directory containing the example scene as well as all the scene assets
- InputCaptureExample.unity - The example test scene
- InputCaptureExample.cs - This is the monobehaviour that creates buttons to capture/release on Ecs and prints values to GUI
- InputCapture.cs - This the monobehaviour expected to be in the scene and receiving update calls
- InputCapture.java - This is the native side that calls Android API functions
- MouseCapture.cs - This is the wrapper around java part
- Readme.md - Attached readme file

  
  
**Supported Unity versions:**

2017.4 - 2022+  
HPE 23.1.21+ kiwi

  
  


**Default behavior**

  


In order to test the plugin works, please import it into the project and open the test scene in Example -> Scene.

![](https://github.com/vlad-unity/games-samples/blob/main/googleplaygamesforpc/unity_projects/input_capture_package/Images/build-window.png)

  


Add this scene in your build window, build and deploy the project. Once the project is running, you can acquire the in-game mouse pointer by toggling the space key.

  


![](https://github.com/vlad-unity/games-samples/blob/main/googleplaygamesforpc/unity_projects/input_capture_package/Images/hit-space.png)

Since mouse delta is actively reported and visual cursor position is adjusted by adding current delta to the last absolute position, resetting the pointer or setting it to any desired location can be done simply by setting it’s last location.

  
  ![](https://github.com/vlad-unity/games-samples/blob/main/googleplaygamesforpc/unity_projects/input_capture_package/Images/mouse-left.png)
  
**Native plugin side implementation and extension**

  
  
  


Native implementation is done via overriding the native android method onCapturedPointer(View view, MotionEvent event) that receives the MotionEvent argument as the event fires. Within the  MotionEvent we can query for the different data, such as distinguishing by the event type. Different data that can be found within the event can be extended for. For the full list of the getters please follow the link: [MotionEvent](https://developer.android.com/reference/android/view/MotionEvent)

![](https://github.com/vlad-unity/games-samples/blob/main/googleplaygamesforpc/unity_projects/input_capture_package/Images/java.png)

  
  
  


In order to pass the data to the native functions, such the we have to use the object array following the next example:

  ```
myVariable = new object\[1];  //allocation of the generic object to avoid C# parameter allocations at runtime

myVariable[0] = myData; //assigning the actual value

nativeHandle.Call<float>("SetMouselX", myVariable); // performing native call while passing our initialized parameter
```
  


**Acquiring and releasing callback**
  

In order to subscribe to the mouse acquisition event, please add your logic to the handler implementation inside of the InputeCaptureExample:

```
private void InputCapture_MouseCaptureChangedCallback(bool mouseCaptured);
```

Here the mouseCaptured flag can be used to execute the custom login depending on the mouse capture acquisition state.


**Screen borders confinement**

Currently the screen border detection happens on the unity side and the pointer will be confined within the screen. Please find the implementation details below.  
  

```
if (Math.Abs(futureMousePosition.x) < Screen.width * 0.5f 
  && Math.Abs(futureMousePosition.y) < Screen.height * 0.5f) {

  if (mouseDelta.x != 0 && mouseDelta.y != 0) 
    m_MousePosition += mouseDelta;

  picture.anchoredPosition = new Vector2(m_MousePosition.x, m_MousePosition.y);
}
```
  


Update() function of InputeCaptureExample.cs is stopping the cursor at the screen borders and utilizes mouseDelta, as opposed to the absolute position.
  


**Resetting and setting mouse cursor location**

In order to set the custom mouse location we can use the callback:  
  

```
private void InputCapture_MouseCaptureChangedCallback;
```
inside of the_InputeCaptureExample

To set the mouse position to any desired location. Notice, that by default the pointer location will reset to (0,0) on each mouse capture release:  
  ```
m_MousePosition = Vector2.zero;
  ```
  
**Setting custom mouse pointer**

![](https://github.com/vlad-unity/games-samples/blob/main/googleplaygamesforpc/unity_projects/input_capture_package/Images/attribute.png)

In order to customize the cursor graphics that are used in the project, please put our own cursor image under Canvas->Cursor in the scene. Make sure to supply square size, power of two, transparent back graphics. Additionally low and high res images can be consumed.

Make sure to center the pivot of the cursor graphics as well.

  


**Cursor states**

Please have a look at the InputeCaptureExample setCursorState() implementation to extend or customize the cursor states. Currently the press or drag state is implemented.

In order to extend the functionality with the state such as load or progress, additional loading image assets need to be imported. After this we can listen and set the state upon appropriate event, such as after clicking the open and load button for instance).

  
  


![](https://github.com/vlad-unity/games-samples/blob/main/googleplaygamesforpc/unity_projects/input_capture_package/Images/states.png)

  
**FPS Implementation**

In order to extend the mouse capture to implement first person mouse rotation, please consider the example below:

```
var mouseDelta = MouseCapture.GetMouseDelta();

horizontalRotation += mouseDelta .x * 0.1f;

varticalRotation -= mouseDelta .y * 0.1f;

cameraTransform.rotation = Quaternion.Euler(varticalRotation , horizontalRotation , 0);
```

The following code can be added to the Update function of any MonoBehaviour that can listen to the raw mouse delta and input capture acquisition.

  


**Known issues**

  
If capture will not work from the first time on the emulator, we need to toggle the dev options->input->touchscreen/kiwi mouse. This should be performed every time the emulator is completely shut down and started again.

  
![](https://github.com/vlad-unity/games-samples/blob/main/googleplaygamesforpc/unity_projects/input_capture_package/Images/kiwi.png)

  
