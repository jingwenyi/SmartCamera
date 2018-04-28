#! /bin/sh
### BEGIN INIT INFO
# File:				rtl8189.sh(wifi_driver.sh)	
# Provides:         8189 driver install and uninstall
# Required-Start:   $
# Required-Stop:
# Default-Start:     
# Default-Stop:
# Short-Description:install driver
# Author:			
# Email: 			
# Date:				2015-3-5
### END INIT INFO

PATH=$PATH:/bin:/sbin:/usr/bin:/usr/sbin
MODE=$1

usage()
{
	echo "Usage: $0 station | smartlink | ap | uninstall"
}
station_uninstall()
{
	#rm sdio wifi driver
	rmmod 8189es
}

station_install()
{
	#install sdio wifi driver
	insmod /usr/modules/8189es.ko
}

smartlink_uninstall()
{
	#rm usb wifi driver(default)
	rmmod 8189es
}

smartlink_install()
{
	#install sdio wifi driver
	insmod /usr/modules/8189es_smartlink.ko
}

ap_install()
{
	#install ap mode driver
	echo "install rtl8189 ap driver"
}

ap_uninstall()
{
	#uninstall ap mode driver
	echo "uninstall rtl8189 ap driver"
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


