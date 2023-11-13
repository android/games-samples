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

#ifndef IMGUI_MANAGER_H_
#define IMGUI_MANAGER_H_

#include "util.h"
#include "imgui.h"

/*
 * Manages the status and rendering of the ImGui system
 */
class ImGuiManager {
 public:
  ImGuiManager();

  ~ImGuiManager();

  void SetDisplaySize(const int displayWidth, const int displayHeight,
                      const int displayDpi);

  void BeginImGuiFrame();

  void EndImGuiFrame();

  float GetFontScale();

  void SetFontScale(const float fontScale);

 private:
  DeltaClock delta_clock_;
};

// utility structure for realtime plot
// utility structure for realtime plot
struct ScrollingBuffer {
  int MaxSize;
  int Offset;
  ImVector<ImVec2> Data;
  ScrollingBuffer(int max_size = 2000) {
    MaxSize = max_size;
    Offset = 0;
    Data.reserve(MaxSize);
  }
  void AddPoint(float x, float y) {
    if (Data.size() < MaxSize)
      Data.push_back(ImVec2(x, y));
    else {
      Data[Offset] = ImVec2(x, y);
      Offset = (Offset + 1) % MaxSize;
    }
  }
  void Erase() {
    if (Data.size() > 0) {
      Data.shrink(0);
      Offset = 0;
    }
  }
};

#endif  // IMGUI_MANAGER_H_
