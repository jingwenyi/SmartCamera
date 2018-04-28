#! /bin/sh
### BEGIN INIT INFO
# File:				service.sh	
# Provides:         init service 
# Required-Start:   $
# Required-Stop:
# Default-Start:     
# Default-Stop:
# Short-Description:web service
# Author:			gao_wangsheng
# Email: 			gao_wangsheng@anyka.oa
# Date:				2012-12-27
### END INIT INFO

MODE=$1
PATH=$PATH:/bin:/sbin:/usr/bin:/usr/sbin

usage()
{
	echo "Usage: $0 start|stop)"
	exit 3
}

stop_service()
{    
	echo "stop daemon service......"
	killall -12 daemon
	sleep 5
	killall -9 daemon
	
    /usr/sbin/eth_manage.sh stop

	killall -9 cmd discovery anyka_ipc
}

start_service ()
{
	daemon &
	
	echo "start net service......"
	/usr/sbin/net_manage.sh

	/usr/sbin/anyka_ipc.sh start ##for onvif
}

restart_service ()
{
	echo "restart service......"
	stop_service
	start_service
}

#
# main:
#

case "$MODE" in
	start)
		start_service
		;;
	stop)
		stop_service
		;;
	restart)
		restart_service
		;;
	*)
		usage
		;;
esac
exit 0

