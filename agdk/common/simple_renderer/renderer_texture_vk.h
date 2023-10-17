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

#ifndef SIMPLERENDERER_TEXTURE_VK_H_
#define SIMPLERENDERER_TEXTURE_VK_H_

#include <cstdint>
#include "renderer_vk_includes.h"
#include "renderer_texture.h"

namespace simple_renderer
{
class TextureVk : public Texture {
 public:
  TextureVk(const Texture::TextureCreationParams& params);
  virtual ~TextureVk();

  VkImageView GetImageView() const { return image_view_; }
  VkSampler GetSampler() const { return sampler_; }

 private:
  VkImage image_;
  VkImageView image_view_;
  VmaAllocation image_alloc_;
  VkSampler sampler_;
};
} // namespace simple_renderer

#endif //SIMPLERENDERER_TEXTURE_VK_H_
