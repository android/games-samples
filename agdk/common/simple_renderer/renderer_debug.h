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

#ifndef SIMPLERENDERER_DEBUG_H
#define SIMPLERENDERER_DEBUG_H

#include <cstdlib>
#include "debug_manager.h"

// Logging, assertion and API error check macros for simple_renderer. They only depend on
// base_game_framework (DebugManager), not on the application using the renderer.
// RENDERER_ERROR, RENDERER_LOG and RENDERER_ASSERT are statements and must be followed
// by a ';'. RENDERER_CHECK_GLES and RENDERER_CHECK_VK are bool expressions (true if there
// was no error) in all builds, so they can be used in conditions.

#define RENDERER_LOG_TAG "SimpleRenderer"

#define RENDERER_ERROR(...)                                                         \
  ::base_game_framework::DebugManager::Log(                                         \
      ::base_game_framework::DebugManager::kLog_Channel_Default,                    \
      ::base_game_framework::DebugManager::kLog_Level_Error, RENDERER_LOG_TAG, __VA_ARGS__)

#define RENDERER_LOG(...)                                                           \
  ::base_game_framework::DebugManager::Log(                                         \
      ::base_game_framework::DebugManager::kLog_Channel_Default,                    \
      ::base_game_framework::DebugManager::kLog_Level_Info, RENDERER_LOG_TAG, __VA_ARGS__)

#ifdef NDEBUG
// Release builds: assertions are compiled out (the condition is type checked but not
// evaluated), and GLES errors are not checked, since glGetError can stall the pipeline.
// Vulkan results are still checked, which only costs a comparison, so failures are logged
// in release too.
#define RENDERER_ASSERT(cond) do { (void)sizeof((cond) ? true : false); } while (0)
#define RENDERER_CHECK_GLES(message) ::simple_renderer::RendererCheckGLESDisabled(message)
#else
#define RENDERER_ASSERT(cond)                                                       \
  do {                                                                              \
    if (!(cond)) {                                                                  \
      ::simple_renderer::RendererAssertFailed(#cond, __FILE__, __LINE__);           \
    }                                                                               \
  } while (0)
#define RENDERER_CHECK_GLES(message) ::simple_renderer::RendererCheckGLES(message)
#endif

#define RENDERER_CHECK_VK(result, message) ::simple_renderer::RendererCheckVk(result, message)

namespace simple_renderer {

// Logs the failed assertion and aborts
[[noreturn]] inline void RendererAssertFailed(const char* condition, const char* file,
                                              int line) {
  RENDERER_ERROR("ASSERTION FAILED: %s (%s:%d)", condition, file, line);
  std::abort();
}

// Logs any pending GL errors. Returns true if there were none, asserts in debug builds
// otherwise.
bool RendererCheckGLES(const char* message);
// Release builds don't check GL errors (see RENDERER_CHECK_GLES)
inline bool RendererCheckGLESDisabled(const char* message) {
  (void)message;
  return true;
}
// Logs a failed VkResult. Returns true if result is not an error, asserts in debug builds
// otherwise.
bool RendererCheckVk(int result, const char* message);
}

#endif // SIMPLERENDERER_DEBUG_H
