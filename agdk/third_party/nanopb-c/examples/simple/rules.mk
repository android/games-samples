NANOPB_DIR := external/nanopb-0.3.9.1-linux-x86

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/simple.c \
	$(LOCAL_DIR)/simple.pb.c \
	$(NANOPB_DIR)/pb_encode.c \
	$(NANOPB_DIR)/pb_decode.c \
	$(NANOPB_DIR)/pb_common.c

MODULE_CPPFLAGS += -std=c++11

MODULE_INCLUDES += \
	$(NANOPB_DIR)

$(LOCAL_DIR)/simple.pb.c: $(LOCAL_DIR)/simple.proto
	$(PROTOC) $(PROTOC_OPTS) --nanopb_out=$(LOCAL_DIR) $(LOCAL_DIR)/simple.proto

include $(NANOPB_DIR)/extra/nanopb.mk
include make/module.mk

