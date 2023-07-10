using System;
using UnityEngine;


public struct MouseButton
{
	public enum Location
	{
		Left,
		Right,
		Middle
	};
	private bool m_IsPressed;
	public bool IsPressed => m_IsPressed;

	public void Update(bool newState)
	{
		m_IsPressed = newState;
	}
}

public class InputCapture
{
	private AndroidJavaClass unityPlayer;
	private AndroidJavaObject unityActivity;
	private AndroidJavaObject nativeHandle;

	private bool m_IsInitialized, m_IsInitializationFailed;
	private bool m_IsMouseCaptureEnabled, m_WasMouseCaptureEnabled;
	public  bool IsMouseCaptureEnabled => m_IsMouseCaptureEnabled;
	private EnableMouseCaptureFinished m_EnableMouseCaptureFinished;
	public delegate void EnableMouseCaptureFinished(bool success, string message);

	private Vector2 m_MousePosition, m_MouseDelta;
	private float m_MouseScroll;
	private MouseButton m_MouseButtonLeft, m_MouseButtonRight, m_MouseButtonMiddle;

	public InputCapture()  {
		try  {
			using (var javaClass = new AndroidJavaClass("java.lang.Class"))  {
				using (var result = javaClass.CallStatic<AndroidJavaObject>("forName", "android.view.View$OnCapturedPointerListener"))  {
					if (result == null) return;
				}
				
				using (var result = javaClass.CallStatic<AndroidJavaObject>("forName", "android.view.View$OnKeyListener"))  {
					if (result == null) return;
				}
			}
		}
		catch (Exception e)  {
			Debug.LogError(e);
			return;
		}
		
		try  {
			unityPlayer = new AndroidJavaClass("com.unity3d.player.UnityPlayer");
		}
		catch (Exception e)  {
			Debug.LogError(e);
			return;
		}
		
		try  {
			unityActivity = unityPlayer.GetStatic<AndroidJavaObject>("currentActivity");
		}
		catch (Exception e)  {
			Debug.LogError(e);
			return;
		}

		try  {
			nativeHandle = new AndroidJavaObject("com.unity.InputCapture.InputCapture");
		}
		catch (Exception e)  {
			Debug.LogError(e);
			return;
		}
		
		try  {
			nativeHandle.Call("Init", unityActivity);
		}
		catch (Exception e)  {
			Debug.LogError(e);
			return;
		}
	}

	public void Dispose()  {
		if (nativeHandle != null)  {
			nativeHandle.Dispose();
			nativeHandle = null;
		}

		if (unityActivity != null)  {
			unityActivity.Dispose();
			unityActivity = null;
		}

		if (unityPlayer != null)  {
			unityPlayer.Dispose();
			unityPlayer = null;
		}
	}

	public void Update()  {
		if (m_IsInitializationFailed) return;
		if (nativeHandle == null || !nativeHandle.Call<bool>("InitCheck")) return;

		m_IsInitialized = true;
		if (!nativeHandle.Call<bool>("InitSuccessCheck"))  {
			m_IsInitializationFailed = true;
			return;
		}
		
		m_IsMouseCaptureEnabled = nativeHandle.Call<bool>("IsMouseCaptureEnabled");
		
		if (m_EnableMouseCaptureFinished != null)  {
			var enableMouseCaptureFinishedRef = m_EnableMouseCaptureFinished;
			m_EnableMouseCaptureFinished = null;
			if (nativeHandle.Call<bool>("EnableMouseCaptureFinishedCheck"))  {
				m_IsMouseCaptureEnabled = nativeHandle.Call<bool>("EnableMouseCaptureSuccessCheck");
				if (m_IsMouseCaptureEnabled) enableMouseCaptureFinishedRef(true, string.Empty);
				else enableMouseCaptureFinishedRef(false, "Native Android API error");
			}
		}
		
		if (m_WasMouseCaptureEnabled != m_IsMouseCaptureEnabled)  {
			m_WasMouseCaptureEnabled = m_IsMouseCaptureEnabled;
			MouseCapture.FireMouseCaptureChangedCallback(m_IsMouseCaptureEnabled);
		}

		m_MousePosition.x = nativeHandle.Call<float>("GetMousePosX");
		m_MousePosition.y = -nativeHandle.Call<float>("GetMousePosY");
		m_MouseDelta.x = nativeHandle.Call<float>("GetMouseDeltaX");
		m_MouseDelta.y = -nativeHandle.Call<float>("GetMouseDeltaY");
		m_MouseScroll = nativeHandle.Call<float>("GetMouseScroll");

		m_MouseButtonLeft.Update(nativeHandle.Call<bool>("GetMouseButtonLeft"));
		m_MouseButtonRight.Update(nativeHandle.Call<bool>("GetMouseButtonRight"));
		m_MouseButtonMiddle.Update(nativeHandle.Call<bool>("GetMouseButtonMiddle"));

		nativeHandle.Call("FinishFrame");
	}

	public void EnableMouseCapture(EnableMouseCaptureFinished finished)  {
		if (!m_IsInitialized)  {
			finished?.Invoke(false, "Android API not initialized");
			return;
		}

		if (m_IsInitializationFailed)  {
			finished?.Invoke(false, "Android API failed to initialize");
			return;
		}

		if (m_EnableMouseCaptureFinished != null)  {
			finished?.Invoke(false, "EnableMouseCapture already called");
			return;
		}

		m_EnableMouseCaptureFinished = finished;
		nativeHandle.Call("EnableMouseCapture");
	}

	public void DisableMouseCapture()  {
		if (m_IsInitialized) nativeHandle.Call("DisableMouseCapture");
	}

	public Vector2 GetMousePosition()  {
		return m_MousePosition;
	}
	
	public Vector2 GetMouseDelta() {
		return m_MouseDelta;
	}

	public Vector2 GetMouseScroll()  {
		return new Vector2(0, m_MouseScroll);
	}

	public bool GetMouseButton(MouseButton.Location location)  {
		switch (location)  {
			case MouseButton.Location.Left: return m_MouseButtonLeft.IsPressed;
			case MouseButton.Location.Right: return m_MouseButtonRight.IsPressed;
			case MouseButton.Location.Middle: return m_MouseButtonMiddle.IsPressed;
		}
		return false;
	}
}