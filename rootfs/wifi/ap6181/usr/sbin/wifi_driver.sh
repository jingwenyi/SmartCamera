#! /bin/sh
### BEGIN INIT INFO
# File:				ap_6181.sh(wifi_driver.sh)	
# Provides:       	ap_6181 driver install and uninstall
# Required-Start:   $
# Required-Stop:
# Default-Start:     
# Default-Stop:
# Short-Description:install driver
# Author:			
# Email: 			
# Date:				2016-1-7
### END INIT INFO

PATH=$PATH:/bin:/sbin:/usr/bin:/usr/sbin
MODE=$1

usage()
{
	echo "Usage: $0 station | smartlink | ap | uninstall"
}

station_uninstall()
{
	#rmmod /usr/modules/bcmdhd.ko
	echo "station_uninstall::rmmod /usr/modules/bcmdhd.ko"
}

station_install()
{
	insmod /usr/modules/bcmdhd.ko "firmware_path=/usr/modules/firmware/fw_bcm40181a2.bin nvram_path=/usr/modules/firmware/nvram.txt iface_name=wlan0"
}

smartlink_uninstall()
{
	#rmmod /usr/modules/bcmdhd.ko
	echo "smartlink_uninstall::rmmod /usr/modules/bcmdhd.ko"
}

smartlink_install()
{
	insmod /usr/modules/bcmdhd.ko "firmware_path=/usr/modules/firmware/fw_bcm40181a2.bin nvram_path=/usr/modules/firmware/nvram.txt iface_name=wlan0"
}

ap_install()
{
	#install ap mode driver
	echo "install ap6181 driver"
}

ap_uninstall()
{
	#uninstall ap mode driver
	echo "uninstall ap6181 driver"
}

####### main

case "$MODE" in
	station)
		station_install
		;;
	smartlink)
		smartlink_install
		ifconfig wlan0 up
		iwconfig wlan0 mode monitor
		;;	
	ap)
		ap_install
		;;
	uninstall)
		station_uninstall
		smartlink_uninstall
		ap_uninstall
		;;
	*)
		usage
		;;
esac
exit 0


