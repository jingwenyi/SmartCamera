
CP = cp

MEDIALIB_LIB		:= ./include 

LOCAL_SRC_CFILES := \
		disk_repair.c 

LOCAL_MODULE := dsk_repair
PREFIX := arm-none-linux-gnueabi-


LOCAL_SHARED_LIBRARIES := $(LOCAL_SHARED_LIBRARIES)
LOCAL_C_INCLUDES := $(MEDIALIB_LIB)
LOCAL_LDFLAGS :=  -Xlinker -Map=output.map -lpthread
LOCAL_LDFLAGS += ./fatlib.a
LOCAL_CFLAGS := -MD -lpthread
include exec_rules.mk
