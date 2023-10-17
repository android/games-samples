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

#ifndef SIMPLERENDERER_VK_INCLUDES_H_
#define SIMPLERENDERER_VK_INCLUDES_H_

// Volk loader and Vulkan Memory allocator must be included first, do not include
// vulkan headers directly, include via volk.h
#include "volk.h"
#include "vk_mem_alloc.h"

#endif // SIMPLERENDERER_VK_INCLUDES_H_
