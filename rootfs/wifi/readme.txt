
	本目录下的文件是目前ANYKA CLOUD39E项目平台支持的几款WiFi，请确保在了解了下面内容后再进行添加。
	一、增加WiFi
	1.每款WiFi对应一个文件夹，以其名称命名；
	2.文件夹中的usr/module/目录下保存的是根据当前内核编译好的WiFi驱动；
	  文件夹中的usr/module/firmware/目录下保存的是WiFi驱动相应的固件；
		文件夹中的usr/sbin/目录下保存的是WiFi驱动加载、卸载脚本。
		
	二、集成WiFi到平台：
	1.在rootfs/rootfs.conf 中增加对应WiFi配置
	2.在rootfs/Makefile 中增加将对应WiFi驱动脚本重命名代码
	3.在rootfs/rootfs.conf 中先配置WiFi后配置平台，这样就先拷贝wifi目录下的，
		而如果各个平台的驱动有什么不通用的地方，再通过各自平台下的文件进行覆盖即可。
