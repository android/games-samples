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

#ifndef ADPF_MANAGER_H_
#define ADPF_MANAGER_H_

#include <android/api-level.h>
#include <android/log.h>
#include <android/thermal.h>

#include <memory>

#include "common.h"
#include "native_engine.h"
#include "util.h"

// Forward declarations of functions that need to be in C decl.
extern "C" {
void nativeThermalStatusChanged(JNIEnv* env, jclass cls, int32_t thermalState);
void nativeRegisterThermalStatusListener(JNIEnv* env, jclass cls);
void nativeUnregisterThermalStatusListener(JNIEnv* env, jclass cls);
}

typedef void (*thermalStateChangeListener)(int32_t, int32_t);

/*
 * ADPFManager class anages the ADPF APIs.
 */
class ADPFManager {
 public:
  // Singleton function.
  static ADPFManager& getInstance() {
    static ADPFManager instance;
    return instance;
  }
  // Dtor.
  ~ADPFManager() {
    // Remove global reference.
    if (app_ != nullptr) {
      if (obj_power_service_ != nullptr) {
        app_->activity->env->DeleteGlobalRef(obj_power_service_);
      }
      if (obj_battery_service_ != nullptr) {
        app_->activity->env->DeleteGlobalRef(obj_battery_service_);
      }
      if (obj_perfhint_service_ != nullptr) {
        app_->activity->env->DeleteGlobalRef(obj_perfhint_service_);
      }
      if (obj_perfhint_session_ != nullptr) {
        app_->activity->env->DeleteGlobalRef(obj_perfhint_session_);
      }
      if (thermal_manager_ != nullptr) {
        if (__builtin_available(android 31, *)) {
          AThermal_releaseManager(thermal_manager_);
        }
      }
    }
  }
  // Delete copy constructor since the class is used as a singleton.
  ADPFManager(ADPFManager const&) = delete;
  void operator=(ADPFManager const&) = delete;

  // Indicates if ADPF is supported on the device.
  bool IsSupported() { return adpf_supported_; }

  // Invoke the method periodically (once a frame) to monitor
  // the device's thermal throttling status.
  void Monitor();

  // Invoke the API first to set the android_app instance.
  void SetApplication(android_app* app);

  // Set callback for thermal state change listener
  void SetThermalListener(thermalStateChangeListener listener);

  // Method to set thermal status. Need to be public since the method
  // is called from C native listener.
  void SetThermalStatus(int32_t i);

  // Get current thermal status and headroom.
  int32_t GetThermalStatus() { return thermal_status_; }
  float GetThermalHeadroom() { return thermal_headroom_; }

  // Set and get thermal headroom forecast period.
  int32_t GetThermalHeadroomForecast() { return thremal_headroom_forcast_; }
  void SetThermalHeadroomForecast(int32_t forecast) {
    if (forecast < kThermalHeadroomForecastMin ||
        forecast > kThermalHeadroomForecastMax) {
      return;
    }
    thremal_headroom_forcast_ = forecast;
    UpdateThermalStatusHeadRoom();
  }

  // Retrieve current battery usage from BatteryManager.
  long GetBatteryUsage();

  // Indicates the start and end of the performance intensive task.
  // The methods call performance hint API to tell the performance
  // hint to the system.
  void BeginPerfHintSession();
  void EndPerfHintSession(jlong target_duration_ns);

  // Initialize performance hint session.
  bool InitializePerformanceHintManager();

  // Close current perf hint session.
  void ClosePerfHintSession();

  // Method to retrieve thermal manager. The API is used to register/unregister
  // callbacks from C API.
  AThermalManager* GetThermalManager() { return thermal_manager_; }

  static constexpr int32_t kThermalHeadroomForecastMin = 1;
  static constexpr int32_t kThermalHeadroomForecastMax = 100;

 private:
  // Update thermal headroom each sec.
  static constexpr int32_t kThermalHeadroomUpdateThreshold = 1;
  static constexpr int32_t kThermalHeadroomForecastDefault = 1;


  // Function pointer from the game, will be invoked when we receive state changed event from Thermal API
  static thermalStateChangeListener thermalListener;

  // Ctor. It's private since the class is designed as a singleton.
  ADPFManager()
      : adpf_supported_(false),
        thermal_manager_(nullptr),
        thermal_status_(0),
        thermal_headroom_(0.f),
        obj_power_service_(nullptr),
        get_thermal_headroom_(0),
        obj_battery_service_(nullptr),
        battery_propertyid_(0),
        get_long_property_(0),
        obj_perfhint_service_(nullptr),
        obj_perfhint_session_(nullptr),
        report_actual_work_duration_(0),
        update_target_work_duration_(0),
        close_session_(0),
        preferred_update_rate_(0),
        thremal_headroom_forcast_(kThermalHeadroomForecastDefault) {
    last_clock_ = Clock();
    perfhintsession_start_ = 0;
  }

  // Functions to initialize ADPF API's calls.
  bool InitializePowerManager();
  float UpdateThermalStatusHeadRoom();
  bool InitializeBatteryManager();

  // Helper function using JNI calls.
  jobject GetService(JNIEnv* env, const char* service);

  bool adpf_supported_;

  AThermalManager* thermal_manager_;
  int32_t thermal_status_;
  float thermal_headroom_;
  float last_clock_;
  std::shared_ptr<android_app> app_;
  jobject obj_power_service_;
  jmethodID get_thermal_headroom_;
  jobject obj_battery_service_;
  jint battery_propertyid_;
  jmethodID get_long_property_;

  jobject obj_perfhint_service_;
  jobject obj_perfhint_session_;
  jmethodID report_actual_work_duration_;
  jmethodID update_target_work_duration_;
  jmethodID close_session_;
  jlong preferred_update_rate_;

  float perfhintsession_start_;

  // Theamal headroom forecast duration.
  int32_t thremal_headroom_forcast_;
};

#endif  // ADPF_MANAGER_H_
