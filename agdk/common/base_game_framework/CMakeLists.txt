#
# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# BaseGameFramework static library (target: base_game_framework).
#
# Usage from a game's CMakeLists.txt:
#   add_subdirectory(<agdk>/common/base_game_framework
#                    ${CMAKE_CURRENT_BINARY_DIR}/base_game_framework)
#   target_link_libraries(<game> base_game_framework)
#
# Requirements:
#   - Android only (the only platform implemented so far).
#   - The AGDK prefab packages game-activity, games-controller and games-frame-pacing
#     (Gradle 'prefab true' with the androidx.games dependencies). They are found here if
#     the caller hasn't already.
#   - The volk and VulkanMemoryAllocator headers. They default to <agdk>/third_party; set
#     BGF_VOLK_INCLUDE_DIR / BGF_VMA_INCLUDE_DIR to use other copies. The implementations of
#     both are compiled into this library.
#   - The Java classes in java/ must be part of the app (BaseGameFrameworkUtils has a native
#     method implemented here).
#
# Linking it keeps GameActivity_onCreate (GameActivity's native entry point) and the JNI method
# exported from the game's shared library, even before the game calls into the library.
#
# Compiler warning flags come from the caller (e.g. CMAKE_CXX_FLAGS set before
# add_subdirectory). The volk and VMA headers are system includes, so their warnings don't
# break -Werror builds.

cmake_minimum_required(VERSION 3.18.1)
project(base_game_framework LANGUAGES C CXX)

if(NOT ANDROID)
  message(FATAL_ERROR "base_game_framework: only Android is supported")
endif()

get_filename_component(BGF_THIRD_PARTY_DIR "${CMAKE_CURRENT_LIST_DIR}/../../third_party" ABSOLUTE)
# Unset or empty: the default. A relative override is relative to the top level source
# directory (where a -D option or a game's CMakeLists.txt would set it).
if(NOT BGF_VOLK_INCLUDE_DIR)
  set(BGF_VOLK_INCLUDE_DIR "${BGF_THIRD_PARTY_DIR}/volk")
endif()
if(NOT BGF_VMA_INCLUDE_DIR)
  set(BGF_VMA_INCLUDE_DIR "${BGF_THIRD_PARTY_DIR}/VulkanMemoryAllocator/include")
endif()
get_filename_component(BGF_VOLK_INCLUDE_DIR "${BGF_VOLK_INCLUDE_DIR}" ABSOLUTE
                       BASE_DIR "${CMAKE_SOURCE_DIR}")
get_filename_component(BGF_VMA_INCLUDE_DIR "${BGF_VMA_INCLUDE_DIR}" ABSOLUTE
                       BASE_DIR "${CMAKE_SOURCE_DIR}")
if(NOT EXISTS "${BGF_VOLK_INCLUDE_DIR}/volk.h")
  message(FATAL_ERROR "base_game_framework: volk.h not found in ${BGF_VOLK_INCLUDE_DIR} "
                      "(set BGF_VOLK_INCLUDE_DIR)")
endif()
if(NOT EXISTS "${BGF_VMA_INCLUDE_DIR}/vk_mem_alloc.h")
  message(FATAL_ERROR "base_game_framework: vk_mem_alloc.h not found in ${BGF_VMA_INCLUDE_DIR} "
                      "(set BGF_VMA_INCLUDE_DIR)")
endif()

# AGDK libraries (prefab packages)
if(NOT TARGET game-activity::game-activity_static)
  find_package(game-activity REQUIRED CONFIG)
endif()
if(NOT TARGET games-controller::paddleboat_static)
  find_package(games-controller REQUIRED CONFIG)
endif()
if(NOT TARGET games-frame-pacing::swappy_static)
  find_package(games-frame-pacing REQUIRED CONFIG)
endif()

set(BGF_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src")

add_library(base_game_framework STATIC
    ${BGF_SRC_DIR}/debug_manager.cpp
    ${BGF_SRC_DIR}/display_manager.cpp
    ${BGF_SRC_DIR}/graphics_api_features.cpp
    ${BGF_SRC_DIR}/system_event_manager.cpp
    ${BGF_SRC_DIR}/user_input_manager.cpp
    ${BGF_SRC_DIR}/android/basegameframework_init_android.cpp
    ${BGF_SRC_DIR}/android/display_manager_android.cpp
    ${BGF_SRC_DIR}/android/filesystem_manager_android.cpp
    ${BGF_SRC_DIR}/android/game_controller_android.cpp
    ${BGF_SRC_DIR}/android/game_controller_manager_android.cpp
    ${BGF_SRC_DIR}/android/platform_event_loop_android.cpp
    ${BGF_SRC_DIR}/android/platform_events_android.cpp
    # Also implements the BaseGameFrameworkUtils.registerUtilObject JNI method (kept by the
    # link options below)
    ${BGF_SRC_DIR}/android/platform_util_android.cpp
    ${BGF_SRC_DIR}/android/platform_util_gles_android.cpp
    ${BGF_SRC_DIR}/android/platform_util_log_android.cpp
    ${BGF_SRC_DIR}/android/platform_util_vulkan_android.cpp
    ${BGF_SRC_DIR}/android/system_event_manager_android.cpp
    ${BGF_SRC_DIR}/android/system_info_utils_android.cpp
    ${BGF_SRC_DIR}/gles/graphics_api_gles.cpp
    ${BGF_SRC_DIR}/vulkan/graphics_api_vulkan.cpp
    # volk and VMA implementations
    ${BGF_SRC_DIR}/vulkan/graphics_api_vulkan_loader.cpp
    ${BGF_SRC_DIR}/vulkan/graphics_api_vulkan_utils.cpp)

# C++17, without compiler extensions (-std=c++17). The standard is set on the target too:
# compile features alone add no flag when the compiler's default (gnu++17) satisfies them.
target_compile_features(base_game_framework PUBLIC cxx_std_17)
set_target_properties(base_game_framework PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON)

# src/ is public too: simple_renderer and games use some of its headers (e.g.
# vulkan/graphics_api_vulkan_resources.h, android/platform_util_android.h)
target_include_directories(base_game_framework PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${BGF_SRC_DIR})
# Third party headers: system includes, so their warnings don't break the library's or its
# users' -Werror builds
target_include_directories(base_game_framework SYSTEM PUBLIC
    ${BGF_VOLK_INCLUDE_DIR}
    ${BGF_VMA_INCLUDE_DIR})

target_link_libraries(base_game_framework PUBLIC
    game-activity::game-activity_static
    games-controller::paddleboat_static
    games-frame-pacing::swappy_static
    android
    EGL
    GLESv3
    log)

# Entry points only called from Java. A static library's objects are only linked in when
# referenced, so without these a game that doesn't (yet) reference them from native code
# would lose them: GameActivity_onCreate (from game-activity, see
# https://github.com/android-ndk/ndk/issues/381) and BaseGameFrameworkUtils.registerUtilObject.
target_link_options(base_game_framework INTERFACE
    "-Wl,-u,GameActivity_onCreate"
    "-Wl,-u,Java_com_google_android_games_basegameframework_BaseGameFrameworkUtils_registerUtilObject")
