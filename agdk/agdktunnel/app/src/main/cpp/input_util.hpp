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

#ifndef agdktunnel_input_util_hpp
#define agdktunnel_input_util_hpp

#include "engine.hpp"
#include "our_key_codes.hpp"

// event type
#define COOKED_EVENT_TYPE_JOY 0
#define COOKED_EVENT_TYPE_POINTER_DOWN 1
#define COOKED_EVENT_TYPE_POINTER_UP 2
#define COOKED_EVENT_TYPE_POINTER_MOVE 3
#define COOKED_EVENT_TYPE_KEY_DOWN 4
#define COOKED_EVENT_TYPE_KEY_UP 5
#define COOKED_EVENT_TYPE_BACK 6
#define COOKED_EVENT_TYPE_TEXT_INPUT 7

#define KEY_ACTION_DOWN 0
#define KEY_ACTION_UP 1

// Keycode for movement
#define KEYCODE_W 51
#define KEYCODE_A 29
#define KEYCODE_S 47
#define KEYCODE_D 32

#define KEY_CONTROL_VERTICAL_SENSIVITY 20.0f
#define KEY_CONTROL_HORIZONTAL_SENSIVITY 30.0f

#define UP_MOVEMENT_BIT 1
#define LEFT_MOVEMENT_BIT 2
#define DOWN_MOVEMENT_BIT 4
#define RIGHT_MOVEMENT_BIT 8

struct CookedEvent {
    int type;

    // for joystick events:
    float joyX, joyY;

    // for pointer events
    int motionPointerId;
    bool motionIsOnScreen;
    float motionX, motionY;
    float motionMinX, motionMaxX;
    float motionMinY, motionMaxY;

    // for key events
    int keyCode;

    // whether a text input has occurred
    bool textInputState;
};

typedef bool (*CookedEventCallback)(struct CookedEvent *event);

bool CookGameActivityKeyEvent(GameActivityKeyEvent *keyEvent, CookedEventCallback callback);

bool
CookGameActivityMotionEvent(GameActivityMotionEvent *motionEvent, CookedEventCallback callback);

bool CookGameControllerEvent(const int32_t gameControllerIndex, CookedEventCallback callback);

bool isMovementKey(const int32_t keyCode);

#endif
