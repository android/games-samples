cmake_minimum_required(VERSION 3.4.1)

include( ../protobuf.cmake )

include_directories(${PROTOBUF_NANO_SRC_DIR})

add_library( protobuf-nano_static
  STATIC ${PROTOBUF_NANO_SRCS})

add_library( protobuf-nano
  SHARED ${PROTOBUF_NANO_SRCS})

extra_pb_nano_link_options(protobuf-nano)
