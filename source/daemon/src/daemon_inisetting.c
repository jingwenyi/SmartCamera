/** @file
* @brief ini file process
*
* Copyright (C) 2006 Anyka (GuangZhou) Software Technology Co., Ltd.
* @author
* @date 2006-01-16
* @version 1.0
*/

#include "inisetting.h"
#include "daemon_inc.h"
#define INI_CAMERA "/etc/jffs2/anyka_cfg.ini"
#define INI_BAKE_CAMERA "/etc/bake/jffs2/anyka_cfg.ini"

#define daemon_settingDebug anyka_print
#define print_error anyka_print

static struct setting_info daemon_setting;
static int nInit = 0;
//int nInit = 0;

/**
* @brief  init  ini file lib
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
int IniSetting_init()
{
	if(nInit == 1)
	{
		daemon_settingDebug("The ini file has been opened, please close it first.\n");
		return -1;
	}

	daemon_setting.ini = iniparser_load(INI_CAMERA);
	if (NULL == daemon_setting.ini){
		print_error("ERROR: load config file fail!\n");
		return -2;
	}
	
	//daemon_settingDebug("load camera ini file:%s success\n", INI_CAMERA);
	//video info
	
	nInit = 1;
	return nInit;
}


/**
* @brief  destroy ini lib
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
void IniSetting_destroy()
{
	if(nInit == 1)
	{
		//printf("destroy inidaemon_setting \n");
		iniparser_freedict(daemon_setting.ini);
	}
	nInit = 0;
	return;
}


/**
* @brief  save file to ini file
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
int IniSetting_save()
{
	FILE *fp = NULL;
	
	if(!nInit)
		return -2;
	fp = fopen(INI_CAMERA, "w");
	if (NULL == fp){
		print_error("%s:open %s file fail!", __func__, INI_CAMERA);
		return -1;
	}

	iniparser_dump_ini(daemon_setting.ini, fp);
	fclose(fp);
	//daemon_settingDebug("save camera ini file:%s success\n", INI_CAMERA);
	system("rm -f /etc/jffs2/bake/anyka_cfg.ini");
    system("cp /etc/jffs2/anyka_cfg.ini /etc/jffs2/bake/anyka_cfg.ini");
	return 1;
}

struct onvif_info * IniSetting_GetOnvifInfo(void)
{
	if(!nInit)
		return NULL;
	
	daemon_setting.onvif.fps1 		= iniparser_getstring(daemon_setting.ini, "onvif:fps1", NULL);
	daemon_setting.onvif.kbps1 	= iniparser_getstring(daemon_setting.ini, "onvif:kbps1", NULL);
	daemon_setting.onvif.quality1 	= iniparser_getstring(daemon_setting.ini, "onvif:quality1", NULL);	
	daemon_setting.onvif.fps2 		= iniparser_getstring(daemon_setting.ini, "onvif:fps2", NULL);
	daemon_setting.onvif.kbps2 	= iniparser_getstring(daemon_setting.ini, "onvif:kbps2", NULL);
	daemon_setting.onvif.quality2 	= iniparser_getstring(daemon_setting.ini, "onvif:quality2", NULL);
	
	return &(daemon_setting.onvif);
}

int IniSetting_SetOnvifInfo(struct onvif_info * onvif)
{
	if(!onvif)
		return -2;
	if(!nInit)
		return -1;
	
	iniparser_set(daemon_setting.ini, "onvif:fps1", onvif->fps1);
	iniparser_set(daemon_setting.ini, "onvif:kbps1", onvif->kbps1);
	iniparser_set(daemon_setting.ini, "onvif:quality1", onvif->quality1);
	iniparser_set(daemon_setting.ini, "onvif:fps2", onvif->fps2);
	iniparser_set(daemon_setting.ini, "onvif:kbps2", onvif->kbps2);
	iniparser_set(daemon_setting.ini, "onvif:quality2", onvif->quality2);
	return 1;
}


/**
* @brief  read video infomation from ini file
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
struct video_info * IniSetting_GetVideoInfo()
{
	if(!nInit)
		return NULL;
	
	daemon_setting.video.minQpR = iniparser_getstring(daemon_setting.ini, "video:minQp", NULL);
	daemon_setting.video.maxQp = iniparser_getstring(daemon_setting.ini, "video:maxQp", NULL);
	daemon_setting.video.V720Pfps = iniparser_getstring(daemon_setting.ini, "video:V720Pfps", NULL);
	daemon_setting.video.V720Pminkbps = iniparser_getstring(daemon_setting.ini, "video:V720Pminkbps", NULL);
	daemon_setting.video.V720Pmaxkbps = iniparser_getstring(daemon_setting.ini, "video:V720Pmaxkbps", NULL);
	daemon_setting.video.save_cyc_flag = iniparser_getstring(daemon_setting.ini, "video:save_cyc_flag", NULL);
	daemon_setting.video.savefilefps = iniparser_getstring(daemon_setting.ini, "video:savefilefps", NULL);
	daemon_setting.video.savefilekbps= iniparser_getstring(daemon_setting.ini, "video:savefilekbps", NULL);
	daemon_setting.video.recpath = iniparser_getstring(daemon_setting.ini, "video:recpath", NULL);
	daemon_setting.video.VGAPfps = iniparser_getstring(daemon_setting.ini, "video:VGAPfps", NULL);
	daemon_setting.video.VGAminkbps = iniparser_getstring(daemon_setting.ini, "video:VGAminkbps", NULL);
	daemon_setting.video.VGAmaxkbps = iniparser_getstring(daemon_setting.ini, "video:VGAmaxkbps", NULL);
	daemon_setting.video.VGAWidth = iniparser_getstring(daemon_setting.ini, "video:VGAWidth", NULL);
	daemon_setting.video.VGAHeight = iniparser_getstring(daemon_setting.ini, "video:VGAHeight", NULL);
	daemon_setting.video.gopLen = iniparser_getstring(daemon_setting.ini, "video:gopLen", NULL);
	daemon_setting.video.quality = iniparser_getstring(daemon_setting.ini, "video:quality", NULL);
	
	
	return &(daemon_setting.video);
}


/**
* @brief  save viedo infomation to ini file
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
int IniSetting_SetVideoInfo(struct video_info * video)
{
	if(!video)
		return -2;
	if(!nInit)
		return -1;
	
	iniparser_set(daemon_setting.ini, "video:minQpR", video->minQpR);
	iniparser_set(daemon_setting.ini, "video:maxQp", video->maxQp);
	iniparser_set(daemon_setting.ini, "video:V720Pfps", video->V720Pfps);
	iniparser_set(daemon_setting.ini, "video:V720Pminkbps", video->V720Pminkbps);
	iniparser_set(daemon_setting.ini, "video:V720Pmaxkbps", video->V720Pmaxkbps);
	iniparser_set(daemon_setting.ini, "video:save_cyc_flag", video->save_cyc_flag);
	iniparser_set(daemon_setting.ini, "video:savefilefps", video->savefilefps);
	iniparser_set(daemon_setting.ini, "video:savefilekbps", video->savefilekbps);
	iniparser_set(daemon_setting.ini, "video:recpath", video->recpath);
	iniparser_set(daemon_setting.ini, "video:VGAPfps", video->VGAPfps);
	iniparser_set(daemon_setting.ini, "video:VGAmaxkbps", video->VGAmaxkbps);
	iniparser_set(daemon_setting.ini, "video:VGAWidth", video->VGAWidth);
	iniparser_set(daemon_setting.ini, "video:VGAHeight", video->VGAHeight);
	iniparser_set(daemon_setting.ini, "video:gopLen", video->gopLen);
	iniparser_set(daemon_setting.ini, "video:quality", video->quality);
	return 1;
}


/**
* @brief  get Record infomation from ini file
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
struct recoder_info * IniSetting_GetRecordInfo(void)
{
	if(!nInit)
		return NULL;
    daemon_setting.recoder.video_index = iniparser_getstring(daemon_setting.ini, "recoder:video_index", NULL);
    daemon_setting.recoder.record_start_run1 = iniparser_getstring(daemon_setting.ini, "recoder:record_start_run1", NULL);
    daemon_setting.recoder.record_start_time1 = iniparser_getstring(daemon_setting.ini, "recoder:record_start_time1", NULL);
    daemon_setting.recoder.record_end_time1 = iniparser_getstring(daemon_setting.ini, "recoder:record_end_time1", NULL);
    daemon_setting.recoder.record_cyc1 = iniparser_getstring(daemon_setting.ini, "recoder:record_cyc1", NULL);
    daemon_setting.recoder.record_flag1 = iniparser_getstring(daemon_setting.ini, "recoder:record_flag1", NULL);
    daemon_setting.recoder.record_start_run2 = iniparser_getstring(daemon_setting.ini, "recoder:record_start_run2", NULL);
    daemon_setting.recoder.record_start_time2 = iniparser_getstring(daemon_setting.ini, "recoder:record_start_time2", NULL);
    daemon_setting.recoder.record_end_time2 = iniparser_getstring(daemon_setting.ini, "recoder:record_end_time2", NULL);
    daemon_setting.recoder.record_cyc2 = iniparser_getstring(daemon_setting.ini, "recoder:record_cyc2", NULL);
    daemon_setting.recoder.record_flag2 = iniparser_getstring(daemon_setting.ini, "recoder:record_flag2", NULL);
    daemon_setting.recoder.record_start_run3 = iniparser_getstring(daemon_setting.ini, "recoder:record_start_run3", NULL);
    daemon_setting.recoder.record_start_time3 = iniparser_getstring(daemon_setting.ini, "recoder:record_start_time3", NULL);
    daemon_setting.recoder.record_end_time3 = iniparser_getstring(daemon_setting.ini, "recoder:record_end_time3", NULL);
    daemon_setting.recoder.record_cyc3 = iniparser_getstring(daemon_setting.ini, "recoder:record_cyc3", NULL);
    daemon_setting.recoder.record_flag3 = iniparser_getstring(daemon_setting.ini, "recoder:record_flag3", NULL);

	
    daemon_settingDebug("sdaemon_setting.recoder.video_index:%s\n", daemon_setting.recoder.video_index); 
    daemon_settingDebug("sdaemon_setting.recoder.record_start_run1:%s\n", daemon_setting.recoder.record_start_run1);
    daemon_settingDebug("sdaemon_setting.recoder.record_start_time1:%s\n", daemon_setting.recoder.record_start_time1);
    daemon_settingDebug("sdaemon_setting.recoder.record_end_time1:%s\n", daemon_setting.recoder.record_end_time1);
    daemon_settingDebug("sdaemon_setting.recoder.record_cyc1:%s\n", daemon_setting.recoder.record_cyc1);
    daemon_settingDebug("sdaemon_setting.recoder.record_flag1:%s\n", daemon_setting.recoder.record_flag1);
    daemon_settingDebug("sdaemon_setting.recoder.record_start_run2:%s\n", daemon_setting.recoder.record_start_run2);
    daemon_settingDebug("sdaemon_setting.recoder.record_start_time2:%s\n", daemon_setting.recoder.record_start_time2);
    daemon_settingDebug("sdaemon_setting.recoder.record_end_time2:%s\n", daemon_setting.recoder.record_end_time2);
    daemon_settingDebug("sdaemon_setting.recoder.record_cyc2:%s\n", daemon_setting.recoder.record_cyc2);
    daemon_settingDebug("sdaemon_setting.recoder.record_flag2:%s\n", daemon_setting.recoder.record_flag2);
    daemon_settingDebug("sdaemon_setting.recoder.record_start_run3:%s\n", daemon_setting.recoder.record_start_run3);
    daemon_settingDebug("sdaemon_setting.recoder.record_start_time3:%s\n", daemon_setting.recoder.record_start_time3);
    daemon_settingDebug("sdaemon_setting.recoder.record_end_time3:%s\n", daemon_setting.recoder.record_end_time3);
    daemon_settingDebug("sdaemon_setting.recoder.record_cyc3:%s\n", daemon_setting.recoder.record_cyc3);
    daemon_settingDebug("sdaemon_setting.recoder.record_flag3:%s\n", daemon_setting.recoder.record_flag3);
	
	return &(daemon_setting.recoder);
}


/**
* @brief  get Record infomation from ini file
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
int IniSetting_SetRecordInfo(struct recoder_info * precord)
{
	if(!precord)
		return -2;
	if(!nInit)
		return -1;

    iniparser_set(daemon_setting.ini, "recoder:video_index", precord->video_index);
    iniparser_set(daemon_setting.ini, "recoder:record_start_run1", precord->record_start_run1);
    iniparser_set(daemon_setting.ini, "recoder:record_start_time1", precord->record_start_time1);
    iniparser_set(daemon_setting.ini, "recoder:record_end_time1", precord->record_end_time1);
    iniparser_set(daemon_setting.ini, "recoder:record_cyc1", precord->record_cyc1);
    iniparser_set(daemon_setting.ini, "recoder:record_flag1", precord->record_flag1);
    iniparser_set(daemon_setting.ini, "recoder:record_start_run2", precord->record_start_run2);
    iniparser_set(daemon_setting.ini, "recoder:record_start_time2", precord->record_start_time2);
    iniparser_set(daemon_setting.ini, "recoder:record_end_time2", precord->record_end_time2);
    iniparser_set(daemon_setting.ini, "recoder:record_cyc2", precord->record_cyc2);
    iniparser_set(daemon_setting.ini, "recoder:record_flag2", precord->record_flag2);
    iniparser_set(daemon_setting.ini, "recoder:record_start_run3", precord->record_start_run3);
    iniparser_set(daemon_setting.ini, "recoder:record_start_time3", precord->record_start_time3);
    iniparser_set(daemon_setting.ini, "recoder:record_end_time3", precord->record_end_time3);
    iniparser_set(daemon_setting.ini, "recoder:record_cyc3", precord->record_cyc3);
    iniparser_set(daemon_setting.ini, "recoder:record_flag3", precord->record_flag3);

	return 1;
}


/**
* @brief  get Record infomation from ini file
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
struct camera_disp_info * IniSetting_GetCameraDispInfo(void)
{
	if(!nInit)
		return NULL;
    daemon_setting.camera.ch_name = iniparser_getstring(daemon_setting.ini, "camera:osd_name", NULL);
	
	return &(daemon_setting.camera);
}


/**
* @brief  get Record infomation from ini file
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
int IniSetting_SetCamerDispInfo(struct camera_disp_info * pcamera)
{
	if(!pcamera)
		return -2;
	if(!nInit)
		return -1;

    iniparser_set(daemon_setting.ini, "camera:osd_name", pcamera->ch_name);

	return 1;
}


/**
* @brief  get picture infomation from ini
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
struct picture_info *IniSetting_GetPictureInfo(void)
{
	if(!nInit)
		return NULL;

	daemon_setting.picture.video_index = iniparser_getstring(daemon_setting.ini, "picture:video_index", NULL);
	daemon_setting.picture.el_hz = iniparser_getstring(daemon_setting.ini, "picture:el_hz", NULL);
	daemon_setting.picture.osd_name = iniparser_getstring(daemon_setting.ini, "picture:osd_name", NULL);
	daemon_setting.picture.osd_place = iniparser_getstring(daemon_setting.ini, "picture:osd_place", NULL);
	daemon_setting.picture.osd_time = iniparser_getstring(daemon_setting.ini, "picture:osd_time", NULL);
	daemon_setting.picture.open = iniparser_getstring(daemon_setting.ini, "picture:open", NULL);
	daemon_setting.picture.interval = iniparser_getstring(daemon_setting.ini, "picture:interval", NULL);
	daemon_setting.picture.email = iniparser_getstring(daemon_setting.ini, "picture:email", NULL);
	daemon_setting.picture.data_palce = iniparser_getstring(daemon_setting.ini, "picture:data_palce", NULL);

	/*daemon_settingDebug("%s \n", daemon_setting.picture.video_index);
	daemon_settingDebug("%s \n", daemon_setting.picture.el_hz);
	daemon_settingDebug("%s \n", daemon_setting.picture.osd_name);
	daemon_settingDebug("%s \n", daemon_setting.picture.osd_place);
	daemon_settingDebug("%s \n", daemon_setting.picture.osd_time);
	daemon_settingDebug("%s \n", daemon_setting.picture.open);
	daemon_settingDebug("%s \n", daemon_setting.picture.interval);
	daemon_settingDebug("%s \n", daemon_setting.picture.email);
	daemon_settingDebug("%s \n", daemon_setting.picture.data_palce);*/
	
	return &(daemon_setting.picture);
}


/**
* @brief  save picture infomation to ini 
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
int IniSetting_SetPictureInfo(struct picture_info * picture)
{
	if(!picture)
		return -2;
	if(!nInit)
		return -1;

	iniparser_set(daemon_setting.ini, "picture:interval", picture->interval);
	iniparser_set(daemon_setting.ini, "picture:email", picture->email);
	iniparser_set(daemon_setting.ini, "picture:data_palce", picture->data_palce);

	return 1;
}

struct ethernet_info* IniSetting_GetNetInfo(void)
{
	if(!nInit)
		return NULL;
	
	//daemon_setting.wireless.open = iniparser_getstring(daemon_setting.ini, "wireless:open", NULL);
	daemon_setting.ethernet.dhcp = iniparser_getstring(daemon_setting.ini, "ethernet:dhcp", NULL);
	daemon_setting.ethernet.ipaddr = iniparser_getstring(daemon_setting.ini, "ethernet:ipaddr", NULL);
	daemon_setting.ethernet.netmask = iniparser_getstring(daemon_setting.ini, "ethernet:netmask", NULL);
	daemon_setting.ethernet.gateway = iniparser_getstring(daemon_setting.ini, "ethernet:gateway", NULL);
	daemon_setting.ethernet.firstdns = iniparser_getstring(daemon_setting.ini, "ethernet:firstdns", NULL);
	daemon_setting.ethernet.backdns = iniparser_getstring(daemon_setting.ini, "ethernet:backdns", NULL);
	
	return &(daemon_setting.ethernet);
}


int IniSetting_SetNetInfo(struct ethernet_info* net_info)
{
	if(!net_info)
		return -2;
	if(!nInit)
		return -1;
	iniparser_set(daemon_setting.ini, "ethernet:dhcp", net_info->dhcp);
	iniparser_set(daemon_setting.ini, "ethernet:ipaddr", net_info->ipaddr);
	iniparser_set(daemon_setting.ini, "ethernet:netmask", net_info->netmask);
	iniparser_set(daemon_setting.ini, "ethernet:gateway", net_info->gateway);
	iniparser_set(daemon_setting.ini, "ethernet:firstdns", net_info->firstdns);
	iniparser_set(daemon_setting.ini, "ethernet:backdns", net_info->backdns);
	
	return 1;	
}


/**
* @brief  get wifi infomation from ini
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
struct wireless_info* IniSetting_GetWilInfo()
{
	if(!nInit)
		return NULL;
	
	//daemon_setting.wireless.open = iniparser_getstring(daemon_setting.ini, "wireless:open", NULL);
	daemon_setting.wireless.ssid = iniparser_getstring(daemon_setting.ini, "wireless:ssid", NULL);
	daemon_setting.wireless.mode = iniparser_getstring(daemon_setting.ini, "wireless:mode", NULL);
	daemon_setting.wireless.security = iniparser_getstring(daemon_setting.ini, "wireless:security", NULL);
	daemon_setting.wireless.password = iniparser_getstring(daemon_setting.ini, "wireless:password", NULL);
	daemon_setting.wireless.running = iniparser_getstring(daemon_setting.ini, "wireless:running", NULL);
	
	//daemon_settingDebug("daemon_setting.wireless.open:%s\n", daemon_setting.wireless.open);
	daemon_settingDebug("daemon_setting.wireless.ssid:%s\n", daemon_setting.wireless.ssid);
	daemon_settingDebug("daemon_setting.wireless.mode:%s\n", daemon_setting.wireless.mode);
	daemon_settingDebug("daemon_setting.wireless.security:%s\n", daemon_setting.wireless.security);
	daemon_settingDebug("daemon_setting.wireless.password:%s\n", daemon_setting.wireless.password);
	daemon_settingDebug("daemon_setting.wireless.running:%s\n", daemon_setting.wireless.running);
	//ap_shop
	//daemon_settingDebug("daemon_setting.wireless.open:%s\n", daemon_setting.wireless.open);
	return &(daemon_setting.wireless);
}


/**
* @brief  save wifi infomation to ini
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
int IniSetting_SetWilInfo(struct wireless_info* wilinfo)
{
	if(!wilinfo)
		return -2;
	if(!nInit)
		return -1;
//	anyka_print("ssid:%s \n", wilinfo->ssid);
//	iniparser_set(daemon_setting.ini, "wireless:open", wilinfo->open);
	iniparser_set(daemon_setting.ini, "wireless:ssid", wilinfo->ssid);
	iniparser_set(daemon_setting.ini, "wireless:mode", wilinfo->mode);
	iniparser_set(daemon_setting.ini, "wireless:security", wilinfo->security);
	iniparser_set(daemon_setting.ini, "wireless:password", wilinfo->password);
	//iniparser_set(daemon_setting.ini, "wireless:running", wilinfo->running);

	return 1;	
}


/**
* @brief  get netcard infomation from ini 
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
struct ethernet_info* IniSetting_GetEthInfo()
{
	if(!nInit)
		return NULL;
	
	daemon_setting.ethernet.dhcp = iniparser_getstring(daemon_setting.ini, "ethernet:dhcp", NULL);
	daemon_setting.ethernet.ipaddr = iniparser_getstring(daemon_setting.ini, "ethernet:ipaddr", NULL);
	daemon_setting.ethernet.netmask = iniparser_getstring(daemon_setting.ini, "ethernet:netmask", NULL);
	daemon_setting.ethernet.gateway = iniparser_getstring(daemon_setting.ini, "ethernet:gateway", NULL);
	daemon_setting.ethernet.firstdns = iniparser_getstring(daemon_setting.ini, "ethernet:firstdns", NULL);
	daemon_setting.ethernet.backdns = iniparser_getstring(daemon_setting.ini, "ethernet:backdns", NULL);
/*
	daemon_settingDebug("daemon_setting.ethernet.dhcp:%s\n", daemon_setting.ethernet.dhcp);
	daemon_settingDebug("daemon_setting.ethernet.ipaddr:%s\n", daemon_setting.ethernet.ipaddr);
	daemon_settingDebug("daemon_setting.ethernet.netmask:%s\n", daemon_setting.ethernet.netmask);
	daemon_settingDebug("daemon_setting.ethernet.gateway:%s\n", daemon_setting.ethernet.gateway);
	daemon_settingDebug("daemon_setting.ethernet.firstdns:%s\n", daemon_setting.ethernet.firstdns);
	daemon_settingDebug("daemon_setting.ethernet.backdns:%s\n", daemon_setting.ethernet.backdns);
*/
	return &(daemon_setting.ethernet);
}


struct position_info *IniSetting_Getpositioninfo(void)
{
	if(!nInit)
		return NULL;

	daemon_setting.position.p1L = iniparser_getstring(daemon_setting.ini, "position:p1L", NULL);
	daemon_setting.position.p1U = iniparser_getstring(daemon_setting.ini, "position:p1U", NULL);
	daemon_setting.position.p2L = iniparser_getstring(daemon_setting.ini, "position:p2L", NULL);
	daemon_setting.position.p2U = iniparser_getstring(daemon_setting.ini, "position:p2U", NULL);
	daemon_setting.position.p3L = iniparser_getstring(daemon_setting.ini, "position:p3L", NULL);
	daemon_setting.position.p3U = iniparser_getstring(daemon_setting.ini, "position:p3U", NULL);
	daemon_setting.position.p4L = iniparser_getstring(daemon_setting.ini, "position:p4L", NULL);
	daemon_setting.position.p4U = iniparser_getstring(daemon_setting.ini, "position:p4U", NULL);
	daemon_setting.position.p5L = iniparser_getstring(daemon_setting.ini, "position:p5L", NULL);
	daemon_setting.position.p5U = iniparser_getstring(daemon_setting.ini, "position:p5U", NULL);
	daemon_setting.position.p6L = iniparser_getstring(daemon_setting.ini, "position:p6L", NULL);
	daemon_setting.position.p6U = iniparser_getstring(daemon_setting.ini, "position:p6U", NULL);

	return &(daemon_setting.position);
}



/**
* @brief  read alarm infomation from ini
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/

struct ini_alarm_info *IniSetting_GetAlarmInfo( void )
{
	if(!nInit)
	return NULL;

	daemon_setting.alarm.motion_detection = iniparser_getstring(daemon_setting.ini, "alarm:motion_detection", NULL);
	daemon_setting.alarm.motion_detection_1 = iniparser_getstring(daemon_setting.ini, "alarm:motion_detection_1", NULL);
	daemon_setting.alarm.motion_detection_2 = iniparser_getstring(daemon_setting.ini, "alarm:motion_detection_2", NULL);
	daemon_setting.alarm.motion_detection_3 = iniparser_getstring(daemon_setting.ini, "alarm:motion_detection_3", NULL);
	daemon_setting.alarm.opensound_detection = iniparser_getstring(daemon_setting.ini, "alarm:opensound_detection", NULL);
	daemon_setting.alarm.opensound_detection_1 = iniparser_getstring(daemon_setting.ini, "alarm:opensound_detection_1", NULL);
	daemon_setting.alarm.opensound_detection_2 = iniparser_getstring(daemon_setting.ini, "alarm:opensound_detection_2", NULL);
	daemon_setting.alarm.opensound_detection_3 = iniparser_getstring(daemon_setting.ini, "alarm:opensound_detection_3", NULL);
	daemon_setting.alarm.openi2o_detection = iniparser_getstring(daemon_setting.ini, "alarm:openi2o_detection", NULL);
	daemon_setting.alarm.smoke_detection = iniparser_getstring(daemon_setting.ini, "alarm:smoke_detection", NULL);
	daemon_setting.alarm.shadow_detection = iniparser_getstring(daemon_setting.ini, "alarm:shadow_detection", NULL);
	daemon_setting.alarm.other_detection = iniparser_getstring(daemon_setting.ini, "alarm:other_detection", NULL);

	//daemon_settingDebug(fmt...)

	return &(daemon_setting.alarm);
}

int IniSetting_SetAlarmInfo( struct ini_alarm_info * info )
{
	if( !info )
		return -2;
	if(!nInit)
		return -1;
	iniparser_set(daemon_setting.ini, "alarm:motion_detection", info->motion_detection);
	iniparser_set(daemon_setting.ini, "alarm:motion_detection_1", info->motion_detection_1);
	iniparser_set(daemon_setting.ini, "alarm:motion_detection_2", info->motion_detection_2);
	iniparser_set(daemon_setting.ini, "alarm:motion_detection_3", info->motion_detection_3);
	iniparser_set(daemon_setting.ini, "alarm:opensound_detection", info->opensound_detection);
	iniparser_set(daemon_setting.ini, "alarm:opensound_detection_1", info->opensound_detection_1);
	iniparser_set(daemon_setting.ini, "alarm:opensound_detection_2", info->opensound_detection_2);
	iniparser_set(daemon_setting.ini, "alarm:opensound_detection_3", info->opensound_detection_3);
	iniparser_set(daemon_setting.ini, "alarm:openi2o_detection", info->openi2o_detection);
	iniparser_set(daemon_setting.ini, "alarm:smoke_detection", info->smoke_detection);
	iniparser_set(daemon_setting.ini, "alarm:shadow_detection", info->shadow_detection);
	iniparser_set(daemon_setting.ini, "alarm:other_detection", info->other_detection);
	
	return 1;
	
}

struct ini_cloud_info *IniSetting_GetCloudInfo( void )
{
	if(!nInit)
	return NULL;

	daemon_setting.cloud.onvif = iniparser_getstring(daemon_setting.ini, "cloud:onvif", NULL);
	daemon_setting.cloud.dana = iniparser_getstring(daemon_setting.ini, "cloud:dana", NULL);
	daemon_setting.cloud.tutk = iniparser_getstring(daemon_setting.ini, "cloud:tutk", NULL);
	//daemon_settingDebug(fmt...)

	return &(daemon_setting.cloud);
}


/**
* @brief  read update infomation from  ini
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/

struct ini_update_info *IniSetting_GetupdateInfo(void)
{
	if(!nInit)
		return NULL;

	daemon_setting.update.devmodel = iniparser_getstring(daemon_setting.ini, "update:devmodel", NULL);
	daemon_setting.update.updateflag = iniparser_getstring(daemon_setting.ini, "update:updateflag", NULL);

	return &(daemon_setting.update);
}


/**
* @brief  save update infomation to ini
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
int IniSetting_SetupdateInfo(struct ini_update_info *info)
{
	if( !info )
		return -2;
	if(!nInit)
		return -1;
	iniparser_set(daemon_setting.ini, "update:devmodel", info->devmodel);
	iniparser_set(daemon_setting.ini, "storage:updateflag", info->updateflag);

	return 0;
}
