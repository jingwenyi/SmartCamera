#
#Makefile
#

dir_y += updater
dir_y += disk_repair
dir_y += daemon
dir_y += smartlink
dir_y += anyka_ipc
dir_y += usb_serial
dir_y += dfrobot_test 
all:
	@for i in $(dir_y); \
	do \
		$(MAKE) -C $$i; \
	done

install:
	@for i in $(dir_y); \
	do \
		$(MAKE) -C $$i install; \
	done

image:

clean:
	@for i in $(dir_y); \
	do \
		$(MAKE) -C $$i clean; \
	done


.PHONY: all install image clean

