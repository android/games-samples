/*
 * Copyright 2021 The Android Open Source Project
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

#include <android/log.h>
#include <memory>

#include "common.h"
#include "util.h"

extern "C" {
void android_main(struct android_app *app);
void nativeThermalStatusChanged(JNIEnv* env,  jclass cls, int32_t thermalState);
}

#define THERMAL_HEADROOM_UPDATE_THRESHOLD (1.0f)

/*
 * Manages the status and rendering of the ImGui system
 */
class ADPFManager {
public:
    static ADPFManager& getInstance() {
        static ADPFManager    instance;
        return instance;
    }
    ~ADPFManager() {
        // Remove global reference.
        if (obj_powerService_ != NULL && app_!= NULL) {
            app_->activity->env->DeleteGlobalRef(obj_powerService_);
        }
    }
    ADPFManager(ADPFManager const&)  = delete;
    void operator=(ADPFManager const&)  = delete;

    void Monitor();

    void SetApplication(android_app* app);

    void SetThermalStatus(int32_t i) { thermal_status_ = i; }
    int32_t GetThermalStatus() { return thermal_status_; }

private:
    ADPFManager() : thermal_status_(0), obj_powerService_(NULL), get_thermal_headroom_(0){
        last_clock_ = Clock();
    }

    int32_t thermal_status_;
    float last_clock_;
    std::shared_ptr<android_app> app_;
    jobject obj_powerService_;
    jmethodID get_thermal_headroom_;

    bool InitializePowerManager();
    float UpdateThermalStatusHeadRoom();
};

#endif // ADPF_MANAGER_H_
