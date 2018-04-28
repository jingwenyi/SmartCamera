#
#Makefile
#

include rootfs.conf
TempDir := temp_dir
RootfsDir := ./rootfs
CameraSrcDir := ./ko
CameraDstDir := $(RootfsDir)
BUILD_JFFS2 := ./create_jffs2fs.sh
BUILD_SQAUSHFS := ./create_squashfs.sh
BUILD_LINK := ./create_linkage.sh

all:
	@echo "Do Nothing for rootfs, exit..."

image:
	@[ -L "$(RootfsDir)/etc/passwd" ] || mv $(RootfsDir)/etc/passwd $(RootfsDir)/etc/jffs2/
	@[ -L "$(RootfsDir)/etc/shadow" ] || mv $(RootfsDir)/etc/shadow $(RootfsDir)/etc/jffs2/
	@[ -L "$(RootfsDir)/etc/passwd" ] || [ -L "$(RootfsDir)/etc/shadow" ] || $(BUILD_LINK)
	@echo 'building root.jffs2 file system ...'
	@rm -f usr.jffs2
	@$(BUILD_JFFS2)

	@echo 'building root.sqsh4 file system ...'
	@rm -f usr.sqsh4 root.sqsh4
	@$(BUILD_SQAUSHFS)

install: extract.sh rootfs.tar.gz
	@rm -fr $(TempDir)
	@mkdir $(TempDir)
	@[ -d "rootfs" ] || ./extract.sh
	
	@[ -d $(CameraSrcDir) ] && \
		$(CP) -af $(CameraSrcDir)/* $(CameraDstDir)

	@for i in $(dir_y); \
	do \
		$(CP) -a $$i/* $(TempDir)/; \
	done
	@rm -f $(TempDir)/*.txt
	@rm -f $(TempDir)/README
	@rm -fr $(TempDir)/include
	@$(CP) -a $(TempDir)/* $(RootfsDir)
	@find $(RootfsDir) -type d -name "\.svn" | xargs rm -fr {}\;
	@rm -f $(RootfsDir)/*.txt
	@rm -f $(RootfsDir)/usr/lib/libasound.so*
	@$(CP) $(RootfsDir)/usr/local/isp.conf $(RootfsDir)/etc/jffs2/isp.conf
	@$(CP) $(RootfsDir)/usr/local/isp_night.conf $(RootfsDir)/etc/jffs2/isp_night.conf


######## profuct test mode config ########
ifeq ($(product_test), y)
	@$(CP) $(RootfsDir)/usr/local/test_cfg.ini  $(RootfsDir)/etc/jffs2/anyka_cfg.ini
else
	@$(CP) $(RootfsDir)/usr/local/factory_cfg.ini  $(RootfsDir)/etc/jffs2/anyka_cfg.ini
endif
	
	@rm -fr $(TempDir)
	@find $(RootfsDir) -name "*.sh" | xargs chmod u+x;

clean:
	@rm -fr rootfs rootfs.initramfs usr.jffs2 usr.sqsh4 root.sqsh4

.PHONY:all install image clean

