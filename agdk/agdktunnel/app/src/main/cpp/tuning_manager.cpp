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

#include <android/choreographer.h>
#include <android/configuration.h>
#include <android/log.h>
#include <dlfcn.h>
#include "pb_common.h"
#include "pb_encode.h"
#include "swappy/swappyGL.h"
#include "swappy/swappyGL_extra.h"
#include "tuningfork/tuningfork.h"
#include "tuningfork/tuningfork_extra.h"

#include "game_consts.hpp"
#include "tuning_manager.hpp"

/** @cond INTERNAL */

/**
 * Internal to this file - do not use.
 */
extern "C" void TuningFork_CProtobufSerialization_Dealloc(
        TuningFork_CProtobufSerialization *c);

/** @endcond */

#if defined(USE_APT)
namespace {
    constexpr TuningFork_InstrumentKey TFTICK_CHOREOGRAPHER = TFTICK_USERDEFINED_BASE;

    TuningFork_LoadingEventHandle startupLoadingHandle;
    TuningFork_LoadingTimeMetadata startupLoadingMetadata;

    typedef void (*func_AChoreographer_postFrameCallback64)(
            AChoreographer *choreographer, AChoreographer_frameCallback64 callback,
            void *data);

    func_AChoreographer_postFrameCallback64 pAChoreographer_postFrameCallback64 = nullptr;

    void choreographer_callback(long /*frameTimeNanos*/, void *data) {
        TuningManager *tuningManager = reinterpret_cast<TuningManager *>(data);
        tuningManager->HandleChoreographerFrame();
    }

    void choreographer_callback64(int64_t /*frameTimeNanos*/, void *data) {
        TuningManager *tuningManager = reinterpret_cast<TuningManager *>(data);
        tuningManager->HandleChoreographerFrame();
    }

    bool serialize_annotation(TuningFork_CProtobufSerialization &cser,
                              const _com_google_tuningfork_Annotation *annotation) {
        bool success = false;
        cser.bytes = NULL;
        cser.size = 0;

        size_t encodedSize = 0;
        if (pb_get_encoded_size(&encodedSize, com_google_tuningfork_Annotation_fields,
                                annotation)) {
            cser.bytes = (uint8_t *) ::malloc(encodedSize);
            cser.size = encodedSize;
            cser.dealloc = TuningFork_CProtobufSerialization_Dealloc;

            pb_ostream_t pbStream = pb_ostream_from_buffer(cser.bytes, encodedSize);
            pb_encode(&pbStream, com_google_tuningfork_Annotation_fields, annotation);
            success = true;
        }
        return success;
    }
}
#endif

TuningManager::TuningManager(JNIEnv *env, jobject activity, AConfiguration *config) {
    mTFInitialized = false;

#if defined(USE_APT)
    TuningFork_Settings settings{};

    // Performance Tuner can work with the Frame Pacing library to automatically
    // record frame time via the tracer function
    if (SwappyGL_isEnabled()) {
        settings.swappy_tracer_fn = &SwappyGL_injectTracer;
        settings.swappy_version = Swappy_version();
    }

    // Setup debug builds to connect to the local Performance Monitor app tool
    // localhost is not permitted on recent versions of Android, you may have
    // to manually set up the IP address of your local device here.
#ifndef NDEBUG
    settings.endpoint_uri_override = "http://localhost:9000";
#endif

    /*
     * AGDKTunnel doesn't have 'dynamic' settings, we are going to fake
     * them by checking against what we consider  'low' and 'high' values
     * for the RENDER_TUNNEL_SECTION_COUNT and TUNNEL_SECTION_LENGTH constants
     * defined in game_consts.hpp and setting the corresponding fidelity level
     * for our reporting
     */
    TuningFork_CProtobufSerialization fps = {};
    bool bHighDensity = (RENDER_TUNNEL_SECTION_COUNT == 8 && TUNNEL_SECTION_LENGTH == 75.0f);
    const char *filename = bHighDensity ? "dev_tuningfork_fidelityparams_2.bin" :
                           "dev_tuningfork_fidelityparams_1.bin";
    if (TuningFork_findFidelityParamsInApk(env, activity, filename, &fps)
        == TUNINGFORK_ERROR_OK) {
        // This overrides the value in default_fidelity_parameters_filename
        //  in tuningfork_settings, if it is there.
        settings.training_fidelity_params = &fps;
    } else {
        ALOGE("Couldn't load fidelity params from %s", filename);
    }

    TuningFork_ErrorCode tfError = TuningFork_init(&settings, env, activity);
    if (tfError == TUNINGFORK_ERROR_OK) {
        mTFInitialized = true;
        StartLoading();
    } else {
        ALOGE("Error initializing TuningFork: %d", tfError);
    }

    // Free any fidelity params we got from the APK
    TuningFork_CProtobufSerialization_free(&fps);

    InitializeChoreographerCallback(config);
#endif
}

TuningManager::~TuningManager() {
#if defined(USE_APT)
    if (mTFInitialized) {
        TuningFork_ErrorCode tfError = TuningFork_destroy();
        if (tfError != TUNINGFORK_ERROR_OK) {
            ALOGE("Error destroying TuningFork: %d", tfError);
        }
    }
#endif
}

void TuningManager::InitializeChoreographerCallback(AConfiguration *config) {
#if defined(USE_APT)
    int32_t sdkVersion = AConfiguration_getSdkVersion(config);
    if (sdkVersion >= 29) {
        // The original postFrameCallback is deprecated in 29 and later, try and
        // get the new postFrameCallback64 call
        void *lib = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
        if (lib != nullptr) {
            // Retrieve function pointer from shared object.
            pAChoreographer_postFrameCallback64 =
                    reinterpret_cast<func_AChoreographer_postFrameCallback64>(
                            dlsym(lib, "AChoreographer_postFrameCallback64"));
            if (pAChoreographer_postFrameCallback64 != nullptr) {
                pAChoreographer_postFrameCallback64(AChoreographer_getInstance(),
                                                    choreographer_callback64, this);
            } else {
                // fallback to old
                AChoreographer_postFrameCallback(AChoreographer_getInstance(),
                                                 choreographer_callback, this);
            }
        }
    } else {
        AChoreographer_postFrameCallback(AChoreographer_getInstance(),
                                         choreographer_callback, this);
    }
#endif
}

void TuningManager::HandleChoreographerFrame() {
#if defined(USE_APT)
    PostFrameTick(TFTICK_CHOREOGRAPHER);

    if (pAChoreographer_postFrameCallback64 != nullptr) {
        pAChoreographer_postFrameCallback64(AChoreographer_getInstance(),
                                            choreographer_callback64, this);
    } else {
        AChoreographer_postFrameCallback(AChoreographer_getInstance(),
                                         choreographer_callback, this);
    }
#endif
}

void TuningManager::PostFrameTick(const TuningFork_InstrumentKey frameKey) {
    if (mTFInitialized) {
        TuningFork_ErrorCode tfError = TuningFork_frameTick(frameKey);
        if (tfError != TUNINGFORK_ERROR_OK) {
            ALOGE("Error calling TuningFork_frameTick: %d", tfError);
        }
    }
}

#if defined(USE_APT)
void TuningManager::SetCurrentAnnotation(const _com_google_tuningfork_Annotation *annotation) {
    TuningFork_CProtobufSerialization cser;
    if (serialize_annotation(cser, annotation)) {
        if (TuningFork_setCurrentAnnotation(&cser) != TUNINGFORK_ERROR_OK) {
            ALOGW("Bad annotation passed to TuningFork_setCurrentAnnotation");
        }
        TuningFork_CProtobufSerialization_free(&cser);
    } else {
        ALOGE("Failed to calculate annotation encode size");
    }
}
#endif

void TuningManager::StartLoading() {
#if defined(USE_APT)
    // Initial annotation of our state
    _com_google_tuningfork_Annotation annotation;
    annotation.loading = com_google_tuningfork_LoadingState_LOADING;
    annotation.level = com_google_tuningfork_Level_STARTUP;
    SetCurrentAnnotation(&annotation);

    // Setup loading start
    TuningFork_CProtobufSerialization cser;
    if (serialize_annotation(cser, &annotation)) {
        startupLoadingMetadata.state =
                TuningFork_LoadingTimeMetadata::LoadingState::COLD_START;
        startupLoadingMetadata.network_latency_ns = 1234567;
        TuningFork_startRecordingLoadingTime(&startupLoadingMetadata,
                                             sizeof(TuningFork_LoadingTimeMetadata),
                                             &cser,
                                             &startupLoadingHandle);
        TuningFork_CProtobufSerialization_free(&cser);
    }
#endif
}

void TuningManager::FinishLoading() {
#if defined(USE_APT)
    TuningFork_stopRecordingLoadingTime(startupLoadingHandle);

    _com_google_tuningfork_Annotation annotation;
    annotation.loading = com_google_tuningfork_LoadingState_NOT_LOADING;
    annotation.level = com_google_tuningfork_Level_LEVEL_1;
    SetCurrentAnnotation(&annotation);
#endif
}
