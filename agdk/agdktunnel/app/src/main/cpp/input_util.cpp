/*
 * Copyright 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "input_util.hpp"
#include "joystick-support.hpp"
#include "our_key_codes.hpp"
#include "util.hpp"

#include "paddleboat/paddleboat.h"

#define INPUT_ACTION_COUNT 6

struct InputAction {
    uint32_t buttonMask;
    int32_t actionCode;
};

static const InputAction _INPUT_ACTIONS[INPUT_ACTION_COUNT] = {
        {PADDLEBOAT_BUTTON_A,          OURKEY_ENTER},
        {PADDLEBOAT_BUTTON_B,          OURKEY_ESCAPE},
        {PADDLEBOAT_BUTTON_DPAD_UP,    OURKEY_UP},
        {PADDLEBOAT_BUTTON_DPAD_LEFT,  OURKEY_LEFT},
        {PADDLEBOAT_BUTTON_DPAD_DOWN,  OURKEY_DOWN},
        {PADDLEBOAT_BUTTON_DPAD_RIGHT, OURKEY_RIGHT}
};

static bool _key_state[OURKEY_COUNT] = {0};
static uint32_t _prev_buttonsDown = 0;

static void _report_key_state(int keyCode, bool state, CookedEventCallback callback) {
    bool wentDown = !_key_state[keyCode] && state;
    bool wentUp = _key_state[keyCode] && !state;
    _key_state[keyCode] = state;

    struct CookedEvent ev;
    memset(&ev, 0, sizeof(struct CookedEvent));
    ev.keyCode = keyCode;

    if (wentUp) {
        ev.type = COOKED_EVENT_TYPE_KEY_UP;
        callback(&ev);
    } else if (wentDown) {
        ev.type = COOKED_EVENT_TYPE_KEY_DOWN;
        callback(&ev);
    }
}

static void _report_key_states_from_axes(float x, float y, CookedEventCallback callback) {
    _report_key_state(OURKEY_LEFT, x < -0.5f, callback);
    _report_key_state(OURKEY_RIGHT, x > 0.5f, callback);
    _report_key_state(OURKEY_UP, y < -0.5f, callback);
    _report_key_state(OURKEY_DOWN, y > 0.5f, callback);
}

static int32_t _checkControllerButton(const uint32_t buttonsDown, const InputAction &inputAction,
                                      CookedEventCallback callback) {
    if (buttonsDown & inputAction.buttonMask) {
        _report_key_state(inputAction.actionCode, true, callback);
        return 1;
    } else if (_prev_buttonsDown & inputAction.buttonMask) {
        _report_key_state(inputAction.actionCode, false, callback);
        return 1;
    }
    return 0;
}

bool CookGameActivityKeyEvent(GameActivityKeyEvent *keyEvent, CookedEventCallback callback) {
    if (keyEvent->keyCode == AKEYCODE_BACK && 0 == keyEvent->action) {
        // back key was pressed
        struct CookedEvent ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = COOKED_EVENT_TYPE_BACK;
        return callback(&ev);
    }
    return false;
}

bool
CookGameActivityMotionEvent(GameActivityMotionEvent *motionEvent, CookedEventCallback callback) {
    if (motionEvent->pointerCount > 0) {
        int action = motionEvent->action;
        int actionMasked = action & AMOTION_EVENT_ACTION_MASK;
        int ptrIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                                 AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

        if (ptrIndex < motionEvent->pointerCount) {
            struct CookedEvent ev;
            memset(&ev, 0, sizeof(ev));

            if (actionMasked == AMOTION_EVENT_ACTION_DOWN ||
                actionMasked == AMOTION_EVENT_ACTION_POINTER_DOWN) {
                ev.type = COOKED_EVENT_TYPE_POINTER_DOWN;
            } else if (actionMasked == AMOTION_EVENT_ACTION_UP ||
                       actionMasked == AMOTION_EVENT_ACTION_POINTER_UP) {
                ev.type = COOKED_EVENT_TYPE_POINTER_UP;
            } else {
                ev.type = COOKED_EVENT_TYPE_POINTER_MOVE;
            }

            ev.motionPointerId = motionEvent->pointers[ptrIndex].id;
            ev.motionIsOnScreen = motionEvent->source == AINPUT_SOURCE_TOUCHSCREEN;
            ev.motionX = GameActivityPointerAxes_getX(&motionEvent->pointers[ptrIndex]);
            ev.motionY = GameActivityPointerAxes_getY(&motionEvent->pointers[ptrIndex]);

            if (ev.motionIsOnScreen) {
                // use screen size as the motion range
                ev.motionMinX = 0.0f;
                ev.motionMaxX = SceneManager::GetInstance()->GetScreenWidth();
                ev.motionMinY = 0.0f;
                ev.motionMaxY = SceneManager::GetInstance()->GetScreenHeight();
            }

            return callback(&ev);
        }
    }
    return false;
}

bool CookGameControllerEvent(const int32_t gameControllerIndex, CookedEventCallback callback) {
    int addedControllerEvent = 0;
    if (gameControllerIndex >= 0) {
        Paddleboat_Controller_Data controllerData;
        if (Paddleboat_getControllerData(gameControllerIndex, &controllerData) ==
            PADDLEBOAT_NO_ERROR) {
            // Generate events from buttons
            for (int i = 0; i < INPUT_ACTION_COUNT; ++i) {
                addedControllerEvent |= _checkControllerButton(controllerData.buttonsDown,
                                                               _INPUT_ACTIONS[i], callback);
            }

            // Generate an event for the stick position
            struct CookedEvent ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = COOKED_EVENT_TYPE_JOY;
            ev.joyX = controllerData.leftStick.stickX;
            ev.joyY = controllerData.leftStick.stickY;
            callback(&ev);

            // Also generate directional button events from the stick position
            _report_key_states_from_axes(ev.joyX, ev.joyY, callback);

            // Update our prev variable so we can detect delta changes from down to up
            _prev_buttonsDown = controllerData.buttonsDown;
        }
    }
    return (addedControllerEvent != 0);
}
