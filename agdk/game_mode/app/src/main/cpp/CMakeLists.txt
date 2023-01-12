#
# Copyright 2022 The Android Open Source Project
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
project(adpf_sample)

set(CMAKE_FIND_DEBUG_MODE TRUE)

find_package(game-activity REQUIRED CONFIG)
find_package(games-frame-pacing REQUIRED CONFIG)

# Retrieve module passes.
get_target_property(game-activity-include game-activity::game-activity INTERFACE_INCLUDE_DIRECTORIES)

# Export GameActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u GameActivity_onCreate")

# Set common compiler options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
add_definitions("-DIMGUI_IMPL_OPENGL_ES2")

# build Dear ImGui as a static lib
set(IMGUI_BASE_DIR "../../../../../third_party/imgui")

# build bullet physics as a static lib
set(BULLET_BASE_DIR "../../../../../third_party/bullet3/src")

# source files common to both the GameActivity and NativeActivity versions
set(COMMON_SRC_DIR "../../../../../common")

# common include directory for all samples
set(COMMON_INCLUDE_DIR "../../../../../common/include")

add_library(imgui STATIC
        ${IMGUI_BASE_DIR}/imgui_demo.cpp
        ${IMGUI_BASE_DIR}/imgui_draw.cpp
        ${IMGUI_BASE_DIR}/imgui_tables.cpp
        ${IMGUI_BASE_DIR}/imgui_widgets.cpp
        ${IMGUI_BASE_DIR}/imgui.cpp
        ${IMGUI_BASE_DIR}/backends/imgui_impl_opengl3.cpp)

target_include_directories(imgui PRIVATE
        ${IMGUI_BASE_DIR})

#bullet3 library
AUX_SOURCE_DIRECTORY(${BULLET_BASE_DIR}/LinearMath BULLET_LINEARMATHh)
AUX_SOURCE_DIRECTORY(${BULLET_BASE_DIR}/Bullet3Common BULLET_COMMON)
AUX_SOURCE_DIRECTORY(${BULLET_BASE_DIR}/BulletCollision/BroadphaseCollision BULLET_COLLISION_BC)
AUX_SOURCE_DIRECTORY(${BULLET_BASE_DIR}/BulletCollision/CollisionDispatch BULLET_COLLISION_CD)
AUX_SOURCE_DIRECTORY(${BULLET_BASE_DIR}/BulletCollision/CollisionShapes BULLET_COLLISION_CS)
AUX_SOURCE_DIRECTORY(${BULLET_BASE_DIR}/BulletCollision/NarrowPhaseCollision BULLET_COLLISION_NPC)
AUX_SOURCE_DIRECTORY(${BULLET_BASE_DIR}/BulletDynamics/ConstraintSolver BULLET_DYNAMICS_CS)
AUX_SOURCE_DIRECTORY(${BULLET_BASE_DIR}/BulletDynamics/Dynamics BULLET_DYNAMICS)
AUX_SOURCE_DIRECTORY(${BULLET_BASE_DIR}/BulletDynamics/Featherstone BULLET_DYNAMICS_FS)
AUX_SOURCE_DIRECTORY(${BULLET_BASE_DIR}/BulletDynamics/MLCPSolvers BULLET_DYNAMICS_MLCP)
AUX_SOURCE_DIRECTORY(${BULLET_BASE_DIR}/BulletDynamics/Vehicle BULLET_DYNAMICS_VEHICLE)
AUX_SOURCE_DIRECTORY(${BULLET_BASE_DIR}/BulletDynamics/Character BULLET_DYNAMICS_CHAR)
AUX_SOURCE_DIRECTORY(${BULLET_BASE_DIR}/Dynamics BULLET_DYNAMICS)
add_library(bullet3 STATIC
        ${BULLET_LINEARMATHh}
        ${BULLET_COMMON}
        ${BULLET_COLLISION_BC}
        ${BULLET_COLLISION_CD}
        ${BULLET_COLLISION_CS}
        ${BULLET_COLLISION_NPC}
        ${BULLET_DYNAMICS_CS}
        ${BULLET_DYNAMICS}
        ${BULLET_DYNAMICS_FS}
        ${BULLET_DYNAMICS_MLCP}
        ${BULLET_DYNAMICS_VEHICLEMLCP}
        ${BULLET_DYNAMICS_CHAR}
        )

target_include_directories(bullet3 PRIVATE
        ${BULLET_BASE_DIR}
        ${BULLET_BASE_DIR}/BulletCollision/CollisionDispatch)

# Ignore warnings for external dependency.
target_compile_options(bullet3
        PRIVATE
        -Wno-unused-variable
        -O0)

target_compile_definitions(bullet3 PRIVATE BT_THREADSAFE=1)

# now build app's shared lib
add_library(game SHARED
        adpf_manager.cpp
        android_main.cpp
        box_renderer.cpp
        demo_scene.cpp
        game_mode_manager.cpp
        imgui_manager.cpp
        input_util.cpp
        native_engine.cpp
        ndk_helper/JNIHelper.cpp
        ndk_helper/Shader.cpp
        ndk_helper/TapCamera.cpp
        ndk_helper/VecMath.cpp
        scene.cpp
        scene_manager.cpp
        util.cpp)

target_include_directories(game PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/ndk_helper
        ${COMMON_SRC_DIR}
        ${COMMON_INCLUDE_DIR}
        ${IMGUI_BASE_DIR}
        ${BULLET_BASE_DIR})

target_compile_options(game
        PRIVATE
        -Wall
        -Wextra-semi
        -Wshadow
        -Wshadow-field
        -O0
        "$<$<CONFIG:DEBUG>:-Werror>")

# add lib dependencies
target_link_libraries(game
        android
        bullet3
        imgui
        game-activity::game-activity_static
        games-frame-pacing::swappy_static
        atomic
        EGL
        GLESv2
        jnigraphics
        log
        z)
