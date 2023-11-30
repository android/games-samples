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

#ifndef BASEGAMEFRAMEWORK_GAME_CONTROLLER_H_
#define BASEGAMEFRAMEWORK_GAME_CONTROLLER_H_

#include <cstdint>
#include "platform_defines.h"

namespace base_game_framework {

/*
 * All enums/flags/values match their equivalents in the Paddleboat library
 */

class GameController {
  friend class GameControllerManager;
 public:

/**
 * @brief Game controller buttons defined as bitmask values.
 * AND against `GameControllerData.buttons_down` to check for button
 * status.
 */
  enum GameControllerButtons : uint32_t {
    /** @brief Bitmask for dpad-up button */
    kButton_Dpad_Up = (1U << 0),
    /** @brief Bitmask for dpad-left button */
    kButton_Dpad_Left = (1U << 1),
    /** @brief Bitmask for dpad-down button */
    kButton_Dpad_Down = (1U << 2),
    /** @brief Bitmask for dpad-right button */
    kButton_Dpad_Right = (1U << 3),
    /** @brief Bitmask for A button */
    kButton_A = (1U << 4),
    /** @brief Bitmask for B button */
    kButton_B = (1U << 5),
    /** @brief Bitmask for X button */
    kButton_X = (1U << 6),
    /** @brief Bitmask for Y button */
    kButton_Y = (1U << 7),
    /** @brief Bitmask for L1 button */
    kButton_L1 = (1U << 8),
    /** @brief Bitmask for L2 button */
    kButton_L2 = (1U << 9),
    /** @brief Bitmask for L3 button */
    kButton_L3 = (1U << 10),
    /** @brief Bitmask for R1 button */
    kButton_R1 = (1U << 11),
    /** @brief Bitmask for R2 button */
    kButton_R2 = (1U << 12),
    /** @brief Bitmask for R3 button */
    kButton_R3 = (1U << 13),
    /** @brief Bitmask for Select button */
    kButton_Select = (1U << 14),
    /** @brief Bitmask for Start button */
    kButton_Start = (1U << 15),
    /** @brief Bitmask for System/Menu button */
    kButton_System = (1U << 16),
    /** @brief Bitmask for touchpad press */
    kButton_Touchpad = (1U << 17),
    /** @brief Bitmask for Aux1 button */
    kButton_Aux1 = (1U << 18),
    /** @brief Bitmask for Aux1 button */
    kButton_Aux2 = (1U << 19),
    /** @brief Bitmask for Aux1 button */
    kButton_Aux3 = (1U << 20),
    /** @brief Bitmask for Aux1 button */
    kButton_Aux4 = (1U << 21),
    /** @brief Count of defined buttons */
    kButton_Count = 22
  };

/**
 * @brief Game controller device feature flags as bitmask values
 * AND against `GameControllerInfo.controller_flags` to determine feature
 * availability.
 */
  enum GameControllerFlags : uint32_t {
    /** @brief Bitmask for generic controller profile in use */
    kFlag_Generic_Profile = (0x0000000010),
    /** @brief Bitmask for accelerometer feature */
    kFlag_Accelerometer = (0x00400000),
    /** @brief Bitmask for gyroscope feature */
    kFlag_Gyroscope = (0x00800000),
    /** @brief Bitmask for player number light feature */
    kFlag_Light_Player = (0x01000000),
    /** @brief Bitmask for RGB light feature */
    kFlag_Light_Rgb = (0x02000000),
    /** @brief Bitmask for battery level feature */
    kFlag_Battery = (0x04000000),
    /** @brief Bitmask for single vibration feature */
    kFlag_Vibration = (0x08000000),
    /** @brief Bitmask for dual-motor vibration feature */
    kFlag_Vibration_Dual_Motor = (0x10000000),
    /** @brief Bitmask for touchpad feature */
    kFlag_Touchpad = (0x20000000),
    /** @brief Bitmask for virtual mouse feature */
    kFlag_Virtual_Mouse = (0x40000000)
  };

/**
 * @brief Battery status of a controller
 */
  enum GameControllerBatteryStatus : uint32_t {
    kBattery_Unknown = 0, ///< Battery status is unknown
    kBattery_Charging = 1, ///< Controller battery is charging
    kBattery_Discharging = 2, ///< Controller battery is discharging
    kBattery_Not_Charging = 3, ///< Controller battery is not charging
    kBattery_Full = 4 ///< Controller battery is completely charged
  };

/**
 * @brief Status of a controller at an index
 */
  enum GameControllerStatus : uint32_t {
    kInactive = 0, ///< Controller at index is disconnected and inactive
    kActive = 1 ///< Controller at index is connected and inactive
  };

/**
 * @brief The button layout and iconography of the controller buttons
 */
  enum GameControllerButtonLayout : uint32_t {
    //!  Y \n
    //! X B\n
    //!  A 
    kLayout_Standard = 0,
    //!  △ \n
    //! □ ○\n
    //!  x \n
    //! x = A, ○ = B, □ = X, △ = Y
    kLayout_Shapes = 1,
    //!  X \n
    //! Y A\n
    //!  B 
    kLayout_Reverse = 2,
    //! X Y R1 L1\n
    //! A B R2 L2
    kLayout_Arcade_Stick = 3,
    //! Mask value, AND with
    //! `GameControllerInfo.controller_flags`
    //! to get the `GameControllerButtonLayout` value
    kLayout_Mask = 3
  };

/**
 * @brief The type of light being specified by a call to
 * ::DetControllerLight
 */
  enum GameControllerLightType : uint32_t {
    /** @brief Light is a player indicator and `light_data` the player number */
    kLight_Player_Number = 0,
    /** @brief Light is a color and `light_data` the ARGB (8888) value */
    kLight_Rgb = 1
  };

/**
 * @brief The type of motion data being reported in a GameControllerMotionData
 * structure
 */
  enum GameControllerMotionType {
    kMotion_Accelerometer = 0, ///< Accelerometer motion data
    kMotion_Gyroscope = 1 ///< Gyroscope motion data
  };

/**
 * @brief A structure that describes the current battery state of a controller.
 * This structure will only be populated if a controller has
 * `kFlag_Battery` set in
 * `GameControllerInfo.controller_flags`
 */
  typedef struct GameControllerBattery {
    GameControllerBatteryStatus
        battery_status = kBattery_Unknown;  /** @brief The current status of the battery */
    float battery_level = 0.0f; /** @brief The current charge level of the battery, from
                                    0.0 to 1.0 */
  } GameControllerBattery;

/**
 * @brief A structure that contains virtual pointer position data.
 * X and Y coordinates are pixel based and range from 0,0 to window
 * width,height.
 */
  typedef struct GameControllerPointer {
    /** @brief X pointer position in window space pixel coordinates */
    float pointer_x = 0.0f;
    /** @brief Y pointer position in window space pixel coordinates */
    float pointer_y = 0.0f;
  } GameControllerPointer;

/**
 * @brief A structure that contains X and Y axis data for an analog thumbstick.
 * Axis ranges from -1.0 to 1.0.
 */
  typedef struct GameControllerThumbstick {
    /** @brief X axis data for the thumbstick */
    float stick_x = 0.0f;
    /** @brief X axis data for the thumbstick */
    float stick_y = 0.0f;
  } GameControllerThumbstick;

/**
 * @brief A structure that contains axis precision data for a thumbstick in the
 * X and Y axis. Value ranges from 0.0 to 1.0. Flat is the extent of a center
 * flat (deadzone) area of a thumbstick axis Fuzz is the error tolerance
 * (deviation) of a thumbstick axis
 */
  typedef struct GameControllerThumbstickPrecision {
    /** @brief X axis flat value for the thumbstick */
    float stick_flat_x = 0.0f;
    /** @brief Y axis flat value for the thumbstick */
    float stick_flat_y = 0.0f;
    /** @brief X axis fuzz value for the thumbstick */
    float stick_fuzz_x = 0.0f;
    /** @brief Y axis fuzz value for the thumbstick */
    float stick_fuzz_y = 0.0f;
  } GameControllerThumbstickPrecision;

/**
 * @brief A structure that contains the current data
 * for a controller's inputs and sensors.
 */
  typedef struct GameControllerData {
    /** @brief Timestamp of most recent controller data update, timestamp is
     * microseconds elapsed since clock epoch. */
    uint64_t timestamp = 0;
    /** @brief Bit-per-button bitfield array */
    uint32_t buttons_down = 0;
    /** @brief Left analog thumbstick axis data */
    GameControllerThumbstick left_stick;
    /** @brief Right analog thumbstick axis data */
    GameControllerThumbstick right_stick;
    /** @brief L1 trigger axis data. Axis range is 0.0 to 1.0. */
    float trigger_l1 = 0.0f;
    /** @brief L2 trigger axis data. Axis range is 0.0 to 1.0. */
    float trigger_l2 = 0.0f;
    /** @brief R1 trigger axis data. Axis range is 0.0 to 1.0. */
    float trigger_r1 = 0.0f;
    /** @brief R2 trigger axis data. Axis range is 0.0 to 1.0. */
    float trigger_r2 = 0.0f;
    /**
     * @brief Virtual pointer pixel coordinates in window space.
     * If `GameControlleInfo.controller_flags` has the
     * `kFlag_Virtual_Mouse` bit set, pointer coordinates
     * are valid. If this bit is not set, pointer coordinates will always be
     * 0,0.
     */
    GameControllerPointer virtual_pointer;
    /**
     * @brief Battery status. This structure will only be populated if the
     * controller has `kFlag_Battery` set in
     * `GameControllerInfo.controller_flags`
     */
    GameControllerBattery battery;
  } GameControllerData;

/**
 * @brief A structure that contains information
 * about a particular controller device. Several fields
 * are populated by the value of the corresponding fields from InputDevice.
 */
  typedef struct GameControllerInfo {
    /** @brief Controller feature flag bits */
    uint32_t controller_flags = 0;
    /** @brief Controller number, maps to InputDevice.getControllerNumber() */
    int32_t controller_number = 0;
    /** @brief Vendor ID, maps to InputDevice.getVendorId() */
    int32_t vendor_id = 0;
    /** @brief Product ID, maps to InputDevice.getProductId() */
    int32_t product_id = 0;
    /** @brief Device ID, maps to InputDevice.getId() */
    int32_t device_id = 0;
    /** @brief the flat and fuzz precision values of the left thumbstick */
    GameControllerThumbstickPrecision left_stick_precision;
    /** @brief the flat and fuzz precision values of the right thumbstick */
    GameControllerThumbstickPrecision right_stick_precision;
  } GameControllerInfo;

/**
 * @brief A structure that contains motion data reported by a controller.
 */
  typedef struct GameControllerMotionData {
    /** @brief Timestamp of when the motion data event occurred, timestamp is
     * nanoseconds elapsed since clock epoch. */
    uint64_t timestamp = 0;
    /** @brief The type of motion event data */
    GameControllerMotionType motion_type = kMotion_Accelerometer;
    /** @brief Motion X axis data. */
    float motion_x = 0.0f;
    /** @brief Motion Y axis data. */
    float motion_y = 0.0f;
    /** @brief Motion Z axis data. */
    float motion_z = 0.0f;
  } GameControllerMotionData;

/**
 * @brief A structure that describes the parameters of a vibration effect.
 */
  typedef struct GameControllerVibrationData {
    /** @brief Duration to vibrate the left motor in milliseconds. */
    int32_t duration_left = 0;
    /** @brief Duration to vibrate the right motor in milliseconds. */
    int32_t duration_right = 0;
    /** @brief Intensity of vibration of left motor, valid range is 0.0 to 1.0.
     */
    float intensity_left = 0.0f;
    /** @brief Intensity of vibration of right motor, valid range is 0.0 to 1.0.
     */
    float intensity_right = 0.0f;
  } GameControllerVibrationData;

/**
 * @brief Retrieves the active status of this controller object
 * @return true if the controller object has an active controller, false if inactive
 */
  bool GetControllerActive() const { return controller_status_ == kActive; }

/**
 * @brief Retrieves the index of this controller object
 * @return A controller index in the range of 0 to `GameControllerManager::kMaxControllers - 1`
 */
  uint32_t GetControllerIndex() const { return controller_index_; }

/**
 * @brief Retrieves the controller data of this controller object
 * @return A `GameControllerData` reference. Only valid if the controller is active.
 */
  const GameControllerData &GetControllerData() const { return controller_data_; }

/**
 * @brief Retrieves the controller information of this controller object
 * @return A `GameControllerInfo` reference. Only valid if the controller is active.
 */
  const GameControllerInfo &GetControllerInfo() const { return controller_info_; }

/**
 * @brief Set light parameters of the controller
 * @param light_type Enum value identifying the type of light being controlled
 * @param light_data Data for the light, the format is dependent on the `light_type`
 */
  void SetControllerLight(const GameControllerLightType light_type, uint32_t light_data);

/**
 * @brief Set vibration parameters of the controller
 * @param vibration_data Reference to a `GameControllerVibrationData` structure specifying
 * the vibration parameters.
 */
  void SetControllerVibrationData(const GameControllerVibrationData &vibration_data);

 private:
  uint32_t controller_index_ = 0;
  GameControllerStatus controller_status_ = kInactive;
  GameControllerData controller_data_;
  GameControllerInfo controller_info_;
};

}

#endif // BASEGAMEFRAMEWORK_GAME_CONTROLLER_MANAGER_H_
