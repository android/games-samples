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

#include "platform_events_android.h"
#include <cmath>
#include "platform_defines.h"
#include "platform_system_event_dispatch.h"
#include "platform_user_input_dispatch.h"
#include "platform_util_android.h"
#include "debug_manager.h"
#include "display_manager.h"
#include "system_event_manager.h"
#include "user_input_manager.h"

namespace base_game_framework {

// Debug names for NativeAppGlueAppCmd values from android_native_app_glue.h.
// A switch on the enum constants (rather than an array indexed by value) can't read out
// of bounds or drift out of order. It deliberately has no default case, so -Wswitch
// flags any command added by a future GameActivity release.
static const char *GetGlueCommandName(const int32_t cmd) {
#define BGF_GLUE_COMMAND_CASE(command) case command: return #command
  switch (static_cast<NativeAppGlueAppCmd>(cmd)) {
    BGF_GLUE_COMMAND_CASE(UNUSED_APP_CMD_INPUT_CHANGED);
    BGF_GLUE_COMMAND_CASE(APP_CMD_INIT_WINDOW);
    BGF_GLUE_COMMAND_CASE(APP_CMD_TERM_WINDOW);
    BGF_GLUE_COMMAND_CASE(APP_CMD_WINDOW_RESIZED);
    BGF_GLUE_COMMAND_CASE(APP_CMD_WINDOW_REDRAW_NEEDED);
    BGF_GLUE_COMMAND_CASE(APP_CMD_CONTENT_RECT_CHANGED);
    BGF_GLUE_COMMAND_CASE(APP_CMD_SOFTWARE_KB_VIS_CHANGED);
    BGF_GLUE_COMMAND_CASE(APP_CMD_GAINED_FOCUS);
    BGF_GLUE_COMMAND_CASE(APP_CMD_LOST_FOCUS);
    BGF_GLUE_COMMAND_CASE(APP_CMD_CONFIG_CHANGED);
    BGF_GLUE_COMMAND_CASE(APP_CMD_LOW_MEMORY);
    BGF_GLUE_COMMAND_CASE(APP_CMD_START);
    BGF_GLUE_COMMAND_CASE(APP_CMD_RESUME);
    BGF_GLUE_COMMAND_CASE(APP_CMD_SAVE_STATE);
    BGF_GLUE_COMMAND_CASE(APP_CMD_PAUSE);
    BGF_GLUE_COMMAND_CASE(APP_CMD_STOP);
    BGF_GLUE_COMMAND_CASE(APP_CMD_DESTROY);
    BGF_GLUE_COMMAND_CASE(APP_CMD_WINDOW_INSETS_CHANGED);
    BGF_GLUE_COMMAND_CASE(APP_CMD_EDITOR_ACTION);
    BGF_GLUE_COMMAND_CASE(APP_CMD_KEY_EVENT);
    BGF_GLUE_COMMAND_CASE(APP_CMD_TOUCH_EVENT);
  }
#undef BGF_GLUE_COMMAND_CASE
  return "UNKNOWN_APP_CMD";
}

void PlatformEventAndroid::ProcessApplicationEvent(struct android_app *app, int32_t cmd) {
  const char *cmdString = GetGlueCommandName(cmd);

  DebugManager::Log(DebugManager::kLog_Channel_Default,
                    DebugManager::kLog_Level_Debug,
                    BGM_CLASS_TAG,
                    "Received command (%d) %s:", cmd, cmdString);

  switch (cmd) {
    case UNUSED_APP_CMD_INPUT_CHANGED:break;
    case APP_CMD_INIT_WINDOW: {
      PlatformUtilAndroid::SetNativeWindow(app->window);
      if (app->savedStateSize > 0 && app->savedState != nullptr) {
        SystemEventManager::SaveState save_state{app->savedState, app->savedStateSize};
        PlatformSystemEventDispatch::DispatchReadSaveStateEvent(save_state);
      }
      if (app->window != nullptr) {
        DisplayManager::GetInstance().HandlePlatformDisplayChange(
            DisplayManager::kDisplay_Change_Window_Init);
      }
    }
      break;
    case APP_CMD_TERM_WINDOW: {
      // No more input events come for the window, e.g. the release of a mouse button held
      // (losing focus doesn't need this: the window still gets the gesture's events)
      ReleaseMouse();
      DisplayManager::GetInstance().HandlePlatformDisplayChange(
          DisplayManager::kDisplay_Change_Window_Terminate);
      PlatformUtilAndroid::SetNativeWindow(nullptr);
    }
      break;
    case APP_CMD_WINDOW_RESIZED: {
      DisplayManager::GetInstance().HandlePlatformDisplayChange(
          DisplayManager::kDisplay_Change_Window_Resized);
    }
      break;
    case APP_CMD_WINDOW_REDRAW_NEEDED: {
      DisplayManager::GetInstance().HandlePlatformDisplayChange(
          DisplayManager::kDisplay_Change_Window_Redraw_Needed);
    }
      break;
    case APP_CMD_CONTENT_RECT_CHANGED: {
      DisplayManager::GetInstance().HandlePlatformDisplayChange(
          DisplayManager::kDisplay_Change_Window_Content_Rect_Changed);
    }
      break;
    case APP_CMD_GAINED_FOCUS:PlatformUtilAndroid::SetHasFocus(true);
      PlatformSystemEventDispatch::DispatchFocusEvent(SystemEventManager::kMadeForeground);
      break;
    case APP_CMD_LOST_FOCUS:PlatformUtilAndroid::SetHasFocus(false);
      PlatformSystemEventDispatch::DispatchFocusEvent(SystemEventManager::kSentToBackground);
      break;
    case APP_CMD_CONFIG_CHANGED:
      // TODO: think about which changes we actually need to handle that aren't covered by other
      // existing commands
      break;
    case APP_CMD_LOW_MEMORY:PlatformSystemEventDispatch::DispatchMemoryEvent(SystemEventManager::kMemoryWarningCritical);
      break;
    case APP_CMD_START:UserInputManager::GetInstance().GetGameControllerManager().OnStart();
      PlatformSystemEventDispatch::DispatchLifecycleEvent(SystemEventManager::kLifecycleStart);
      break;
    case APP_CMD_RESUME:PlatformSystemEventDispatch::DispatchLifecycleEvent(SystemEventManager::kLifecycleResume);
      break;
    case APP_CMD_SAVE_STATE:PlatformSystemEventDispatch::DispatchLifecycleEvent(SystemEventManager::kLifecycleSaveState);
      break;
    case APP_CMD_PAUSE:PlatformSystemEventDispatch::DispatchLifecycleEvent(SystemEventManager::kLifecyclePause);
      break;
    case APP_CMD_STOP:UserInputManager::GetInstance().GetGameControllerManager().OnStop();
      PlatformSystemEventDispatch::DispatchLifecycleEvent(SystemEventManager::kLifecycleStop);
      break;
    case APP_CMD_DESTROY:PlatformSystemEventDispatch::DispatchLifecycleEvent(SystemEventManager::kLifecycleQuit);
      break;
    case APP_CMD_WINDOW_INSETS_CHANGED: {
      DisplayManager::GetInstance().HandlePlatformDisplayChange(
          DisplayManager::kDisplay_Change_Window_Insets_Changed);
    }
      break;
    default:break;
  }
}

void PlatformEventAndroid::ProcessKeyEvent(struct android_app *app,
                                           const GameActivityKeyEvent &event) {

  KeyEvent key_event;
  key_event.key_action = static_cast<KeyEventAction>(event.action);
  key_event.key_modifiers = static_cast<KeyEventModifiers>(event.modifiers);
  key_event.key_code = static_cast<KeyCode>(event.keyCode);
  key_event.key_repeat_count = event.repeatCount;
  key_event.event_time = event.eventTime;
  key_event.down_time = event.downTime;
  PlatformUserInputDispatch::DispatchKeyEvent(key_event);
}

namespace {
// True if source includes all the bits of source type (a source type includes its class bits,
// e.g. a stylus on a touchscreen reports AINPUT_SOURCE_TOUCHSCREEN | AINPUT_SOURCE_STYLUS)
bool IsSourceType(const int32_t source, const int32_t source_type) {
  return (source & source_type) == source_type;
}

// AMOTION_EVENT_BUTTON_PRIMARY .. AMOTION_EVENT_BUTTON_FORWARD are the MouseButton bits
constexpr uint32_t kMouseButtonMask = kMouse_Button_Left | kMouse_Button_Right |
                                      kMouse_Button_Middle | kMouse_Button_Back |
                                      kMouse_Button_Forward;

// State of the mouse, and of the touch emulated with its left button
struct MouseState {
  uint32_t buttons = 0;
  int32_t x = 0;
  int32_t y = 0;
  // False until the first event, and after the pointer left the window: no move is reported
  // from the previous position
  bool has_position = false;
  // A press without a button state (e.g. a touchpad tap, or injected events) is the left
  // button, held until the gesture ends
  bool buttonless_press = false;
  bool emulating_touch = false;
  int32_t touch_id = 0;
  // Scroll amounts not yet reported, less than a notch
  float scroll_x = 0.0f;
  float scroll_y = 0.0f;
};
MouseState mouse_state;

void DispatchMouseTouch(const TouchEventAction action, const int32_t touch_id, const int32_t x,
                        const int32_t y) {
  TouchEvent touch_event;
  touch_event.touch_action = action;
  touch_event.touch_id = touch_id;
  touch_event.touch_x = x;
  touch_event.touch_y = y;
  PlatformUserInputDispatch::DispatchTouchEvent(touch_event);
}

bool DispatchMouse(const MouseEventAction action, const uint32_t buttons, const int32_t x,
                   const int32_t y, const int32_t delta_x, const int32_t delta_y) {
  MouseEvent mouse_event;
  mouse_event.mouse_action = action;
  mouse_event.mouse_index = 0;
  mouse_event.mouse_buttons = buttons;
  mouse_event.mouse_x = x;
  mouse_event.mouse_y = y;
  mouse_event.delta_x = delta_x;
  mouse_event.delta_y = delta_y;
  return PlatformUserInputDispatch::DispatchMouseEvent(mouse_event);
}

// Releases the buttons in `released` (reporting the buttons still down), ending the emulated
// touch if the left button is released
void ReleaseMouseButtons(MouseState &state, const uint32_t released) {
  if (released == 0) {
    return;
  }
  state.buttons &= ~released;
  DispatchMouse(kMouse_Button_Up, state.buttons, state.x, state.y, 0, 0);
  if ((released & kMouse_Button_Left) != 0 && state.emulating_touch) {
    state.emulating_touch = false;
    DispatchMouseTouch(kTouch_Up, state.touch_id, state.x, state.y);
  }
}

// Adds a scroll axis value to the amount not yet reported, returning the whole notches to
// report: smooth scrolling (touchpads, high resolution wheels) reports fractions of a notch
int32_t AccumulateScroll(float &accumulated, const float value) {
  if (std::isnan(value)) {
    return 0;
  }
  if ((value > 0.0f && accumulated < 0.0f) || (value < 0.0f && accumulated > 0.0f)) {
    accumulated = 0.0f;  // Direction reversed: drop the remainder of the other direction
  }
  // (Clamped so the notches convert to int)
  accumulated = std::fmax(-1.0e6f, std::fmin(accumulated + value, 1.0e6f));
  // (With a little tolerance, so fractions adding up to a notch report it despite rounding)
  const float notches = std::trunc(accumulated + std::copysign(1.0e-3f, accumulated));
  accumulated -= notches;
  return static_cast<int32_t>(notches);
}

// Float screen coordinate to int (NaN as 0, out of range values clamped)
int32_t ToCoordinate(const float value) {
  if (std::isnan(value)) {
    return 0;
  }
  return static_cast<int32_t>(std::fmax(-1.0e9f, std::fmin(value, 1.0e9f)));
}

// Touchscreen or stylus: one touch per pointer
void ProcessTouchscreenEvent(const GameActivityMotionEvent &event) {
  const int action = event.action;
  const int action_masked = action & AMOTION_EVENT_ACTION_MASK;
  const uint32_t action_index = static_cast<uint32_t>(
      (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
      AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
  // The pointers to report, and as what
  uint32_t first = 0;
  uint32_t count = 0;
  TouchEventAction touch_action = kTouch_Moved;
  switch (action_masked) {
    case AMOTION_EVENT_ACTION_DOWN:
      count = 1;
      touch_action = kTouch_Down;
      break;
    case AMOTION_EVENT_ACTION_POINTER_DOWN:
      first = action_index;
      count = 1;
      touch_action = kTouch_Down;
      break;
    case AMOTION_EVENT_ACTION_UP:
      count = 1;
      touch_action = kTouch_Up;
      break;
    case AMOTION_EVENT_ACTION_POINTER_UP:
      first = action_index;
      count = 1;
      touch_action = kTouch_Up;
      break;
    case AMOTION_EVENT_ACTION_MOVE:
      // Includes all active pointers
      count = event.pointerCount;
      touch_action = kTouch_Moved;
      break;
    case AMOTION_EVENT_ACTION_CANCEL:
      // The gesture was aborted (e.g. taken over by a system gesture): end all its touches
      // rather than leaving them down
      count = event.pointerCount;
      touch_action = kTouch_Up;
      break;
    default:
      break;
  }
  for (uint32_t i = first; i < event.pointerCount && i - first < count; ++i) {
    TouchEvent touch_event;
    touch_event.touch_action = touch_action;
    touch_event.touch_id = event.pointers[i].id;
    touch_event.touch_x = ToCoordinate(GameActivityPointerAxes_getX(&event.pointers[i]));
    touch_event.touch_y = ToCoordinate(GameActivityPointerAxes_getY(&event.pointers[i]));
    PlatformUserInputDispatch::DispatchTouchEvent(touch_event);
  }
}

// Mouse (also a touchpad moving the mouse pointer): mouse events, and a touch while the left
// button is down
void ProcessMouseEvent(const GameActivityMotionEvent &event) {
  const GameActivityPointerAxes &pointer = event.pointers[0];
  const int32_t x = ToCoordinate(GameActivityPointerAxes_getX(&pointer));
  const int32_t y = ToCoordinate(GameActivityPointerAxes_getY(&pointer));
  const int action_masked = event.action & AMOTION_EVENT_ACTION_MASK;
  MouseState &state = mouse_state;

  switch (action_masked) {
    case AMOTION_EVENT_ACTION_SCROLL: {
      // (The scroll axes are enabled by PlatformEventLoop)
      const int32_t delta_x = AccumulateScroll(state.scroll_x,
          GameActivityPointerAxes_getAxisValue(&pointer, AMOTION_EVENT_AXIS_HSCROLL));
      const int32_t delta_y = AccumulateScroll(state.scroll_y,
          GameActivityPointerAxes_getAxisValue(&pointer, AMOTION_EVENT_AXIS_VSCROLL));
      if (delta_x != 0 || delta_y != 0) {
        DispatchMouse(kMouse_Scroll_Wheel, state.buttons, x, y, delta_x, delta_y);
      }
      return;
    }
    case AMOTION_EVENT_ACTION_OUTSIDE:
      // Only the position of a press outside the window, which isn't followed by its release
      return;
    default:
      break;
  }

  // The buttons down after this event. The button state has all the buttons (a right or
  // middle press is also a down/up gesture), and back/forward can be held while hovering.
  uint32_t buttons = static_cast<uint32_t>(event.buttonState) & kMouseButtonMask;
  bool position_valid = true;
  switch (action_masked) {
    case AMOTION_EVENT_ACTION_DOWN:
      state.buttonless_press = (event.buttonState == 0);
      break;
    case AMOTION_EVENT_ACTION_UP:
      state.buttonless_press = false;
      break;
    case AMOTION_EVENT_ACTION_CANCEL:
      // The gesture was aborted: no release follows, even if a button is still held
      state.buttonless_press = false;
      buttons = 0;
      break;
    case AMOTION_EVENT_ACTION_HOVER_ENTER:
    case AMOTION_EVENT_ACTION_HOVER_MOVE:
      // Not pressed
      state.buttonless_press = false;
      buttons &= ~(kMouse_Button_Left | kMouse_Button_Right | kMouse_Button_Middle);
      break;
    case AMOTION_EVENT_ACTION_HOVER_EXIT:
      // The pointer left the window (or is about to be pressed): moves are reported again
      // from where it next appears
      position_valid = false;
      buttons = state.buttons;
      break;
    default:
      // MOVE, and BUTTON_PRESS/BUTTON_RELEASE (sent with DOWN/MOVE/UP, same button state)
      break;
  }
  if (state.buttonless_press) {
    buttons |= kMouse_Button_Left;
  }

  if (state.has_position && (x != state.x || y != state.y)) {
    DispatchMouse(kMouse_Moved, state.buttons, x, y, x - state.x, y - state.y);
    if (state.emulating_touch) {
      DispatchMouseTouch(kTouch_Moved, state.touch_id, x, y);
    }
  }
  state.x = x;
  state.y = y;
  state.has_position = position_valid;

  ReleaseMouseButtons(state, state.buttons & ~buttons);
  const uint32_t pressed = buttons & ~state.buttons;
  if (pressed != 0) {
    state.buttons = buttons;
    const bool consumed = DispatchMouse(kMouse_Button_Down, buttons, x, y, 0, 0);
    if ((pressed & kMouse_Button_Left) != 0 && !consumed) {
      state.emulating_touch = true;
      state.touch_id = pointer.id;
      DispatchMouseTouch(kTouch_Down, state.touch_id, x, y);
    }
  }
}
} // anonymous namespace

void PlatformEventAndroid::ProcessMotionEvent(struct android_app * /*app*/,
                                              const GameActivityMotionEvent &event) {
  if (event.pointerCount == 0) {
    return;
  }
  // Screen coordinates: touchscreen and stylus (on the touchscreen, or a drawing tablet mapped
  // to the display), and mouse pointer. A touchpad in its own right (AINPUT_SOURCE_TOUCHPAD,
  // e.g. with pointer capture) reports positions on the pad, not the screen, and isn't
  // reported; as a mouse pointer it reports AINPUT_SOURCE_MOUSE.
  if (IsSourceType(event.source, AINPUT_SOURCE_TOUCHSCREEN) ||
      IsSourceType(event.source, AINPUT_SOURCE_STYLUS)) {
    ProcessTouchscreenEvent(event);
  } else if (IsSourceType(event.source, AINPUT_SOURCE_MOUSE)) {
    ProcessMouseEvent(event);
  }
}

void PlatformEventAndroid::ReleaseMouse() {
  ReleaseMouseButtons(mouse_state, mouse_state.buttons);
  mouse_state.buttonless_press = false;
  mouse_state.has_position = false;
  mouse_state.scroll_x = 0.0f;
  mouse_state.scroll_y = 0.0f;
}

void PlatformEventAndroid::ResetMouseState() {
  mouse_state = MouseState();
}

bool PlatformEventAndroid::IsMouseEvent(const GameActivityMotionEvent &event) {
  return !IsSourceType(event.source, AINPUT_SOURCE_TOUCHSCREEN) &&
         !IsSourceType(event.source, AINPUT_SOURCE_STYLUS) &&
         IsSourceType(event.source, AINPUT_SOURCE_MOUSE);
}

bool PlatformUserInputDispatch::DispatchKeyEvent(const KeyEvent &key_event) {
  return UserInputManager::GetInstance().OnKeyEvent(key_event);
}

bool PlatformUserInputDispatch::DispatchTouchEvent(const TouchEvent &touch_event) {
  return UserInputManager::GetInstance().OnTouchEvent(touch_event);
}

bool PlatformUserInputDispatch::DispatchMouseEvent(const MouseEvent &mouse_event) {
  return UserInputManager::GetInstance().OnMouseEvent(mouse_event);
}

void PlatformSystemEventDispatch::DispatchFocusEvent(
    const SystemEventManager::FocusEvent focus_event) {
  SystemEventManager::GetInstance().OnFocusEvent(focus_event);
}

void PlatformSystemEventDispatch::DispatchLifecycleEvent(
    const SystemEventManager::LifecycleEvent lifecycle_event) {
  SystemEventManager::GetInstance().OnLifecycleEvent(lifecycle_event);
}

void PlatformSystemEventDispatch::DispatchMemoryEvent(
    const SystemEventManager::MemoryWarningEvent memory_warning_event) {
  SystemEventManager::GetInstance().OnMemoryEvent(memory_warning_event);
}

void PlatformSystemEventDispatch::DispatchReadSaveStateEvent(
    const SystemEventManager::SaveState &save_state) {
  SystemEventManager::GetInstance().OnReadSaveStateEvent(save_state);
}
} // namespace base_game_framework