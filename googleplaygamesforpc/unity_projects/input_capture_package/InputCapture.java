package com.unity.InputCapture;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.KeyEvent;


public class InputCapture
{
	class OnCapturedPointerListener implements View.OnCapturedPointerListener {
         public float posX, posY, deltaX, deltaY, scroll;

        public boolean leftButton, rightButton, middleButton;
        public boolean leftButtonDown, rightButtonDown, middleButtonDown;

        @Override
        public boolean onCapturedPointer(View view, MotionEvent event) {
            int buttonNumber = event.getActionButton();
            switch (event.getActionMasked()) {
                case MotionEvent.ACTION_MOVE:
                    posX += event.getRawX();
                    posY += event.getRawY();

                    // sum up deltas in case we get more than 1 per frame
                    deltaX += event.getRawX() * event.getXPrecision();
                    deltaY += event.getRawY() * event.getYPrecision();
                    return true;
                    
                case MotionEvent.ACTION_SCROLL:
                    scroll = event.getAxisValue(MotionEvent.AXIS_VSCROLL);
                    return true;

                case MotionEvent.ACTION_BUTTON_PRESS:
                    if (buttonNumber == 1) {
                        leftButton = true;
                        leftButtonDown = true;
                    }
                    if (buttonNumber == 2) {
                        rightButton = true;
                        rightButtonDown = true;
                    }
                    if (buttonNumber >= 3) {
                        middleButton = true;
                        middleButtonDown = true;
                    }
                    break;

                case MotionEvent.ACTION_BUTTON_RELEASE:
                    if (buttonNumber == 1) leftButton = false;
                    if (buttonNumber == 2) rightButton = false;
                    if (buttonNumber >= 3) middleButton = false;
                    break;
                    

                case MotionEvent.ACTION_HOVER_ENTER:
                    Log.d("MA", "Mouse entered at " + event.getX() + ", " + event.getY());
                    break;
                case MotionEvent.ACTION_HOVER_EXIT:
                    Log.d("MA", "Mouse exited at " + event.getX() + ", " + event.getY());
                    break;
                case MotionEvent.ACTION_HOVER_MOVE:
                    Log.d("MA", "Mouse hovered at " + event.getX() + ", " + event.getY());
                    break;
            }
            return false;
        }
    }
    

    private OnCapturedPointerListener onCapturedPointerListener = new OnCapturedPointerListener();

    private boolean init, initSuccess;
    private boolean enableMouseCaptureFinished, enableMouseCaptureSuccess;
    private View inputView;
    private Activity unityActivity;


    public boolean InitCheck() {
         return init;
    }

    public boolean InitSuccessCheck() {
         return initSuccess;
    }

    public void Init(Activity unityActivity) {
         if (init) return;

        this.unityActivity = unityActivity;
        unityActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Window window = unityActivity.getWindow();
                inputView = window.getCurrentFocus();
                if (inputView == null) {
                    initSuccess = false;
                    init = true;
                    return;
                }
                inputView.setOnCapturedPointerListener(onCapturedPointerListener);

                initSuccess = true;
                init = true;
            }
        });
    }

    public boolean EnableMouseCaptureFinishedCheck() {
         return enableMouseCaptureFinished;
    }

    public boolean EnableMouseCaptureSuccessCheck() {
         return enableMouseCaptureSuccess;
    }

    public void EnableMouseCapture() {
         if (!init || inputView == null) {
            enableMouseCaptureSuccess = false;
            enableMouseCaptureFinished = true;
            return;
        }

        unityActivity.runOnUiThread(new Runnable() {
            @Override
            public void run()
            {
                try {
                    inputView.requestPointerCapture();
                }
                catch (Exception e) {
                    enableMouseCaptureSuccess = false;
                    enableMouseCaptureFinished = true;
                    return;
                }

                enableMouseCaptureSuccess = true;
                enableMouseCaptureFinished = true;
            }
        });
    }

    public void DisableMouseCapture() {
        if (!init || inputView == null) return;

        unityActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    inputView.releasePointerCapture();
                }
                catch (Exception e) {
                    Log.e("", e.getMessage());
                }
            }
        });
    }

    private boolean m_IsMouseCaptureEnabled, m_IsMouseCaptureEnabledChecking;
    private Runnable checkMouseCaptureStateRunnable;
    public boolean IsMouseCaptureEnabled() {
        if (unityActivity == null || inputView == null) return false;
        if (checkMouseCaptureStateRunnable == null) {
            checkMouseCaptureStateRunnable = new Runnable()
            {
                @Override
                public void run() {
                    try {
                        m_IsMouseCaptureEnabled = inputView != null && inputView.hasPointerCapture();
                    }
                    catch (Exception e) {
                        Log.e("", e.getMessage());
                    }
                    m_IsMouseCaptureEnabledChecking = false;
                }
            };
        }

        if (!m_IsMouseCaptureEnabledChecking) {
            m_IsMouseCaptureEnabledChecking = true;
            unityActivity.runOnUiThread(checkMouseCaptureStateRunnable);
        }

        return m_IsMouseCaptureEnabled;
    }
    

    public float GetMousePosX() {
        float result = onCapturedPointerListener.posX;
        return result;
    }

    public float GetMousePosY() {
        float result = onCapturedPointerListener.posY;
        return result;
    }
    
    public float GetMouseDeltaX() {
            float result = onCapturedPointerListener.deltaX;
            return result;
        }
    
        public float GetMouseDeltaY() {
            float result = onCapturedPointerListener.deltaY;
            return result;
        }

    public float GetMouseScroll() {
        float result = onCapturedPointerListener.scroll;
        onCapturedPointerListener.scroll = 0;
        return result;
    }

    public boolean GetMouseButtonLeft() {
        if (onCapturedPointerListener.leftButtonDown) {
            onCapturedPointerListener.leftButtonDown = false;
            return true;
        }
        return onCapturedPointerListener.leftButton;
    }

    public boolean GetMouseButtonRight() {
        if (onCapturedPointerListener.rightButtonDown) {
            onCapturedPointerListener.rightButtonDown = false;
            return true;
        }
        return onCapturedPointerListener.rightButton;
    }

    public boolean GetMouseButtonMiddle() {
        if (onCapturedPointerListener.middleButtonDown) {
            onCapturedPointerListener.middleButtonDown = false;
            return true;
        }
        return onCapturedPointerListener.middleButton;
    }

    public void FinishFrame() {
        // clear deltas so we properly go to 0 if we get no updates
        onCapturedPointerListener.deltaX = 0;
        onCapturedPointerListener.deltaY = 0;
    }
}