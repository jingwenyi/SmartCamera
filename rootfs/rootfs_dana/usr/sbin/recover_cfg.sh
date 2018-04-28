#!/bin/sh
# File:				update.sh	
# Provides:         
# Description:      recover system configuration
# Author:			aj

play_recover_tip()
{
	echo "/usr/share/anyka_recover_device.mp3" > /tmp/alarm_audio_list
	killall -12 anyka_ipc
	sleep 5
}

play_recover_tip

#recover factory config ini
cp /usr/local/factory_cfg.ini /etc/jffs2/anyka_cfg.ini

#recover isp config ini
cp /usr/local/isp.conf /etc/jffs2/isp.conf 
cp /usr/local/isp_night.conf /etc/jffs2/isp_night.conf 

