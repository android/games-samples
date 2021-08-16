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

include( ../protobuf.cmake )

include_directories(${PROTOBUF_SRC_DIR})
include_directories(${PROTOBUF_SRC_DIR}/..)

add_library( protobuf-static
  STATIC ${PROTOBUF_LITE_SRCS} ${PROTOBUF_SRCS}
)
target_compile_options(protobuf-static PUBLIC
  "-Wno-tautological-constant-compare" "-Wno-enum-compare-switch")

add_library( protobuf
  SHARED ${PROTOBUF_LITE_SRCS} ${PROTOBUF_SRCS}
)
target_compile_options(protobuf PUBLIC
  "-Wno-tautological-constant-compare" "-Wno-enum-compare-switch")
extra_pb_link_options(protobuf)
