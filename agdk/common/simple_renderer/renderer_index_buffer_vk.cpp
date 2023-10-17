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

#include "renderer_index_buffer_vk.h"
#include "renderer_debug.h"
#include "renderer_vk.h"

namespace simple_renderer {

IndexBufferVk::IndexBufferVk(const IndexBuffer::IndexBufferCreationParams& params)
    : IndexBuffer(params) {
  RendererVk& renderer = RendererVk::GetInstanceVk();
  VmaAllocator allocator = renderer.GetAllocator();

  VkBufferCreateInfo create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
  create_info.size = params.data_byte_size;
  create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo alloc_info = {};
  alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
  alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
      VMA_ALLOCATION_CREATE_MAPPED_BIT;

  VkBuffer staging_buffer = VK_NULL_HANDLE;
  VmaAllocation staging_alloc = VK_NULL_HANDLE;
  VmaAllocationInfo staging_info = {};
  const VkResult staging_alloc_result = vmaCreateBuffer(allocator, &create_info, &alloc_info,
      &staging_buffer, &staging_alloc, &staging_info);
  RENDERER_CHECK_VK(staging_alloc_result, "vmaCreateBuffer (staging)");
  RENDERER_ASSERT(staging_info.pMappedData != nullptr)
  memcpy(staging_info.pMappedData, params.index_data, params.data_byte_size);

  create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  alloc_info.flags = 0;
  const VkResult buffer_alloc_result = vmaCreateBuffer(allocator, &create_info, &alloc_info,
                                                       &index_buffer_, &index_buffer_alloc_,
                                                       nullptr);
  RENDERER_CHECK_VK(staging_alloc_result, "vmaCreateBuffer (index buffer)");

  VkCommandBuffer command_buffer = renderer.BeginStagingCommandBuffer();
  VkBufferCopy copy_region = {};
  copy_region.srcOffset = 0;
  copy_region.dstOffset = 0;
  copy_region.size = params.data_byte_size;
  vkCmdCopyBuffer(command_buffer, staging_buffer, index_buffer_, 1, &copy_region);
  renderer.EndStagingCommandBuffer();
  vmaDestroyBuffer(allocator, staging_buffer, staging_alloc);
}

IndexBufferVk::~IndexBufferVk() {
  RENDERER_ASSERT(index_buffer_ != VK_NULL_HANDLE)
  RendererVk& renderer = RendererVk::GetInstanceVk();
  vmaDestroyBuffer(renderer.GetAllocator(), index_buffer_, index_buffer_alloc_);
}
}