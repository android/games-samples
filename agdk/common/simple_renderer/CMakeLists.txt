#
# Copyright 2026 The Android Open Source Project
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

# simple_renderer static library (target: simple_renderer), a GLES/Vulkan renderer on top of
# BaseGameFramework.
#
# Usage from a game's CMakeLists.txt:
#   add_subdirectory(<agdk>/common/simple_renderer ${CMAKE_CURRENT_BINARY_DIR}/simple_renderer)
#   target_link_libraries(<game> simple_renderer)
# Include its headers as "simple_renderer/renderer_interface.h" etc.
#
# It uses the base_game_framework target, and adds it (from ../base_game_framework) if the
# caller hasn't. Compiler warning flags come from the caller.

cmake_minimum_required(VERSION 3.18.1)
project(simple_renderer LANGUAGES CXX)

if(NOT TARGET base_game_framework)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../base_game_framework
                   ${CMAKE_CURRENT_BINARY_DIR}/base_game_framework)
endif()

set(SIMPLE_RENDERER_DIR "${CMAKE_CURRENT_LIST_DIR}")
get_filename_component(SIMPLE_RENDERER_PARENT_DIR "${SIMPLE_RENDERER_DIR}/.." ABSOLUTE)

add_library(simple_renderer STATIC
    ${SIMPLE_RENDERER_DIR}/renderer_debug_gles.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_debug_vk.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_gles.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_index_buffer_gles.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_index_buffer_vk.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_interface.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_render_pass_gles.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_render_pass_vk.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_render_state_gles.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_render_state_vk.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_resources.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_shader_program_gles.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_shader_program_vk.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_texture.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_texture_gles.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_texture_vk.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_uniform_buffer.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_uniform_buffer_gles.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_uniform_buffer_vk.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_vertex_buffer_gles.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_vertex_buffer_vk.cpp
    ${SIMPLE_RENDERER_DIR}/renderer_vk.cpp)

# C++17, without compiler extensions (-std=c++17). The standard is set on the target too:
# compile features alone add no flag when the compiler's default (gnu++17) satisfies them.
target_compile_features(simple_renderer PUBLIC cxx_std_17)
set_target_properties(simple_renderer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON)

# Users include "simple_renderer/<header>.h" (the parent directory); the library's own
# sources include their headers by name
target_include_directories(simple_renderer
    PUBLIC ${SIMPLE_RENDERER_PARENT_DIR}
    PRIVATE ${SIMPLE_RENDERER_DIR})

target_link_libraries(simple_renderer PUBLIC base_game_framework)
