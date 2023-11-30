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

#include "platform_event_loop.h"
#include "debug_manager.h"
#include "game_controller_manager.h"
#include "user_input_manager.h"
#include "game-activity/native_app_glue/android_native_app_glue.h"
#include "platform_events_android.h"
#include "platform_util_android.h"

#include "paddleboat/paddleboat.h"

namespace base_game_framework {

static bool _all_motion_filter(const GameActivityMotionEvent * /*event*/) {
  // Process all motion events
  return true;
}

static void _handle_cmd_proxy(struct android_app *app, int32_t cmd) {
  PlatformEventAndroid::ProcessApplicationEvent(app, cmd);
}

static void _update_active_axis(uint64_t *active_axis) {
  // Tell GameActivity about any new axis ids so it reports
  // their events
  const uint64_t active_axis_ids = Paddleboat_getActiveAxisMask();
  uint64_t new_axis_ids = active_axis_ids ^ (*active_axis);
  if (new_axis_ids != 0) {
    *active_axis = active_axis_ids;
    int32_t current_axis_id = 0;
    while (new_axis_ids != 0) {
      if ((new_axis_ids & 1) != 0) {
        GameActivityPointerAxes_enableAxis(current_axis_id);
      }
      ++current_axis_id;
      new_axis_ids >>= 1;
    }
  }
}

std::unique_ptr<PlatformEventLoop> PlatformEventLoop::instance_ = nullptr;

PlatformEventLoop &PlatformEventLoop::GetInstance() {
  if (!instance_) {
    instance_ = std::unique_ptr<PlatformEventLoop>(new PlatformEventLoop());
  }
  return *instance_;
}

void PlatformEventLoop::ShutdownInstance() {
  PlatformEventLoop::instance_.reset();
}

PlatformEventLoop::PlatformEventLoop() {
  platform_data_ = std::unique_ptr<PlatformEventLoopData>(new PlatformEventLoopData());
  platform_data_->android_app_ = PlatformUtilAndroid::GetAndroidApp();
  platform_data_->android_app_->onAppCmd = _handle_cmd_proxy;
  platform_data_->android_app_->motionEventFilter = _all_motion_filter;
}

PlatformEventLoop::~PlatformEventLoop() {
  platform_data_.reset();
}

void PlatformEventLoop::PollEvents() {
  int events;
  android_poll_source *source;

  GameControllerManager &game_controller_manager =
      UserInputManager::GetInstance().GetGameControllerManager();
  const bool do_controller = game_controller_manager.GetControllerSupport();

  if (do_controller) {
    // Process and fire off any connection callbacks
    game_controller_manager.Update();
  }

  while ((ALooper_pollAll(0, nullptr, &events, (void **) &source)) >= 0) {
    if (source != nullptr) {
      source->process(platform_data_->android_app_, source);
    }
  }
  // Process key and motion events pending via GameActivity
  // Swap input buffers so we don't miss any events while processing inputBuffer.
  android_input_buffer *input_buffer = android_app_swap_input_buffers(platform_data_->android_app_);
  if (input_buffer != nullptr) {
    if (input_buffer->keyEventsCount != 0) {
      for (uint64_t i = 0; i < input_buffer->keyEventsCount; ++i) {
        GameActivityKeyEvent *keyEvent = &input_buffer->keyEvents[i];
        bool controller_ate_key = false;
        if (do_controller) {
          controller_ate_key = Paddleboat_processGameActivityKeyInputEvent(keyEvent,
                                                                           sizeof(GameActivityKeyEvent));
        }
        if (!controller_ate_key) {
          PlatformEventAndroid::ProcessKeyEvent(platform_data_->android_app_, *keyEvent);
        }
      }
      android_app_clear_key_events(input_buffer);
    }
    if (input_buffer->motionEventsCount != 0) {
      for (uint64_t i = 0; i < input_buffer->motionEventsCount; ++i) {
        GameActivityMotionEvent *motionEvent = &input_buffer->motionEvents[i];
        bool controller_ate_motion = false;
        if (do_controller) {
          controller_ate_motion = Paddleboat_processGameActivityMotionInputEvent(motionEvent,
                                                                                 sizeof(GameActivityMotionEvent));
        }
        if (!controller_ate_motion) {
          PlatformEventAndroid::ProcessMotionEvent(platform_data_->android_app_, *motionEvent);
        }
      }
      android_app_clear_motion_events(input_buffer);
    }
    _update_active_axis(&platform_data_->active_axis_);
  }

  if (do_controller) {
    // Update our controller data after reading any new key or motion events that
    // might have been tied to controllers
    game_controller_manager.RefreshControllerData();
  }

}

} // namespace base_game_framework
