LIBROOT_DIR = ../../../librootfs
LIBMEDIA_DIR = ../../rootfs

LOCAL_SRC_CFILES := \
		usb_serial.c			
		
LOCAL_MODULE := usb_serial

LOCAL_CFLAGS := -D__LINUX__ -Wall

LOCAL_SHARED_LIBRARIES := pthread

PREFIX := arm-none-linux-gnueabi-

LOCAL_C_INCLUDES :=	./

LOCAL_LDFLAGS := -L./

include exec_rules.mk
