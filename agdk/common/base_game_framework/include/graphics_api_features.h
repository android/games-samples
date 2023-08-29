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

#ifndef BASEGAMEFRAMEWORK_GRAPHICSAPI_FEATURES_H_
#define BASEGAMEFRAMEWORK_GRAPHICSAPI_FEATURES_H_

#include <cstdint>

namespace base_game_framework {

/**
 * @brief The base class definition for the `GraphicsAPIFeatures` class of BaseGameFramework.
 * This class is used to encapsulate feature information about an active graphics API
 */
class GraphicsAPIFeatures {
 public:

  /** @brief Enum bitmask of possible graphic features supported by a graphics API */
  enum GraphicsFeature : uint64_t {
    /** @brief Bit flag if 16-bit float arithmetic is supported in shader code */
    kGraphicsFeature_F16_Math = 0,
    /** @brief Bit flag if 8-bit integer arithmetic is supported in shader code */
    kGraphicsFeature_I8_Math,
    /** @brief Bit flag if 16-bit integer arithmetic is supported in shader code */
    kGraphicsFeature_I16_Math,
    /** @brief Bit flag if 16-bit integer/float storage is supported in storage buffer objects */
    kGraphicsFeature_F16_I16_SSBO,
    /** @brief Bit flag if 16-bit integer/float storage is supported in uniform buffer objects */
    kGraphicsFeature_F16_I16_UBO,
    /** @brief Bit flag if 16-bit integer/float storage is supported in push constant blocks */
    kGraphicsFeature_F16_I16_Push_Constant,
    /** @brief Bit flag if 16-bit integer/float storage is supported in shader input/output */
    kGraphicsFeature_F16_I16_Input_Output,
    /** @brief Bit flag if wide lines are supported */
    kGraphicsFeature_Wide_Lines
  };

/**
 * @brief Constructor
 */
  GraphicsAPIFeatures();

/**
 * @brief Constructor
 * @param feature_bits Bitmask of `GraphicsFeature` enum values of supported features.
 * @param active_api_version API specific version information for active API
 */
  GraphicsAPIFeatures(uint64_t feature_bits, uint32_t active_api_version);

/**
 * @brief Get the feature bitmask of supported features.
 * @return Bitmask of `GraphicsFeature` enum values of supported features.
 */
  uint64_t GetFeatureBits() const { return feature_bits_; }

/**
 * @brief Check if a graphics feature is supported by the active graphics API
 * @param feature Enum value from `GraphicsFeature` of the feature to check for
 * @return true if the specified feature is supported by the active graphics API
 */
  bool HasGraphicsFeature(const GraphicsFeature feature) const;

/**
 * @brief Set the graphics features of the active graphics API
 * @param feature Enum value from `GraphicsFeature` of the feature to set as supported
 */
  void SetGraphicsFeature(const GraphicsFeature feature) {
    feature_bits_ |= (1ULL << feature);
  }

/**
 * @brief Set the graphics features of the active graphics API
 * @param bits Bitmask of `GraphicsFeature` enum values of supported features
 */
  void SetFeatureBits(const uint64_t bits) { feature_bits_ = bits; }

/**
 * @brief Get the version information of the active graphics API
 * @return API specific version information for active API
 */
  uint32_t GetActiveAPIVersion() const { return active_api_version_; }

/**
 * @brief Set the version information of the active graphics API
 * @param version API specific version information for active API
 */
  void SetActiveAPIVersion(const uint32_t version) { active_api_version_ = version; }

 private:
  uint64_t feature_bits_;
  uint32_t active_api_version_;
};
}

#endif // BASEGAMEFRAMEWORK_GRAPHICSAPI_FEATURES_H_
