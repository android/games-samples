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

#include <random>
#include "sfxman.hpp"

#define MAX_SAMPLES_PER_SEC 48000 // was 8000
#define BUF_SAMPLES_MAX MAX_SAMPLES_PER_SEC*5 // 5 seconds
#define DEFAULT_VOLUME 0.9f

static SfxMan *_instance = new SfxMan();
static volatile int32_t _frameCount = 0;
static volatile int32_t _frameCursor = 0;
static volatile int32_t _sampleRate = 0;
static int16_t _sample_buf[BUF_SAMPLES_MAX];
static volatile bool _bufferActive = false;


SfxMan *SfxMan::GetInstance() {
    return _instance ? _instance : (_instance = new SfxMan());
}

SfxMan::SfxMan() {
    oboe::AudioStreamBuilder audioStreamBuilder;
    audioStreamBuilder.setChannelCount(oboe::ChannelCount::Mono);
    audioStreamBuilder.setDataCallback(this);
    audioStreamBuilder.setDirection(oboe::Direction::Output);
    audioStreamBuilder.setFormat(oboe::AudioFormat::I16);
    audioStreamBuilder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    audioStreamBuilder.setSharingMode(oboe::SharingMode::Exclusive);
    oboe::Result result = audioStreamBuilder.openStream(mAudioStream);
    if (result == oboe::Result::OK) {
        ALOGI("SfxMan: initialization complete.");
        result = mAudioStream->requestStart();
        if (result == oboe::Result::OK) {
            _sampleRate = mAudioStream->getSampleRate();
            if (_sampleRate <= MAX_SAMPLES_PER_SEC && _sampleRate != oboe::kUnspecified) {
                ALOGI("Audio stream sample rate: %d Hz", _sampleRate);
                mInitOk = true;
            } else {
                ALOGE("Audio stream has unspecified or too high sample rate.");
            }
        } else {
            ALOGE("Failed to start audio stream. Error: %s", oboe::convertToText(result));
        }
    } else {
        ALOGE("Failed to create audio stream. Error: %s", oboe::convertToText(result));
    }
}

SfxMan::~SfxMan() {
    if (mInitOk) {
        mAudioStream->stop();
        mAudioStream->close();
        mInitOk = false;
    }
}

bool SfxMan::IsIdle() {
    return !_bufferActive;
}

static const char *_parseInt(const char *s, int *result) {
    *result = 0;
    while (*s >= '0' && *s <= '9') {
        *result = *result * 10 + (*s - '0');
        s++;
    }
    return s;
}

static int _synth(int frequency, int /*duration*/, float amplitude, int16_t *sample_buf, int samples) {
    int i;

    for (i = 0; i < samples; i++) {
        float t = i / (float) _sampleRate;
        float v;
        if (frequency > 0) {
            v = amplitude * sin(frequency * t * 2 * M_PI) +
                (amplitude * 0.1f) * sin(frequency * 2 * t * 2 * M_PI);
        } else {
            int r = rand();
            r = r > 0 ? r : -r;
            v = amplitude * (-0.5f + (r % 1024) / 512.0f);
        }
        int value = (int) (v * 32768.0f);
        sample_buf[i] = value < -32767 ? -32767 : value > 32767 ? 32767 : value;

        if (i > 0 && sample_buf[i - 1] < 0 && sample_buf[i] >= 0) {
            // start of new wave -- check if we have room for a full period of it
            int period_samples = (1.0f / frequency) * _sampleRate;
            if (i + period_samples >= samples) {
                size_t paddingSize = (samples - i) * sizeof(int16_t);
                memset(&sample_buf[i], 0, paddingSize);
                break;
            }
        }
    }

    return i;
}

static void _taper(int16_t *sample_buf, int samples) {
    int i;
    const float TAPER_SAMPLES_FRACTION = 0.1f;
    int taper_samples = (int) (TAPER_SAMPLES_FRACTION * samples);
    for (i = 0; i < taper_samples && i < samples; i++) {
        float factor = i / (float) taper_samples;
        sample_buf[i] = (int16_t) ((float) sample_buf[i] * factor);
    }
    for (i = samples - taper_samples; i < samples; i++) {
        if (i < 0) continue;
        float factor = (samples - i) / (float) taper_samples;
        sample_buf[i] = (int16_t) ((float) sample_buf[i] * factor);
    }
}

void SfxMan::PlayTone(const char *tone) {
    if (!mInitOk) {
        ALOGW("SfxMan: not playing sound because initialization failed.");
        return;
    }
    if (_bufferActive) {
        // can't play -- the buffer is in use
        ALOGW("SfxMan: can't play tone; buffer is active.");
        return;
    }

    // synth the tone
    int total_samples = 0;
    int num_samples;
    int frequency = 100;
    int duration = 50;
    int volume_int;
    float amplitude = DEFAULT_VOLUME;

    while (*tone) {
        switch (*tone) {
            case 'f':
                // set frequency
                tone = _parseInt(tone + 1, &frequency);
                break;
            case 'd':
                // set duration
                tone = _parseInt(tone + 1, &duration);
                break;
            case 'a':
                // set amplitude.
                tone = _parseInt(tone + 1, &volume_int);
                amplitude = volume_int / 100.0f;
                amplitude = amplitude < 0.0f ? 0.0f : amplitude > 1.0f ? 1.0f : amplitude;
                break;
            case '.':
                // synth
                num_samples = duration * _sampleRate / 1000;
                if (num_samples > (BUF_SAMPLES_MAX - total_samples - 1)) {
                    num_samples = BUF_SAMPLES_MAX - total_samples - 1;
                }
                num_samples = _synth(frequency, duration, amplitude, _sample_buf + total_samples,
                                     num_samples);
                total_samples += num_samples;
                tone++;
                break;
            default:
                // ignore and advance to next character
                tone++;
        }
    }

    int total_size = total_samples * sizeof(int16_t);
    if (total_size <= 0) {
        ALOGW("Tone is empty. Not playing.");
        return;
    }

    _taper(_sample_buf, total_samples);

    _bufferActive = true;
    _frameCursor = 0;
    _frameCount = total_samples;
}

oboe::DataCallbackResult
SfxMan::onAudioReady(oboe::AudioStream */*audioStream*/, void *audioData, int32_t numFrames) {
    if (_frameCount > 0) {
        const int32_t remainingFrames = _frameCount - _frameCursor;
        if (numFrames < remainingFrames) {
            memcpy(audioData, &_sample_buf[_frameCursor], numFrames * sizeof(int16_t));
            _frameCursor += numFrames;
        } else {
            const int32_t paddingFrames = numFrames - remainingFrames;
            const size_t copySize = remainingFrames * sizeof(int16_t);
            const size_t paddingSize = paddingFrames * sizeof(int16_t);
            uint8_t *baseBuffer = static_cast<uint8_t *>(audioData);
            memcpy(baseBuffer, &_sample_buf[_frameCursor], copySize);
            memset(&baseBuffer[copySize], 0, paddingSize);
            // We've finished playing the buffer
            _frameCount = 0;
            _frameCursor = 0;
        }
    } else {
        const size_t paddingSize = numFrames * sizeof(int16_t);
        memset(audioData, 0, paddingSize);
        if (_bufferActive) {
            // First callback after finish playing the buffer, mark the
            // buffer as inactive
            _bufferActive = false;
        }
    }

    return oboe::DataCallbackResult::Continue;
}
