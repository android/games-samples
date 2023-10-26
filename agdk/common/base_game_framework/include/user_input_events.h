/*
 * Copyright 2023 Google LLC
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

#ifndef BASEGAMEFRAMEWORK_USER_INPUT_EVENTS_H_
#define BASEGAMEFRAMEWORK_USER_INPUT_EVENTS_H_

#include <cstdint>

#include "platform_keycodes.h"

namespace base_game_framework {

/** @brief Enum of possible key event actions */
enum KeyEventAction : int32_t {
  kKeyEvent_Down = 0, ///< Key press down
  kKeyEvent_Up ///< Key released up
};

/**
 * @brief Key modifiers defined as bitmask values.
 * AND against `KeyEvent.key_modifiers` to check for modifier status.
 */
enum KeyEventModifiers : uint32_t {
  kKeyModifier_None = 0, ///< No modifier keys down
  kKeyModifier_Shift = 0x1, ///< Shift modifier key down
  kKeyModifier_Alt = 0x2, ///< Alt modifier key down
  kKeyModifier_Sym = 0x4, ///< Sym modifier key down
  kKeyModifier_Fn = 0x8, ///< Fn modifier key down
  kKeyModifier_Alt_Left = 0x10, ///< Left Alt modifier key down
  kKeyModifier_Alt_Right = 0x20, ///< Right Alt modifier key down
  kKeyModifier_Shift_Left = 0x40, ///< Left Shift modifier key down
  kKeyModifier_Shift_Right = 0x80, ///< Right Shift modifier key down
  kKeyModifier_Ctrl = 0x1000, ///< Ctrl Shift modifier key down
  kKeyModifier_Ctrl_Left = 0x2000, ///< Left Ctrl modifier key down
  kKeyModifier_Ctrl_Right = 0x4000, ///< Right Ctrl modifier key down
  kKeyModifier_Meta = 0x10000, ///< Meta modifier key down
  kKeyModifier_Meta_Left = 0x20000, ///< Left Meta modifier key down
  kKeyModifier_Meta_Right = 0x40000, ///< Right Meta modifier key down
  kKeyModifier_Caps_Lock = 0x100000, ///< Caps Lock modifier key down
  kKeyModifier_Num_Lock = 0x200000, ///< Num Lock modifier key down
  kKeyModifier_Scroll_Lock = 0x400000 ///< Scroll lock modifier key down
};

/** @brief Type definition for key event key code */
typedef int32_t KeyCode;

/** @brief Structure defining the data associated with a key event */
struct KeyEvent {
  KeyEventAction key_action; ///< The type of key event that occurred
  KeyEventModifiers key_modifiers; ///< Bitmask of pressed modifier keys during the event
  KeyCode key_code; ///< Keycode of the key associated with the event
  int32_t key_repeat_count; ///< Repeat press count of the event
  int64_t event_time; ///< Timestamp of the event start, in milliseconds since system up
  int64_t down_time; ///< Most recent down press of the event, in milliseconds since system up
};

/** @brief Enum of possible touch event actions */
enum TouchEventAction : int32_t {
  kTouch_Down = 0, ///< Touch down (began)
  kTouch_Up, ///< Touch up (ended)
  kTouch_Moved ///< Touch moved
};

/** @brief Structure defining the data associated with a touch event */
struct TouchEvent {
  TouchEventAction touch_action; ///< The type of touch event that occurred
  int32_t touch_id; ///< The 'serial' number of the touch event
  int32_t touch_x; ///< The X position of the touch, in screen pixel coordinates
  int32_t touch_y; ///< The Y position of the touch, in screen pixel coordinates
};

/** @brief Enum of possible mouse event actions */
enum MouseEventAction : int32_t {
  kMouse_Button_Down = 0, ///< Mouse button down
  kMouse_Button_Up, ///< Mouse button up
  kMouse_Moved, ///< Mouse moved: delta_x/delta_y have changes
  kMouse_Scroll_Wheel ///< Scroll wheel moved: delta_x/delta_y have changes
};

/** @brief Structure defining the data associated with a mouse event */
struct MouseEvent {
  MouseEventAction mouse_action; ///< The type of mouse event that occurred
  uint32_t mouse_index; ///< The index of the mouse associated with the event
  uint32_t mouse_buttons; ///< Bitmask of currently down mouse buttons
  int32_t mouse_x; ///< The X position of the mouse, in screen pixel coordinates
  int32_t mouse_y; ///< The Y position of the mouse, in screen pixel coordinates
  int32_t delta_x; ///< The delta X change of the mouse, or scroll wheel
  int32_t delta_y; ///< The delta X change of the mouse, or scroll wheel
};

} // namespace base_game_framework

#endif //BASEGAMEFRAMEWORK_USER_INPUT_EVENTS_H_
