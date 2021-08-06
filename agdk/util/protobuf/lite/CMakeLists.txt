cmake_minimum_required(VERSION 3.4.1)

include( ../protobuf.cmake )

include_directories(${PROTOBUF_SRC_DIR})
include_directories(${PROTOBUF_SRC_DIR}/..)

add_library( protobuf-lite-static
  STATIC
  ${PROTOBUF_LITE_SRCS}
)
target_compile_options(protobuf-lite-static PUBLIC
  "-Wno-tautological-constant-compare" "-Wno-enum-compare-switch")

add_library( protobuf-lite
  SHARED
  ${PROTOBUF_LITE_SRCS}
)
target_compile_options(protobuf-lite PUBLIC
  "-Wno-tautological-constant-compare" "-Wno-enum-compare-switch")
extra_pb_link_options(protobuf-lite)
