#!/bin/sh
# File:				recover_cfg.sh	
# Provides:         
# Description:      recover system config
# Author:			aj

#recover factory config ini
cp /usr/local/factory_cfg.ini /etc/jffs2/anyka_cfg.ini

#recover isp config ini
cp /usr/local/isp.conf /etc/jffs2/isp.conf 
cp /usr/local/isp_night.conf /etc/jffs2/isp_night.conf 
