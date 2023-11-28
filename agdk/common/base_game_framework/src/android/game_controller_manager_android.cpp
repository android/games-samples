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

static void controller_status_callback_(const int32_t controller_index,
                                        const Paddleboat_ControllerStatus controller_status,
                                        void *user_data) {
  GameControllerManager *manager = reinterpret_cast<GameControllerManager *>(user_data);
  if (manager != nullptr) {
    GameController::GameControllerStatus status =
        controller_status == PADDLEBOAT_CONTROLLER_ACTIVE ?
        GameController::kActive : GameController::kInactive;
    manager->SetControllerStatus(static_cast<uint32_t>(controller_index), status);
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
    Paddleboat_setMotionDataCallbackWithIntegratedFlags(motion_data_callback_,
    Paddleboat_getIntegratedMotionSensorFlags(), this);
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
      // Check for status mismatch if we didn't get the status callback yet
      if (status == PADDLEBOAT_CONTROLLER_ACTIVE
          && game_controllers_[i].controller_status_ == GameController::kInactive) {
        game_controllers_[i].controller_status_ = GameController::kActive;
        UpdateControllerInfo(i);
      } else if (status == PADDLEBOAT_CONTROLLER_INACTIVE
          && game_controllers_[i].controller_status_ == GameController::kActive) {
        game_controllers_[i].controller_status_ = GameController::kInactive;
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
  if (controller_index >= 0 && controller_index < GameControllerManager::kMaxControllers) {
    game_controllers_[controller_index].controller_status_ = status;
    if (status == GameController::kActive) {
      UpdateControllerInfo(controller_index);
    }
    if (status_callback_ != nullptr) {
      status_callback_(controller_index, status, callback_user_data_);
    }
  }
}

void GameControllerManager::SetIntegratedMotionDataCallback(
    IntegratedMotionDataCallback callback, void *user_data) {
  if (integrated_motion_data_) {
    integrated_motion_data_callback_ = callback;
    integrated_user_data_ = user_data;
  }
}

void GameControllerManager::SetGameControllerMotionDataCallback(
    GameControllerMotionDataCallback callback, void *user_data) {
  game_controller_motion_data_callback_ = callback;
  motion_user_data_ = user_data;
}

void GameControllerManager::SetGameControllerStatusCallback(GameControllerStatusCallback callback,
                                                            void *user_data) {
  status_callback_ = callback;
  callback_user_data_ = user_data;
}

} // namespace base_game_framework
