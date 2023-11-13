/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PERFMONITOR_H_
#define PERFMONITOR_H_

#ifdef __ANDROID__
#include <jni.h>
#else

#include <cstdint>

#endif

#include <errno.h>
#include <time.h>
#include "JNIHelper.h"

namespace ndk_helper {

    const int32_t kNumSamples = 100;

/******************************************************************
 * Helper class for a performance monitoring and get current tick time
 */
    class PerfMonitor {
    private:
        float current_FPS_;
        time_t tv_last_sec_;

        double last_tick_;
        int32_t tickindex_;
        double ticksum_;
        double ticklist_[kNumSamples];

        double UpdateTick(double current_tick);

    public:
        PerfMonitor();

        virtual ~PerfMonitor();

        bool Update(float &fFPS);

#ifndef __ANDROID__
        typedef struct timeval {
            long tv_sec;
            long tv_usec;
        } timeval;

        static int gettimeofday(struct timeval *tp, struct timezone *tzp) {
            (void) tzp;
            // Note: some broken versions only have 8 trailing zero's, the correct epoch has
            // 9 trailing zero's
            // This magic number is the number of 100 nanosecond intervals since
            // January 1, 1601 (UTC)
            // until 00:00:00 January 1, 1970
            static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

            SYSTEMTIME system_time;
            FILETIME file_time;
            uint64_t time;

            GetSystemTime(&system_time);
            SystemTimeToFileTime(&system_time, &file_time);
            time = ((uint64_t) file_time.dwLowDateTime);
            time += ((uint64_t) file_time.dwHighDateTime) << 32;

            tp->tv_sec = (long) ((time - EPOCH) / 10000000L);
            tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
            return 0;
        }

#endif

        static double GetCurrentTime() {
            struct timeval time;
            gettimeofday(&time, NULL);
            double ret = time.tv_sec + time.tv_usec * 1.0 / 1000000.0;
            return ret;
        }
    };

}  // namespace ndkHelper
#endif /* PERFMONITOR_H_ */