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
cmake_minimum_required(VERSION 3.18.1)
project(agdktunnel)

file(GLOB_RECURSE native_wrappers CONFIGURE_DEPENDS "native_wrappers/*.cpp" "native_wrappers/*.cc")

# (base_game_framework finds the other AGDK packages it uses: games-controller and
# games-frame-pacing)
find_package(oboe REQUIRED CONFIG)
find_package(game-activity REQUIRED CONFIG)
find_package(com.google.android.gms.gni.c REQUIRED CONFIG)

# common include directory for all samples
set(COMMON_INCLUDE_DIR "../../../../../common/include")
set(COMMON_SRC_DIR "../../../../../common/src")
set(SIMPLE_RENDERER_DIR "../../../../../common/simple_renderer")
set(BGF_ROOT_DIR "../../../../../common/base_game_framework")

# (Linking base_game_framework keeps GameActivity_onCreate exported, see
# https://github.com/android-ndk/ndk/issues/381)

# C++17, without compiler extensions (-std=c++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Common compiler options (warnings, no RTTI). These are set before adding
# base_game_framework and simple_renderer below, so they are compiled with them too. The
# optimization level comes from the build type (Debug: none, so -O0; release:
# RelWithDebInfo, -O2). The game's own defines are set on its target (below).
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wno-unused-private-field -Wno-nullability-completeness -Wno-thread-safety-analysis -Wno-unused-function -Wno-unused-variable" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wno-unused-private-field -Wno-nullability-completeness -Wno-thread-safety-analysis -Wno-unused-function -Wno-unused-variable -Werror=unguarded-availability" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti" )

set(THIRD_PARTY_DIR ../../../../../third_party)
# Import the CMakeLists.txt for the glm library
add_subdirectory(${THIRD_PARTY_DIR}/glm ${CMAKE_CURRENT_BINARY_DIR}/glm)
# glm's configuration (public: the glm library and the game must agree on it)
target_compile_definitions(glm PUBLIC GLM_FORCE_SIZE_T_LENGTH GLM_FORCE_RADIANS)

# BaseGameFramework and simple_renderer libraries (targets base_game_framework and
# simple_renderer)
add_subdirectory(${BGF_ROOT_DIR} ${CMAKE_CURRENT_BINARY_DIR}/base_game_framework)
add_subdirectory(${SIMPLE_RENDERER_DIR} ${CMAKE_CURRENT_BINARY_DIR}/simple_renderer)

# now build app's shared lib
add_library(${CMAKE_PROJECT_NAME} SHARED
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
     tunnel_engine.cpp
     ui_scene.cpp
     util.cpp
     vibration_helper.cpp
     welcome_scene.cpp
     ${COMMON_SRC_DIR}/Versions.cpp
     )

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
     _LIBCPP_ENABLE_THREAD_SAFETY_ANNOTATIONS
     GOOGLE_PROTOBUF_NO_RTTI
     HAVE_PTHREAD)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
     ${THIRD_PARTY_DIR}/glm/glm
     ${COMMON_INCLUDE_DIR}
     ${CMAKE_CURRENT_SOURCE_DIR}
     ${CMAKE_CURRENT_SOURCE_DIR}/data)

# add lib dependencies
target_link_libraries(${CMAKE_PROJECT_NAME}
     PUBLIC com.google.android.gms.gni.c::gni_static
     simple_renderer
     base_game_framework
     android
     atomic
     c++
     EGL
     game-activity::game-activity_static
     oboe::oboe
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
