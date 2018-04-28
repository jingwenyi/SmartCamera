#! /bin/sh
### BEGIN INIT INFO
# File:				wifi_run.sh	
# Provides:         manage wifi station and smartlink
# Required-Start:   $
# Required-Stop:
# Default-Start:     
# Default-Stop:
# Short-Description:start wifi run at station or smartlink
# Author:			
# Email: 			
# Date:				2012-8-8
### END INIT INFO

PATH=$PATH:/bin:/sbin:/usr/bin:/usr/sbin
MODE=$1
cfgfile="/etc/jffs2/anyka_cfg.ini"

play_please_config_net()
{
	echo "/usr/share/anyka_please_config_net.mp3" > /tmp/alarm_audio_list
	echo "play please config wifi tone"
	killall -12 anyka_ipc	## send signal to anyka_ipc
}

play_get_config_info()
{
	echo "/usr/share/anyka_camera_get_config.mp3" > /tmp/alarm_audio_list
	killall -12 anyka_ipc	## send signal to anyka_ipc
}

play_afresh_net_config()
{
	echo "/usr/share/anyka_afresh_net_config.mp3" > /tmp/alarm_audio_list
	echo "play please afresh config net tone"
	killall -12 anyka_ipc	## send signal to anyka_ipc
}

using_static_ip()
{
	ipaddress=`awk 'BEGIN {FS="="}/\[ethernet\]/{a=1} a==1&&$1~/^ipaddr/{gsub(/\"/,"",$2);gsub(/\;.*/, "", $2);gsub(/^[[:blank:]]*/,"",$2);print $2}' $cfgfile`
	
	netmask=`awk 'BEGIN {FS="="}/\[ethernet\]/{a=1} a==1&&$1~/^netmask/{gsub(/\"/,"",$2);gsub(/\;.*/, "", $2);gsub(/^[[:blank:]]*/,"",$2);print $2}' $cfgfile`
	gateway=`awk 'BEGIN {FS="="}/\[ethernet\]/{a=1} a==1&&$1~/^gateway/{gsub(/\"/,"",$2);gsub(/\;.*/, "", $2);gsub(/^[[:blank:]]*/,"",$2);print $2}' $cfgfile`

	ifconfig wlan0 $ipaddress netmask $netmask
	route add default gw $gateway
	sleep 1
}

station_start()
{
	### remove all wifi driver
	/usr/sbin/wifi_driver.sh uninstall

	## stop smartlink app
	

	## install station driver
	/usr/sbin/wifi_driver.sh station
	i=0
	###### wait until the wifi driver insmod finished.
	while [ $i -lt 3 ]
	do
		if [ -d "/sys/class/net/wlan0" ];then
			ifconfig wlan0 up
			break
		else
			sleep 1
			i=`expr $i + 1`
		fi
	done
	
	if [ $i -eq 3 ];then
		echo "wifi driver install error, exit"
		return 1
	fi

	echo "wifi driver install OK"
	/usr/sbin/wifi_station.sh start
	
	pid=`pgrep wpa_supplicant`
	if [ -z "$pid" ];then
		echo "the wpa_supplicant init failed, exit start wifi"
		return 1
	fi

	/usr/sbin/wifi_station.sh connect
	ret=$?
	echo "wifi connect return val: $ret"
	if [ $ret -eq 0 ];then
		if [ -d "/sys/class/net/eth0" ]
		then
			ifconfig eth0 down
			ifconfig eth0 up
		fi
		/usr/sbin/led.sh blink 4000 200
		echo "wifi connected!"
		return 0
	else
		echo "[station start] wifi station connect failed"
	fi

	return $ret
}

smartlink_start()
{
	/usr/sbin/wifi_driver.sh uninstall
	### start smartlink status led
	/usr/sbin/led.sh blink 1000 200
	/usr/sbin/wifi_driver.sh smartlink
	
	
}

#main
	
ssid=`awk 'BEGIN {FS="="}/\[wireless\]/{a=1} a==1 && $1~/^ssid/{gsub(/\"/,"",$2);
	gsub(/\;.*/, "", $2);gsub(/^[[:blank:]]*/,"",$2);print $2}' $cfgfile`

if [ "$ssid" = "" ]
then
    usb_serial & #start usb_serial here to burn tencent.conf

	sleep 3 #### sleep, wait the anyka_ipc load to the memery
	play_voice_flag=0
	while true
	do
		#wait anyka_ipc play the voice
		smartlink_start
	
		while true
		do
			if [ "$play_voice_flag" = "0" ];then
				#echo "we will check anyka_ipc"
				check_ipc=`pgrep anyka_ipc`
				if [ "$check_ipc" != "" ];then
					play_voice_flag=1
					sleep 2 
					play_please_config_net
				fi
			fi

			if [ -e "/tmp/wireless/gbk_ssid" ];then		
				play_get_config_info
				station_start
				if [ "$?" = 0 ];then
					### start station status led
					/usr/sbin/led.sh blink 4000 200
					exit 0
				else
					echo "connect failed, ret: $?, please check your ssid and password !!!"
					play_afresh_net_config
					#### clean config file and re-config
					cp /usr/local/factory_cfg.ini /etc/jffs2/anyka_cfg.ini	
					rm -rf /tmp/wireless/
					/usr/sbin/wifi_station.sh stop
					break
				fi
			fi
			sleep 1
		done
	done

	killall usb_serial
	rmmod g_file_storage
fi

station_start
