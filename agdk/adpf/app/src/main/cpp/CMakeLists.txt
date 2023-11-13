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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Werror=unguarded-availability")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Werror=unguarded-availability")
add_definitions("-DSAMPLE_USES_IMPLOT")
add_definitions("-DIMGUI_IMPL_OPENGL_ES2")
add_definitions("-D__ANDROID_UNAVAILABLE_SYMBOLS_ARE_WEAK__")

# build Dear ImGui as a static lib
set(IMGUI_BASE_DIR "../../../../../third_party/imgui")

# build Implot as a static lib
set(IMPLOT_BASE_DIR "../../../../../third_party/implot")

# build bullet physics as a static lib
set(BULLET_BASE_DIR "../../../../../third_party/bullet3/src")

# source files common to all samples
set(COMMON_SRC_DIR "../../../../../common")

# common include directory for all samples
set(COMMON_INCLUDE_DIR "../../../../../common/include")

add_library(imgui STATIC
        ${IMGUI_BASE_DIR}/imgui_draw.cpp
        ${IMGUI_BASE_DIR}/imgui_tables.cpp
        ${IMGUI_BASE_DIR}/imgui_widgets.cpp
        ${IMGUI_BASE_DIR}/imgui.cpp
        ${IMGUI_BASE_DIR}/backends/imgui_impl_opengl3.cpp
        ${IMPLOT_BASE_DIR}/implot.cpp
        ${IMPLOT_BASE_DIR}/implot_items.cpp)

target_include_directories(imgui PRIVATE
        ${IMGUI_BASE_DIR})

#bullet3 library
AUX_SOURCE_DIRECTORY(${BULLET_BASE_DIR}/LinearMath BULLET_LINEARMATH)
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
        ${BULLET_LINEARMATH}
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

# Common source files shared with other samples
set(COMMON_SRCS
        ${COMMON_SRC_DIR}/ndk_helper/JNIHelper.cpp
        ${COMMON_SRC_DIR}/ndk_helper/Shader.cpp
        ${COMMON_SRC_DIR}/ndk_helper/TapCamera.cpp
        ${COMMON_SRC_DIR}/ndk_helper/VecMath.cpp
        ${COMMON_SRC_DIR}/src/adpf_manager.cpp
        ${COMMON_SRC_DIR}/src/box_renderer.cpp
        ${COMMON_SRC_DIR}/src/imgui_manager.cpp
        ${COMMON_SRC_DIR}/src/input_util.cpp
        ${COMMON_SRC_DIR}/src/scene.cpp
        ${COMMON_SRC_DIR}/src/scene_manager.cpp
        ${COMMON_SRC_DIR}/src/util.cpp)

# now build app's shared lib
add_library(game SHARED
        ${COMMON_SRCS}
        android_main.cpp
        demo_scene.cpp
        native_engine.cpp)

target_include_directories(game PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/ndk_helper
        ${COMMON_SRC_DIR}
        ${COMMON_INCLUDE_DIR}
        ${IMGUI_BASE_DIR}
        ${IMPLOT_BASE_DIR}
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
