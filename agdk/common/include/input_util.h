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

#ifndef INPUT_UTIL_H_
#define INPUT_UTIL_H_

#include "engine.h"

// event type
#define COOKED_EVENT_TYPE_POINTER_DOWN 0
#define COOKED_EVENT_TYPE_POINTER_UP 1
#define COOKED_EVENT_TYPE_POINTER_MOVE 2

struct CookedEvent {
  int type_;

  // for pointer events
  int motion_pointer_id_;
  bool motion_is_on_screen_;
  float motion_x_, motion_y_;
  float motion_min_x_, motion_max_x_;
  float motion_min_y_, motion_max_y_;
};

typedef bool (*CookedEventCallback)(struct CookedEvent *event);

bool CookEvent(AInputEvent *event, CookedEventCallback callback);

#endif  // INPUT_UTIL_H_
