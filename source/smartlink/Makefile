LIBROOT_DIR = ../../../librootfs
ROOTFS_DIR = ../../rootfs

LOCAL_SRC_CFILES := \
		src/smartlink.c					\
		src/anyka_ini.c					\
		src/sys_interface.c				\
		src/convert.c				\
		src/realtek/realtek.c				\
		src/mtk/mt7601.c				\
		src/bcmdhd/bcmdhd.c				\
		src/bcmdhd/cooee.c				\
		src/bcmdhd/scan.c				\
		src/bcmdhd/akiss.c				\
		src/bcmdhd/easy_setup.c				\
		src/bcmdhd/neeze.c				\
				
LOCAL_MODULE := smartlink

LOCAL_CFLAGS := -D__LINUX__ -Wall

PREFIX := arm-none-linux-gnueabi-

LOCAL_C_INCLUDES :=	./include \
					../anyka_ipc/cloud/tencent/include \
					./ \
					./include/realtek \
					./include/mt7601 \
					./include/bcmdhd \

LOCAL_LDFLAGS := -L./ \
				 -L$(ROOTFS_DIR)/rootfs_tencent/lib \
				 -L$(ROOTFS_DIR)/rootfs_tencent/usr/lib \

LOCAL_SHARED_LIBRARIES := iconv txdevicesdk 

include exec_rules.mk
