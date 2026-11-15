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
#include "game_controller_manager.h"
#include "debug_manager.h"
#include "platform_util_android.h"
#include "paddleboat/paddleboat.h"

namespace base_game_framework {

static_assert(GameControllerManager::kMaxControllers <= PADDLEBOAT_MAX_CONTROLLERS,
              "GameControllerManager slots must map to valid Paddleboat controller indices");

static void controller_status_callback_(const int32_t controller_index,
                                        const Paddleboat_ControllerStatus controller_status,
                                        void *user_data) {
  GameControllerManager *manager = reinterpret_cast<GameControllerManager *>(user_data);
  if (manager != nullptr) {
    // Status callbacks report JUST_CONNECTED / JUST_DISCONNECTED transitions, not just
    // ACTIVE / INACTIVE.
    switch (controller_status) {
      case PADDLEBOAT_CONTROLLER_ACTIVE:
      case PADDLEBOAT_CONTROLLER_JUST_CONNECTED:
        manager->SetControllerStatus(controller_index, GameController::kActive);
        break;
      case PADDLEBOAT_CONTROLLER_INACTIVE:
      case PADDLEBOAT_CONTROLLER_JUST_DISCONNECTED:
        manager->SetControllerStatus(controller_index, GameController::kInactive);
        break;
    }
  }
}

static void motion_data_callback_(const int32_t controller_index,
                                  const Paddleboat_Motion_Data *motion_data, void *user_data) {
  GameControllerManager *manager = reinterpret_cast<GameControllerManager*>(user_data);
  if (manager != nullptr) {
    GameController::GameControllerMotionData gc_motion_data;
    gc_motion_data.motion_type = static_cast<GameController::GameControllerMotionType>(
        motion_data->motionType);
    gc_motion_data.motion_x = motion_data->motionX;
    gc_motion_data.motion_y = motion_data->motionY;
    gc_motion_data.motion_z = motion_data->motionZ;
    gc_motion_data.timestamp = motion_data->timestamp;
    manager->MotionDataEvent(controller_index, gc_motion_data);
  }
}

GameControllerManager::GameControllerManager() {
  for (int32_t i = 0; i < kMaxControllers; ++i) {
    game_controllers_[i].controller_index_ = i;
  }
  const Paddleboat_ErrorCode
      error_code = Paddleboat_init(PlatformUtilAndroid::GetMainThreadJNIEnv(),
                                   PlatformUtilAndroid::GetActivityClassObject());
  if (error_code == PADDLEBOAT_NO_ERROR) {
    initialized_ = true;
    Paddleboat_setControllerStatusCallback(controller_status_callback_, this);
    // Only controller motion is reported until an integrated motion callback is set, so the
    // handset's accelerometer/gyroscope aren't sampled for apps that never use them.
    Paddleboat_setMotionDataCallback(motion_data_callback_, this);
    integrated_sensor_flags_ = Paddleboat_getIntegratedMotionSensorFlags();
    integrated_motion_data_ = (integrated_sensor_flags_ != PADDLEBOAT_INTEGRATED_SENSOR_NONE);
  } else {
    DebugManager::Log(DebugManager::kLog_Channel_Default, DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG, "Paddleboat_init failed with error code %d", error_code);
  }
}

GameControllerManager::~GameControllerManager() {
  if (initialized_) {
    Paddleboat_setMotionDataCallback(nullptr, nullptr);
    Paddleboat_setControllerStatusCallback(nullptr, nullptr);
    Paddleboat_destroy(PlatformUtilAndroid::GetMainThreadJNIEnv());
  }
}

void GameControllerManager::MotionDataEvent(const int32_t controller_index,
                                            const GameController::GameControllerMotionData &motion_data) {
  if (controller_index == PADDLEBOAT_INTEGRATED_SENSOR_INDEX &&
      integrated_motion_data_callback_ != nullptr) {
    integrated_motion_data_callback_(motion_data, integrated_user_data_);
  } else if (controller_index >= 0 && controller_index < kMaxControllers &&
      game_controller_motion_data_callback_ != nullptr) {
    game_controller_motion_data_callback_(game_controllers_[controller_index],
                                          motion_data, motion_user_data_);
  }
}

void GameControllerManager::OnStart() {
  if (initialized_) {
    Paddleboat_onStart(PlatformUtilAndroid::GetMainThreadJNIEnv());
  }
}

void GameControllerManager::OnStop() {
  if (initialized_) {
    Paddleboat_onStop(PlatformUtilAndroid::GetMainThreadJNIEnv());
  }
}

void GameControllerManager::Update() {
  if (initialized_) {
    Paddleboat_update(PlatformUtilAndroid::GetMainThreadJNIEnv());

    for (int32_t i = 0; i < kMaxControllers; ++i) {
      const Paddleboat_ControllerStatus status = Paddleboat_getControllerStatus(i);
      // Check for status mismatch if we didn't get the status callback yet, e.g. a
      // controller already connected before the callback was registered.
      const GameController::GameControllerStatus current = game_controllers_[i].controller_status_;
      if (status == PADDLEBOAT_CONTROLLER_ACTIVE && current == GameController::kInactive) {
        SetControllerStatus(i, GameController::kActive);
      } else if (status == PADDLEBOAT_CONTROLLER_INACTIVE && current == GameController::kActive) {
        SetControllerStatus(i, GameController::kInactive);
      }
    }
  }
}

void GameControllerManager::RefreshControllerData() {
  if (initialized_) {
    for (int32_t i = 0; i < kMaxControllers; ++i) {
      if (game_controllers_[i].controller_status_ == GameController::kActive) {
        UpdateControllerData(i);
      }
    }
  }
}

void GameControllerManager::UpdateControllerData(const int32_t controller_index) {
  Paddleboat_Controller_Data controller_data;
  const Paddleboat_ErrorCode error_code = Paddleboat_getControllerData(controller_index,
                                                                       &controller_data);
  if (error_code == PADDLEBOAT_NO_ERROR) {
    GameController::GameControllerData &data = game_controllers_[controller_index].controller_data_;
    data.timestamp = controller_data.timestamp;
    data.buttons_down = controller_data.buttonsDown;
    data.left_stick.stick_x = controller_data.leftStick.stickX;
    data.left_stick.stick_y = controller_data.leftStick.stickY;
    data.right_stick.stick_x = controller_data.rightStick.stickX;
    data.right_stick.stick_y = controller_data.rightStick.stickY;
    data.trigger_l1 = controller_data.triggerL1;
    data.trigger_l2 = controller_data.triggerL2;
    data.trigger_r1 = controller_data.triggerR1;
    data.trigger_r2 = controller_data.triggerR2;
    data.virtual_pointer.pointer_x = controller_data.virtualPointer.pointerX;
    data.virtual_pointer.pointer_y = controller_data.virtualPointer.pointerY;
    data.battery.battery_status =
        static_cast<GameController::GameControllerBatteryStatus>(
            controller_data.battery.batteryStatus);
    data.battery.battery_level = controller_data.battery.batteryLevel;
  }
}

void GameControllerManager::UpdateControllerInfo(const int32_t controller_index) {
  Paddleboat_Controller_Info controller_info;
  const Paddleboat_ErrorCode error_code = Paddleboat_getControllerInfo(controller_index,
                                                                       &controller_info);
  if (error_code == PADDLEBOAT_NO_ERROR) {
    GameController::GameControllerInfo &info = game_controllers_[controller_index].controller_info_;
    info.controller_flags = controller_info.controllerFlags;
    info.controller_number = controller_info.controllerNumber;
    info.vendor_id = controller_info.vendorId;
    info.product_id = controller_info.productId;
    info.device_id = controller_info.deviceId;
    info.left_stick_precision.stick_flat_x = controller_info.leftStickPrecision.stickFlatX;
    info.left_stick_precision.stick_flat_y = controller_info.leftStickPrecision.stickFlatY;
    info.left_stick_precision.stick_fuzz_x = controller_info.leftStickPrecision.stickFuzzX;
    info.left_stick_precision.stick_fuzz_y = controller_info.leftStickPrecision.stickFuzzY;
    info.right_stick_precision.stick_flat_x = controller_info.rightStickPrecision.stickFlatX;
    info.right_stick_precision.stick_flat_y = controller_info.rightStickPrecision.stickFlatY;
    info.right_stick_precision.stick_fuzz_x = controller_info.rightStickPrecision.stickFuzzX;
    info.right_stick_precision.stick_fuzz_y = controller_info.rightStickPrecision.stickFuzzY;
  }
}

GameController &GameControllerManager::GetGameController(const int32_t controller_index) {
  if (controller_index >= 0 && controller_index < GameControllerManager::kMaxControllers) {
    return game_controllers_[controller_index];
  }
  return game_controllers_[0];
}

void GameControllerManager::SetControllerStatus(const int32_t controller_index,
                                                const GameController::GameControllerStatus status) {
  if (controller_index < 0 || controller_index >= GameControllerManager::kMaxControllers) {
    return;
  }
  GameController &controller = game_controllers_[controller_index];
  if (status == GameController::kActive) {
    const bool was_active = (controller.controller_status_ == GameController::kActive);
    const int32_t previous_device_id = controller.controller_info_.device_id;
    UpdateControllerInfo(controller_index);
    if (was_active) {
      if (controller.controller_info_.device_id == previous_device_id) {
        return;  // Same controller, no change to report.
      }
      // A different controller took this slot without a disconnect being reported
      // (e.g. swapped while the app was stopped): report the old one as disconnected.
      if (status_callback_ != nullptr) {
        status_callback_(controller_index, GameController::kInactive, callback_user_data_);
      }
    }
    controller.controller_status_ = GameController::kActive;
    controller.controller_data_ = GameController::GameControllerData();
  } else {
    if (controller.controller_status_ == GameController::kInactive) {
      return;
    }
    controller.controller_status_ = GameController::kInactive;
    // Don't leave buttons/sticks held at disconnect time stuck in the slot's data.
    controller.controller_data_ = GameController::GameControllerData();
    controller.controller_info_ = GameController::GameControllerInfo();
  }
  if (status_callback_ != nullptr) {
    status_callback_(controller_index, status, callback_user_data_);
  }
}

void GameControllerManager::SetIntegratedMotionDataCallback(
    IntegratedMotionDataCallback callback, void *user_data) {
  if (!integrated_motion_data_) {
    return;
  }
  if (callback != nullptr) {
    // Store the callback before Paddleboat starts reporting integrated sensor events.
    integrated_motion_data_callback_ = callback;
    integrated_user_data_ = user_data;
    const Paddleboat_ErrorCode error = Paddleboat_setMotionDataCallbackWithIntegratedFlags(
        motion_data_callback_,
        static_cast<Paddleboat_Integrated_Motion_Sensor_Flags>(integrated_sensor_flags_), this);
    if (error != PADDLEBOAT_NO_ERROR) {
      DebugManager::Log(DebugManager::kLog_Channel_Default, DebugManager::kLog_Level_Error,
                        BGM_CLASS_TAG, "Enabling integrated motion data failed: %d", error);
      integrated_motion_data_callback_ = nullptr;
      integrated_user_data_ = nullptr;
    }
  } else {
    // Stop sampling the handset sensors before clearing the callback.
    Paddleboat_setMotionDataCallback(motion_data_callback_, this);
    integrated_motion_data_callback_ = nullptr;
    integrated_user_data_ = nullptr;
  }
}

void GameControllerManager::SetGameControllerMotionDataCallback(
    GameControllerMotionDataCallback callback, void *user_data) {
  game_controller_motion_data_callback_ = callback;
  motion_user_data_ = user_data;
}

void GameControllerManager::SetConsumeBackButton(bool consume_back_button) {
  if (initialized_) {
    Paddleboat_setBackButtonConsumed(consume_back_button);
  }
}

void GameControllerManager::SetGameControllerStatusCallback(GameControllerStatusCallback callback,
                                                            void *user_data) {
  status_callback_ = callback;
  callback_user_data_ = user_data;
}

} // namespace base_game_framework
