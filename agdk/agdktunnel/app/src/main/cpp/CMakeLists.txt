#
# Copyright 2021 The Android Open Source Project
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
cmake_minimum_required(VERSION 3.4.1)
project(agdktunnel)

file(GLOB_RECURSE native_wrappers CONFIGURE_DEPENDS "native_wrappers/*.cpp" "native_wrappers/*.cc")

find_package(oboe REQUIRED CONFIG)
find_package(game-activity REQUIRED CONFIG)
find_package(games-controller REQUIRED CONFIG)
find_package(games-frame-pacing REQUIRED CONFIG)
find_package(games-performance-tuner REQUIRED CONFIG)
find_package(com.google.android.gms.gni.c REQUIRED CONFIG)

# Set the base dir
set(GAMESDK_BASE_DIR "../../../../..")

set(PROTOBUF_NANO_SRC_DIR "${GAMESDK_BASE_DIR}/third_party/nanopb-c")
# Include the protobuf utility file from the Android Game SDK
include("${GAMESDK_BASE_DIR}/util/protobuf/protobuf.cmake")
# Directory of nano protobuf library source files
include_directories(${PROTOBUF_NANO_SRC_DIR})
if("${USE_APT}" STREQUAL "true")
    # generate runtime files using protoc
    protobuf_generate_nano_c( ${CMAKE_CURRENT_SOURCE_DIR}/../proto ../proto/dev_tuningfork.proto)
    protobuf_generate_nano_c( ${CMAKE_CURRENT_SOURCE_DIR}/../proto ../proto/tuningfork.proto)
    include_directories(${PROTO_GENS_DIR})
endif()

# common include directory for all samples
set(COMMON_BASE_DIR "../../../../../common")
set(COMMON_INCLUDE_DIR "../../../../../common/include")
set(COMMON_SRC_DIR "../../../../../common/src")
set(SIMPLE_RENDERER_DIR "../../../../../common/simple_renderer")
set(BGF_ROOT_DIR "../../../../../common/base_game_framework")
set(BGF_SRC_DIR ${BGF_ROOT_DIR}/src)
set(BGF_INCLUDE_DIR ${BGF_ROOT_DIR}/include)

# Export GameActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u GameActivity_onCreate")

# Set common compiler options
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wno-nullability-completeness -Wno-thread-safety-analysis -Wno-unused-function -Wno-unused-variable" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Werror -Wno-nullability-completeness -Wno-thread-safety-analysis -Wno-unused-function -Wno-unused-variable -Werror=unguarded-availability" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D _LIBCPP_ENABLE_THREAD_SAFETY_ANNOTATIONS -O0 -fPIC" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGOOGLE_PROTOBUF_NO_RTTI -DHAVE_PTHREAD")
add_definitions("-DGLM_FORCE_SIZE_T_LENGTH -DGLM_FORCE_RADIANS")

if("${USE_APT}" STREQUAL "true")
    add_definitions("-DUSE_APT")
endif()

set(THIRD_PARTY_DIR ../../../../../third_party)
# Import the CMakeLists.txt for the glm library
add_subdirectory(${THIRD_PARTY_DIR}/glm ${CMAKE_CURRENT_BINARY_DIR}/glm)

if("${USE_APT}" STREQUAL "true")
    set(PROTOGEN_SRCS
        ${PROTO_GENS_DIR}/nano/dev_tuningfork.pb.c
        ${PROTO_GENS_DIR}/nano/tuningfork.pb.c)
else()
    set(PROTOGEN_SRCS "")
endif()

# TODO: migrate to imported cmake file for commonality with other samples
set(BGF_SRCS
     ${BGF_SRC_DIR}/debug_manager.cpp
     ${BGF_SRC_DIR}/display_manager.cpp
     ${BGF_SRC_DIR}/system_event_manager.cpp
     ${BGF_SRC_DIR}/user_input_manager.cpp
     ${BGF_SRC_DIR}/graphics_api_features.cpp
     ${BGF_SRC_DIR}/android/basegameframework_init_android.cpp
     ${BGF_SRC_DIR}/android/display_manager_android.cpp
     ${BGF_SRC_DIR}/android/filesystem_manager_android.cpp
     ${BGF_SRC_DIR}/android/game_controller_android.cpp
     ${BGF_SRC_DIR}/android/game_controller_manager_android.cpp
     ${BGF_SRC_DIR}/android/platform_event_loop_android.cpp
     ${BGF_SRC_DIR}/android/platform_events_android.cpp
     ${BGF_SRC_DIR}/android/platform_util_gles_android.cpp
     ${BGF_SRC_DIR}/android/platform_util_log_android.cpp
     ${BGF_SRC_DIR}/android/platform_util_vulkan_android.cpp
     ${BGF_SRC_DIR}/android/platform_util_android.cpp
     ${BGF_SRC_DIR}/android/system_event_manager_android.cpp
     ${BGF_SRC_DIR}/android/system_info_utils_android.cpp
     ${BGF_SRC_DIR}/gles/graphics_api_gles.cpp
     ${BGF_SRC_DIR}/vulkan/graphics_api_vulkan.cpp
     ${BGF_SRC_DIR}/vulkan/graphics_api_vulkan_loader.cpp
     ${BGF_SRC_DIR}/vulkan/graphics_api_vulkan_utils.cpp)


# TODO: migrate to imported cmake file for commonality with other samples
set(SIMPLE_RENDERER_SRCS
     ${SIMPLE_RENDERER_DIR}/renderer_debug_gles.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_debug_vk.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_index_buffer_gles.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_index_buffer_vk.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_interface.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_gles.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_render_pass_gles.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_render_pass_vk.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_render_state_gles.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_render_state_vk.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_resources.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_shader_program_gles.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_shader_program_vk.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_texture_gles.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_texture_vk.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_uniform_buffer.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_uniform_buffer_gles.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_uniform_buffer_vk.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_vertex_buffer_gles.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_vertex_buffer_vk.cpp
     ${SIMPLE_RENDERER_DIR}/renderer_vk.cpp)

# now build app's shared lib
add_library(${CMAKE_PROJECT_NAME} SHARED
     ${PROTOBUF_NANO_SRCS}
     ${PROTOGEN_SRCS}
     ${BGF_SRCS}
     ${SIMPLE_RENDERER_SRCS}
     ${native_wrappers}
     android_main.cpp
     anim.cpp
     ascii_to_geom.cpp
     dialog_scene.cpp
     game_asset_manager.cpp
     game_asset_manifest.cpp
     gfx_manager.cpp
     input_util.cpp
     jni_util.cpp
     loader_scene.cpp
     loading_thread.cpp
     data_loader_machine.cpp
     native_engine.cpp
     obstacle.cpp
     obstacle_generator.cpp
     play_scene.cpp
     scene.cpp
     scene_manager.cpp
     sfxman.cpp
     shape_renderer.cpp
     tex_quad.cpp
     text_renderer.cpp
     texture_manager.cpp
     tuning_manager.cpp
     tunnel_engine.cpp
     ui_scene.cpp
     util.cpp
     vibration_helper.cpp
     welcome_scene.cpp
     ${COMMON_SRC_DIR}/Versions.cpp
     )

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
     ${THIRD_PARTY_DIR}/glm/glm
     ${THIRD_PARTY_DIR}/volk
     ${THIRD_PARTY_DIR}/VulkanMemoryAllocator/include
     ${COMMON_BASE_DIR}
     ${COMMON_INCLUDE_DIR}
     ${BGF_INCLUDE_DIR}
     ${BGF_SRC_DIR}
     ${CMAKE_CURRENT_SOURCE_DIR}
     ${CMAKE_CURRENT_SOURCE_DIR}/data)

# add lib dependencies
target_link_libraries(${CMAKE_PROJECT_NAME}
     PUBLIC com.google.android.gms.gni.c::gni_shared
     android
     atomic
     EGL
     game-activity::game-activity_static
     oboe::oboe
     games-controller::paddleboat_static
     games-frame-pacing::swappy_static
     games-performance-tuner::tuningfork_static
     GLESv3
     glm
     log)

include_directories(./native_wrappers/c)
include_directories(./native_wrappers/cpp)

if("${USE_ASSET_PACKS}" STREQUAL "false")
    add_definitions("-DNO_ASSET_PACKS")
else()
    # Add a static library called “playcore” built with the c++_static STL.
    include(${PLAYCORE_LOCATION}/playcore.cmake)
    add_playcore_static_library()
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${PLAYCORE_LOCATION}/include)

    target_link_libraries(${CMAKE_PROJECT_NAME} playcore)
endif()
