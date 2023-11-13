/*
 * Copyright 2022 The Android Open Source Project
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

#include "input_util.h"

#include "util.h"

static bool _init_done = false;

static void _init() {
  if (_init_done) {
    return;
  }

  _init_done = true;
}

static bool CookEvent_Motion(AInputEvent *event, CookedEventCallback callback) {
  int src = AInputEvent_getSource(event);

  if (src != AINPUT_SOURCE_TOUCHSCREEN) {
    return false;
  }

  int action = AMotionEvent_getAction(event);
  int actionMasked = action & AMOTION_EVENT_ACTION_MASK;
  int ptrIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                 AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

  struct CookedEvent ev;
  memset(&ev, 0, sizeof(ev));

  if (actionMasked == AMOTION_EVENT_ACTION_DOWN ||
      actionMasked == AMOTION_EVENT_ACTION_POINTER_DOWN) {
    ev.type_ = COOKED_EVENT_TYPE_POINTER_DOWN;
  } else if (actionMasked == AMOTION_EVENT_ACTION_UP ||
             actionMasked == AMOTION_EVENT_ACTION_POINTER_UP) {
    ev.type_ = COOKED_EVENT_TYPE_POINTER_UP;
  } else {
    ev.type_ = COOKED_EVENT_TYPE_POINTER_MOVE;
  }

  ev.motion_pointer_id_ = AMotionEvent_getPointerId(event, ptrIndex);
  ev.motion_is_on_screen_ = (src == AINPUT_SOURCE_TOUCHSCREEN);
  ev.motion_x_ = AMotionEvent_getX(event, ptrIndex);
  ev.motion_y_ = AMotionEvent_getY(event, ptrIndex);

  // use screen size as the motion range
  ev.motion_min_x_ = 0.0f;
  ev.motion_max_x_ = SceneManager::GetInstance()->GetScreenWidth();
  ev.motion_min_y_ = 0.0f;
  ev.motion_max_y_ = SceneManager::GetInstance()->GetScreenHeight();

  // deliver event
  callback(&ev);

  // deliver motion info about other pointers (for multi-touch)
  int ptrCount = AMotionEvent_getPointerCount(event);
  for (int i = 0; i < ptrCount; i++) {
    ev.type_ = COOKED_EVENT_TYPE_POINTER_MOVE;
    ev.motion_x_ = AMotionEvent_getX(event, i);
    ev.motion_y_ = AMotionEvent_getY(event, i);
    ev.motion_pointer_id_ = AMotionEvent_getPointerId(event, i);
    callback(&ev);
  }

  return true;
}

bool CookEvent(AInputEvent *event, CookedEventCallback callback) {
  int type = AInputEvent_getType(event);

  if (!_init_done) {
    _init();
    _init_done = true;
  }

  if (type == AINPUT_EVENT_TYPE_MOTION) {
    return CookEvent_Motion(event, callback);
  }

  return false;
}
