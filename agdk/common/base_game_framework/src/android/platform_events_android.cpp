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
#include "platform_defines.h"
#include "platform_system_event_dispatch.h"
#include "platform_user_input_dispatch.h"
#include "platform_util_android.h"
#include "debug_manager.h"
#include "display_manager.h"
#include "system_event_manager.h"
#include "user_input_manager.h"

namespace base_game_framework {

// enums from NativeAppGlueAppCmd in android_native_app_glue.h
constexpr const char *kGlueCommandStrings[] = {
    "UNUSED_APP_CMD_INPUT_CHANGED",
    "APP_CMD_INIT_WINDOW",
    "APP_CMD_TERM_WINDOW",
    "APP_CMD_WINDOW_RESIZED",
    "APP_CMD_WINDOW_REDRAW_NEEDED",
    "APP_CMD_CONTENT_RECT_CHANGED",
    "APP_CMD_GAINED_FOCUS",
    "APP_CMD_LOST_FOCUS",
    "APP_CMD_CONFIG_CHANGED",
    "APP_CMD_LOW_MEMORY",
    "APP_CMD_START",
    "APP_CMD_RESUME",
    "APP_CMD_SAVE_STATE",
    "APP_CMD_PAUSE",
    "APP_CMD_STOP",
    "APP_CMD_DESTROY",
    "APP_CMD_WINDOW_INSETS_CHANGED"
};

void PlatformEventAndroid::ProcessApplicationEvent(struct android_app *app, int32_t cmd) {
  const char *cmdString = "INVALID COMMAND";
  if (cmd >= UNUSED_APP_CMD_INPUT_CHANGED && cmd <= APP_CMD_WINDOW_INSETS_CHANGED) {
    cmdString = kGlueCommandStrings[cmd];
  }

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

void PlatformEventAndroid::ProcessMotionEvent(struct android_app *app,
                                              const GameActivityMotionEvent &event) {

  // Screen for touch events, Paddleboat will handle mouse events.
  if (event.source == AINPUT_SOURCE_TOUCHSCREEN && event.pointerCount > 0) {
    const int action = event.action;
    const int action_masked = action & AMOTION_EVENT_ACTION_MASK;
    // Initialize pointer_index to the max size, we only dispatch an
    // event at the end of the function if pointer_index is set to a valid index range
    uint32_t pointer_index = GAMEACTIVITY_MAX_NUM_POINTERS_IN_MOTION_EVENT;
    TouchEvent touch_event;

    switch (action_masked) {
      case AMOTION_EVENT_ACTION_DOWN:
        pointer_index = 0;
        touch_event.touch_action = kTouch_Down;
        break;
      case AMOTION_EVENT_ACTION_POINTER_DOWN:
        pointer_index = ((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
            >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
        touch_event.touch_action = kTouch_Down;
        break;
      case AMOTION_EVENT_ACTION_UP:pointer_index = 0;
        touch_event.touch_action = kTouch_Up;
        break;
      case AMOTION_EVENT_ACTION_POINTER_UP:
        pointer_index = ((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
            >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
        touch_event.touch_action = kTouch_Up;
        break;
      case AMOTION_EVENT_ACTION_MOVE: {
        // Move includes all active pointers, so loop and process them here,
        // we do not set pointer_index since we are dispatching the events in
        // this loop rather than at the bottom of the function
        touch_event.touch_action = kTouch_Moved;
        for (uint32_t i = 0; i < event.pointerCount; ++i) {
          touch_event.touch_id = event.pointers[i].id;
          touch_event.touch_x = GameActivityPointerAxes_getX(&event.pointers[i]);
          touch_event.touch_y = GameActivityPointerAxes_getY(&event.pointers[i]);
          PlatformUserInputDispatch::DispatchTouchEvent(touch_event);
        }
        break;
      }
      default:break;
    }
    if (pointer_index != GAMEACTIVITY_MAX_NUM_POINTERS_IN_MOTION_EVENT) {
      touch_event.touch_id = event.pointers[pointer_index].id;
      touch_event.touch_x = GameActivityPointerAxes_getX(&event.pointers[pointer_index]);
      touch_event.touch_y = GameActivityPointerAxes_getY(&event.pointers[pointer_index]);
      PlatformUserInputDispatch::DispatchTouchEvent(touch_event);
    }
  }
}

bool PlatformUserInputDispatch::DispatchKeyEvent(const KeyEvent &key_event) {
  return UserInputManager::GetInstance().OnKeyEvent(key_event);
}

bool PlatformUserInputDispatch::DispatchTouchEvent(const TouchEvent &touch_event) {
  return UserInputManager::GetInstance().OnTouchEvent(touch_event);
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