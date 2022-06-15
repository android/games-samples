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

find_package(oboe REQUIRED CONFIG)
find_package(game-activity REQUIRED CONFIG)
find_package(games-controller REQUIRED CONFIG)
find_package(games-frame-pacing REQUIRED CONFIG)
find_package(games-memory-advice REQUIRED CONFIG)
find_package(games-performance-tuner REQUIRED CONFIG)

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
set(COMMON_INCLUDE_DIR "../../../../../common/include")
set(COMMON_SRC_DIR "../../../../../common/src")

# Export GameActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u GameActivity_onCreate")

# Set common compiler options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14 -Wall -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
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

# now build app's shared lib
add_library(${CMAKE_PROJECT_NAME} SHARED
     ${PROTOBUF_NANO_SRCS}
     ${PROTOGEN_SRCS}
     android_main.cpp
     anim.cpp
     ascii_to_geom.cpp
     dialog_scene.cpp
     game_activity_included.cpp
     game_asset_manager.cpp
     game_asset_manifest.cpp
     game_text_input_included.cpp
     indexbuf.cpp
     input_util.cpp
     jni_util.cpp
     loader_scene.cpp
     loading_thread.cpp
     memory_consumer.cpp
     data_loader_machine.cpp
     native_app_glue_included.c
     native_engine.cpp
     obstacle.cpp
     obstacle_generator.cpp
     our_shader.cpp
     play_scene.cpp
     scene.cpp
     scene_manager.cpp
     sfxman.cpp
     shader.cpp
     shape_renderer.cpp
     tex_quad.cpp
     text_renderer.cpp
     texture.cpp
     texture_manager.cpp
     tuning_manager.cpp
     ui_scene.cpp
     util.cpp
     vertexbuf.cpp
     welcome_scene.cpp
     ${COMMON_SRC_DIR}/Versions.cpp
     )

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
     ${THIRD_PARTY_DIR}/glm/glm
     ${COMMON_INCLUDE_DIR}
     ${CMAKE_CURRENT_SOURCE_DIR}
     ${CMAKE_CURRENT_SOURCE_DIR}/data)

# add lib dependencies
target_link_libraries(${CMAKE_PROJECT_NAME}
     android
     atomic
     EGL
     game-activity::game-activity
     oboe::oboe
     games-controller::paddleboat_static
     games-frame-pacing::swappy_static
     games-performance-tuner::tuningfork_static
     games-memory-advice::memory_advice
     GLESv3
     glm
     log)

if("${USE_ASSET_PACKS}" STREQUAL "false")
    add_definitions("-DNO_ASSET_PACKS")
else()
    # Add a static library called “playcore” built with the c++_static STL.
    include(${PLAYCORE_LOCATION}/playcore.cmake)
    add_playcore_static_library()
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${PLAYCORE_LOCATION}/include)

    target_link_libraries(${CMAKE_PROJECT_NAME} playcore)
endif()
