/*
 * Copyright 2023 The Android Open Source Project
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

#ifndef SIMPLERENDERER_UNIFORM_BUFFER_H_
#define SIMPLERENDERER_UNIFORM_BUFFER_H_

#include <string>
#include "renderer_buffer.h"

namespace simple_renderer
{
/**
 * @brief The base class definition for the `UniformBuffer` class of SimpleRenderer.
 * Use the `UniformBuffer` class interface to create and destroy `VertexBuffer` objects.
 * Currently the UniformBuffer only supports a small amount of data types, number of elements
 * total size of the uniform buffer.
 */
class UniformBuffer : public RendererBuffer {
 public:
  /** @brief Max number of uniform elements in a uniform buffer */
  static constexpr uint32_t kMaxUniforms = 4;
  /** @brief Max combined byte size of all elements in a uniform buffer
    * (128 bytes since the Vulkan renderer is currently using push constants
    * for uniform data and that is the guaranteed max size in Android Baseline Profile 2021)
    */
  static constexpr size_t kMaxUniformBufferByteSize = 128;
  /** @brief Max number of float values in a uniform buffer */
  static constexpr size_t kMaxUniformBufferFloatSize = kMaxUniformBufferByteSize / sizeof(float);

  /** @brief Flag bit for `UniformBufferElement::element_stage_flags`
   * specifying uniform buffer element is used by the vertex shader
   */
  static constexpr uint32_t kElementStageVertexFlag = (1U << 0);
  /** @brief Flag bit for `UniformBufferElement::element_stage_flags`
   * specifying uniform buffer element is used by the fragment shader
   */
  static constexpr uint32_t kElementStageFragmentFlag = (1U << 1);

  /** @brief Size of a 4 float vector buffer element, in bytes */
  static constexpr size_t kElementSize_Float4 = (sizeof(float) * 4);

  /** @brief Size of a 16 float 4x4 matrix buffer element, in bytes */
  static constexpr size_t kElementSize_Matrix44 = (sizeof(float) * 16);

  /** @brief Constant for unused stage fields in UniformBufferStageRanges */
  static constexpr uint32_t kUnusedStageRange = 0xFFFFFFFF;

  /**
   * @brief The uniform buffer element types supported by `UniformBuffer`
   */
  enum UniformBufferElementType : uint32_t {
    /** @brief A 16-byte vector of 4 32-bit floats */
    kBufferElement_Float4 = 0,
    /** @brief A 64-byte 4x4 matrix of 32-bit floats */
    kBufferElement_Matrix44
  };

  enum UniformBufferFlags : uint32_t {
    /**
     * @brief If set, Uniform buffer data is modified per draw call,
     * otherwise is assumed static after creation
     */
    kBufferFlag_UpdateDynamicPerDraw = (1U << 0),
    /** @brief Deliver via push constant on Vulkan (only method supported currently */
    kBufferFlag_UsePushConstants = (1U << 1)
  };

  /**
   * @brief A structure that defines a data element stored in a `UniformBuffer`
   */
  struct UniformBufferElement {
    /** @brief The data type of the uniform buffer element */
    UniformBufferElementType element_type;
    /** @brief Flag bits that specify which (vertex/fragment) shader stages use the element */
    uint32_t element_stage_flags;
    /** @brief The bind set of the element, used for Vulkan descriptor sets */
    uint32_t element_bind_set;
    /** @brief The bind index of the element, used for Vulkan descriptor sets */
    uint32_t element_bind_index;
    /** @brief The name of the element, used for OpenGL ES element binding (required there).
     * May be null when unused (treated as an empty name).
     */
    const char* element_name;
  };

  /**
   * @brief A structure that defines stage specific usage ranges of the
   * `UniformBuffer`. This is used when a buffer is fed to a shader
   * via Vulkan push constants, and each stage uses part of the buffer.
   * Overlap (buffer fields live in both stages) is supported.
   * Set the kUnusedStageRange constant in offset/size fields for
   * stages that don't use the buffer data
   */
  struct UniformBufferStageRanges {
    /** @brief Byte offset in the buffer to the start of vertex shader data */
    uint32_t vertex_stage_offset;
    /** @brief Byte size of vertex shader data in the buffer */
    uint32_t vertex_stage_size;
    /** @brief Byte offset in the buffer to the start of fragment shader data */
    uint32_t fragment_stage_offset;
    /** @brief Byte size of fragment shader data in the buffer */
    uint32_t fragment_stage_size;
  };

  /**
   * @brief A structure holding required parameters to create a new `UniformBuffer`.
   * Passed to the Renderer::CreateUniformBuffer function.
   */
  struct UniformBufferCreationParams {
    /**
     * @brief An array of `UniformBufferElement` that describe the data
     * elements stored in the `UniformBuffer`. The elements (and their names) are copied
     * by Renderer::CreateUniformBuffer, so the array doesn't need to persist after the call.
     */
    const UniformBufferElement* element_array;
    /** @brief The number of elements in the `element_array` */
    uint32_t element_count;
    /** @brief Uniform buffer bitflags defined by the UniformBufferFlags enum */
    uint32_t buffer_flags;
    /** @brief The stage usage ranges of the buffer data */
    UniformBufferStageRanges stage_ranges;
    /** @brief The size of the uniform buffer data in bytes (the total size of the elements) */
    size_t data_byte_count;
  };

  /**
   * @brief Virtual destructor, do not call directly.
   */
  virtual ~UniformBuffer() {}

  /**
   * @brief Get the element description of a `UniformBuffer` element at the specified index.
   * @param index The index of the element, must be less than ::GetBufferElementCount
   * @return The specified `UniformBufferElement` of the `UniformBuffer`
   */
  const UniformBufferElement& GetElement(uint32_t index) const;

  /**
   * @brief Get the offset, in floats (not bytes), to the data for the specified element in the
   * `UniformBuffer`. Elements are tightly packed in `element_array` order.
   * @param index The index of the element, must be less than ::GetBufferElementCount
   * @return The offset, in floats, to the data for the specified element in the `UniformBuffer`
   */
  uint32_t GetElementOffset(uint32_t index) const;

  /**
   * @brief Get the number of floats of data used by an element type.
   * @return The float count, or 0 for an unknown type
   */
  static uint32_t GetElementFloatCount(UniformBufferElementType element_type);

  /**
   * @brief Get the total size, in bytes, of the data of all elements in the `UniformBuffer`.
   */
  uint32_t GetElementDataByteSize() const { return element_data_float_count_ * sizeof(float); }

  /**
   * @brief Update the data of a `UniformBuffer` element at the specified index.
   * @param index The index of the element, must be less than ::GetBufferElementCount
   * @param data A pointer to the floating point data to use to update the element
   * @param size The number of bytes from `data` to copy to the start of the element: a nonzero
   * multiple of sizeof(float), no larger than the element
   */
  virtual void SetBufferElementData(const uint32_t index, const float* data, const size_t size) = 0;

 protected:
  // The element count and size are limited to the elements that fit
  // (see GetUsableElementLayout)
  UniformBuffer(const UniformBufferCreationParams& params);

  /**
   * @brief Copy element data into a buffer of `kMaxUniformBufferFloatSize` floats laid out
   * per ::GetElementOffset. Checks the index, and that the size is a nonzero multiple of
   * sizeof(float) that fits in the element.
   * @return true if the data was copied
   */
  bool CopyElementData(uint32_t index, const float* data, size_t size, float* buffer) const;

 private:
  struct ElementLayout {
    uint32_t element_count;
    uint32_t float_count;
  };

  UniformBuffer(const UniformBufferCreationParams& params, const ElementLayout& layout);

  // Element names point into element_names_
  UniformBuffer(const UniformBuffer&) = delete;
  UniformBuffer& operator=(const UniformBuffer&) = delete;

  // The leading elements of the creation parameters that can be used: at most kMaxUniforms,
  // with a known type, and together within kMaxUniformBufferFloatSize floats
  static ElementLayout GetUsableElementLayout(const UniformBufferCreationParams& params);

  // Copies of the usable elements, with names pointing to the copied names
  UniformBufferElement elements_[kMaxUniforms];
  std::string element_names_[kMaxUniforms];
  uint32_t element_offsets_[kMaxUniforms];
  uint32_t element_data_float_count_;
};
}

#endif // SIMPLERENDERER_UNIFORM_BUFFER_H_
