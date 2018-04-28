#! /bin/sh
### BEGIN INIT INFO
# File:				camera.sh	
# Provides:         camera service 
# Required-Start:   $
# Required-Stop:
# Default-Start:     
# Default-Stop:
# Short-Description:web service
# Author:			gao_wangsheng
# Email: 			gao_wangsheng@anyka.oa
# Date:				2012-8-8
### END INIT INFO

MODE=$1
PATH=$PATH:/bin:/sbin:/usr/bin:/usr/sbin
mode=hostapd
network=
usage()
{
	echo "Usage: $0 start|stop)"
	exit 3
}

stop()
{
	killall -15 anyka_ipc
#	pid=`pgrep anyka_ipc`
#	while [ "$pid" != "" ]
#	do         
#	    sleep 0.5        
#		pid=`pgrep anyka_ipc`
#   done
	echo "we don't stop ipc service......"
}

start ()
{
	echo "start ipc service......"
	
	inifile="/etc/jffs2/anyka_cfg.ini"
	onvif=`awk 'BEGIN {FS="="}/\[cloud\]/{a=1} a==1&&$1~/onvif/{gsub(/\"/,"",$2);gsub(/\;.*/, "", $2);gsub(/^[[:blank:]]*/,"",$2);print $2}' $inifile`
	if [ "$onvif" = "1" ]
	then
		echo "start onvif service ......"
		pid=`pgrep cmd`
		if [ "$pid" = "" ]
		then
			cmd &    ##for onvif
		fi
		pid=`pgrep discovery`
		if [ "$pid" = "" ]
		then
			discovery & ##for onvif
		fi
	fi
	
	pid=`pgrep anyka_ipc`
    if [ "$pid" = "" ]
    then
	    anyka_ipc &
	fi
	
	
}

restart ()
{
	echo "restart ipc service......"
	stop
	start
}

#
# main:
#

case "$MODE" in
	start)
		start
		;;
	stop)
		stop
		;;
	restart)
		restart
		;;
	*)
		usage
		;;
esac
exit 0

