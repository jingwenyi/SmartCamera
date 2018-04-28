#
#Makefile
#
WORKDIR:=$(shell pwd)
LIBFS:=$(WORKDIR)/../librootfs
ROOTFS_BIN_DIR ?= $(WORKDIR)/rootfs/rootfs/usr/bin
ROOTFS_SBIN_DIR ?= $(WORKDIR)/rootfs/rootfs/sbin
ROOTFS_CGI_DIR ?= $(WORKDIR)/rootfs/rootfs/usr/www/cgi-bin
MAKE:=make
CP:=cp -f

CFLAGS := -fno-strict-aliasing -Os
CXXFLAGS := -fno-strict-aliasing -Os

export MAKE CP LIBFS ROOTFS_BIN_DIR ROOTFS_SBIN_DIR ROOTFS_CGI_DIR CFLAGS CXXFLAGS

##rootfs dir MUST be placed at first
dir-y += rootfs 
dir-y += source


all:
	@for i in $(dir-y); \
	do \
		$(MAKE) -C $$i; \
	done

install:
	@for i in $(dir-y);	\
	do	\
		$(MAKE) -C $$i install;	\
	done

image:
	@for i in $(dir-y);	\
	do	\
		$(MAKE) -C $$i image; \
	done

clean:
	@for i in $(dir-y); \
	do \
		$(MAKE) -C $$i clean; \
	done


.PHONY: all install image clean

