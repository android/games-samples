using UnityEngine;

[DefaultExecutionOrder(-32000)]
public class MouseCapture : MonoBehaviour
{
	public static MouseCapture captureInstance { get; private set; }
	public static InputCapture InputCapture { get; private set; }
	private bool canDisposeAPI;
	public static bool isMouseCaptureEnabled => InputCapture is {IsMouseCaptureEnabled: true};
	public delegate void MouseCaptureChangedCallbackMethod(bool mouseCaptured);
	public static event MouseCaptureChangedCallbackMethod MouseCaptureChangedCallback;

	internal static void FireMouseCaptureChangedCallback(bool mouseCaptured) => MouseCaptureChangedCallback?.Invoke(mouseCaptured);
	private void Awake()  {
		if (captureInstance != null)  {
			Destroy(gameObject);
			return;
		}
		canDisposeAPI = true;
		captureInstance = this;
		DontDestroyOnLoad(gameObject);
		
		InputCapture = new InputCapture();
	}

    private void OnDestroy()  {
        if (canDisposeAPI && InputCapture != null)  {
			InputCapture.Dispose();
			InputCapture = null;
        }
    }

    public static void EnableMouseCapture(InputCapture.EnableMouseCaptureFinished finished)  {
		if (InputCapture != null) InputCapture.EnableMouseCapture(finished);
		else if (finished != null) finished(false, "Native API is not initialized");
	}

	public static void DisableMouseCapture()  {
		if (InputCapture != null) InputCapture.DisableMouseCapture();
	}

	private void Update()  {
		if (InputCapture != null) InputCapture.Update();
	}


	public static Vector2 GetMousePosition()  {
		if (InputCapture == null) return Vector2.zero;
		return InputCapture.GetMousePosition();
	}
	
	public static Vector2 GetMouseDelta()  {
		if (InputCapture == null) return Vector2.zero;
		var delta = InputCapture.GetMouseDelta();
		return delta;
	}


	public static Vector2 GetMouseScroll()  {
		if (InputCapture == null) return Vector2.zero;
		return InputCapture.GetMouseScroll();
	}
	
	public static bool GetMouseButton(MouseButton.Location location)  {
		if (InputCapture == null) return false;
		return InputCapture.GetMouseButton(location);
	}
}