LIBROOT_DIR = ../../../librootfs
LIBMEDIA_DIR = ../../rootfs

LOCAL_SRC_CFILES := \
		fha_char_interface.c	\
		md5.c					\
		httpclient.c			\
		ftpclient.c				\
		updater.c				
		
LOCAL_MODULE := updater

LOCAL_CFLAGS := -D__LINUX__ -Wall

LOCAL_SHARED_LIBRARIES := pthread

PREFIX := arm-none-linux-gnueabi-

LOCAL_C_INCLUDES :=	./

LOCAL_LDFLAGS := -L./

include exec_rules.mk
