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

#ifndef agdktunnel_tuning_manager_hpp
#define agdktunnel_tuning_manager_hpp

#include "common.hpp"
#include "nano/dev_tuningfork.pb.h"
#include "nano/tuningfork.pb.h"

struct AConfiguration;

class TuningManager {
private:
    bool mTFInitialized;

    void InitializeChoreographerCallback(AConfiguration *config);

public:
    TuningManager(JNIEnv *env, jobject context, AConfiguration *config);

    ~TuningManager();

    void HandleChoreographerFrame();

    void PostFrameTick(const uint16_t frameKey);

    void SetCurrentAnnotation(const _com_google_tuningfork_Annotation *annotation);

    void StartLoading();

    void FinishLoading();
};

#endif
