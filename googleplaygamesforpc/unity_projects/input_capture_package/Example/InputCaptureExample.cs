using System;
using UnityEngine;

public class InputCaptureExample : MonoBehaviour
{
	private const string enableMouseCaptureText = "Hit Space to Enable Capture";
	private const string disableMouseCaptureText = "Hit Space to Disable Capture. Hit Return to reset pointer location";

	public UnityEngine.UI.Image cursor;
	private RectTransform picture;
	private Vector2 m_MousePosition = Vector2.zero;

	private string labelText;
	private GUIStyle textStyle;

	private void Start()  {
		labelText = enableMouseCaptureText;
		MouseCapture.MouseCaptureChangedCallback += InputCapture_MouseCaptureChangedCallback;
		picture = cursor.GetComponent<RectTransform>();
		textStyle = new GUIStyle { fontSize = 32 };
	}

	private void OnDestroy()  {
		MouseCapture.MouseCaptureChangedCallback -= InputCapture_MouseCaptureChangedCallback;
	}
	
	private void InputCapture_MouseCaptureChangedCallback(bool mouseCaptured) {
		labelText = mouseCaptured ? disableMouseCaptureText : enableMouseCaptureText;
		m_MousePosition = Vector2.zero;
	}
	
	private void SetMousePointerLocation(Vector2 newLocation) {
		if (MouseCapture.isMouseCaptureEnabled)  {
			m_MousePosition = newLocation;
		}
	}

	private void OnGUI()  {
		if (Input.GetKey(KeyCode.Space) || Input.GetKey("space"))  {
			if (!MouseCapture.isMouseCaptureEnabled)  {
				MouseCapture.EnableMouseCapture(EnableMouseCaptureFinished);
			}
			else  {
				MouseCapture.DisableMouseCapture();
				labelText = enableMouseCaptureText;
			}
		}
		if (Input.GetKey(KeyCode.Return) || Input.GetKey("return"))  {
			SetMousePointerLocation(Vector2.zero);
		}

		var mouseDelta = MouseCapture.GetMouseDelta();
		var futureMousePosition = m_MousePosition + mouseDelta;

		if (Math.Abs(futureMousePosition.x) < Screen.width * 0.5f && Math.Abs(futureMousePosition.y) < Screen.height * 0.5f)  {
			if (mouseDelta.x != 0 && mouseDelta.y != 0) m_MousePosition += mouseDelta;
			picture.anchoredPosition = new Vector2(m_MousePosition.x, m_MousePosition.y);
		}

		string text = "";
		text += labelText + Environment.NewLine;
		var scroll = MouseCapture.GetMouseScroll();
		if (scroll != 0) text += $" (Scroll: {scroll})";
		if (MouseCapture.GetMouseButton(MouseButton.Location.Left)) text += " (MouseButtonLeft)";
		else if (MouseCapture.GetMouseButton(MouseButton.Location.Right)) text += " (MouseButtonRight)";
		else if (MouseCapture.GetMouseButton(MouseButton.Location.Middle)) text += " (MouseButtonMiddle)";
		
		GUI.Label(new Rect(40, 40, 800, 256), text, textStyle);
	}

	private void EnableMouseCaptureFinished(bool success, string message)  {
		if (success) labelText = disableMouseCaptureText;
		else labelText = enableMouseCaptureText + " Error";
	}
}