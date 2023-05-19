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

#include "renderer_interface.h"
#include "renderer_gles.h"
// uncomment when including vk CL #include "renderer_vk.h"

namespace simple_renderer {

Renderer::RendererAPI Renderer::renderer_api_ = Renderer::kAPI_GLES;
std::unique_ptr<Renderer> Renderer::instance_ = nullptr;

void Renderer::SetRendererAPI(const RendererAPI api) {
  Renderer::renderer_api_ = api;
}

Renderer &Renderer::GetInstance() {
  if (!instance_) {
    if (renderer_api_ == Renderer::kAPI_GLES) {
      instance_ = std::unique_ptr<Renderer>(new RendererGLES());
    } /* uncomment when inluding vk cl else if (renderer_api_ == Renderer::kAPI_Vulkan) {
      instance_ = std::unique_ptr<Renderer>(new RendererVk());
    } */
  }
  return *instance_;
}

void Renderer::ShutdownInstance() {
  Renderer::instance_.reset();
}

Renderer::Renderer() {

}

Renderer::~Renderer() {

}
}