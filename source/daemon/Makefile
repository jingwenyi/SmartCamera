include config.mk

LIBROOT_DIR = ../../../librootfs
LIBMEDIA_DIR = ../../rootfs

LOCAL_SRC_CFILES := \
		src/daemon_init.c              \
		src/daemon_inc.c              \
		src/anyka_ini.c              \
		src/daemon.c              \
		src/rtc.c					\
		
LOCAL_MODULE := daemon

LOCAL_CFLAGS := -D__LINUX__

PREFIX := arm-none-linux-gnueabi-

LOCAL_SHARED_LIBRARIES := pthread

LOCAL_C_INCLUDES :=	./include \
					./ \

LOCAL_LDFLAGS := -L./ \

ifeq ($(WATCHDOG_ENABLE), y)
	LOCAL_CFLAGS += -DWATCHDOG_ENABLE=1
endif
		
include exec_rules.mk
