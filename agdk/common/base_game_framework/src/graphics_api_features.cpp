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

#include "graphics_api_features.h"

namespace base_game_framework {

GraphicsAPIFeatures::GraphicsAPIFeatures()
  : feature_bits_(0)
  , active_api_version_(0) {

}
GraphicsAPIFeatures::GraphicsAPIFeatures(uint64_t feature_bits, uint32_t active_api_version)
 : feature_bits_(feature_bits)
 , active_api_version_(active_api_version) {
}

bool GraphicsAPIFeatures::HasGraphicsFeature(const GraphicsFeature feature) const {
  const uint64_t mask = (1ULL << feature);
  return ((feature_bits_ & mask) != 0);
}

}