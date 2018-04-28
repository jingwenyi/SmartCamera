
#include "common.h"
#include "anyka_config.h"
#include "PTZControl.h"

#define BUF_SIZE (100)

/* 各种系统配置文件路径、目前为同一个 */

#define CONFIG_ANYKA_FILE_NAME  "/etc/jffs2/anyka_cfg.ini"

#define CONFIG_VIDEO_FILE_NAME  CONFIG_ANYKA_FILE_NAME
#define CONFIG_ALARM_FILE_NAME  CONFIG_ANYKA_FILE_NAME
#define CONFIG_CAMERA_FILE_NAME  CONFIG_ANYKA_FILE_NAME
#define CONFIG_CLOUD_FILE_NAME  CONFIG_ANYKA_FILE_NAME
#define CONFIG_ONVIF_FILE_NAME  CONFIG_ANYKA_FILE_NAME
#define CONFIG_ETH_FILE_NAME    CONFIG_ANYKA_FILE_NAME
#define CONFIG_RECORD_FILE_NAME  CONFIG_ANYKA_FILE_NAME
#define CONFIG_USER_FILE_NAME  CONFIG_ANYKA_FILE_NAME
#define CONFIG_WIFI_FILE_NAME  CONFIG_ANYKA_FILE_NAME
#define CONFIG_FTP_UPDATE_NAME  CONFIG_ANYKA_FILE_NAME
#define CONFIG_PTZ_UNHIT_NAME  "/tmp/ak_sys_start_flag"


/* 定义各种设置句柄 */
system_user_info *psystem_user_info = NULL;
video_setting *pvideo_default_config = NULL;
video_record_setting *psys_record_plan_info = NULL;
Psystem_alarm_set_info psys_alarm_info = NULL;
Psystem_cloud_set_info psys_cloud_info = NULL;
Psystem_onvif_set_info psys_onvif_info = NULL;
Psystem_net_set_info  psys_net_info = NULL;
Psystem_wifi_set_info   psys_wifi_info = NULL;
Pcamera_disp_setting    psys_camera_info = NULL;
system_ftp_update_info *psys_ftp_update_info = NULL;

/**
* @brief 	anyka_init_ftp_update_info
* 			初始化FTP升级配置
* @date 	2015/3
* @param:	system_ftp_update_info *psys_ftp_update_info
* @return 	int
* @retval
*/

int anyka_init_ftp_update_info(system_ftp_update_info *psys_ftp_update_info)
{
	char value[50];
    void *config_handle;
	
	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_FTP_UPDATE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return -1;
    }
	
	/*** ??ȡftp ?εĸ?????Ϣ????û????ʹ??Ĭ??ֵ ***/
    anyka_config_get_title(config_handle, "ftp_info", "ftp_server", value, "121.14.38.22");     
    strcpy(psys_ftp_update_info->ftp_server, value);
    anyka_config_get_title(config_handle, "ftp_info", "user_name", value, "anyka_ipc");  
    strcpy(psys_ftp_update_info->user_name, value);
    anyka_config_get_title(config_handle, "ftp_info", "ftp_pwd", value, "Anyka_Ipc");  
    strcpy(psys_ftp_update_info->password, value);
    anyka_config_get_title(config_handle, "ftp_info", "ftp_file_path", value, "IntCamPTZ/IntCam-A/");  
    strcpy(psys_ftp_update_info->ftp_file_path, value);
    anyka_config_get_title(config_handle, "ftp_info", "update_start_time", value, "2");  
    psys_ftp_update_info->update_start_time = atoi(value);
    anyka_config_get_title(config_handle, "ftp_info", "update_end_time", value, "4");  
    psys_ftp_update_info->update_end_time = atoi(value);

	/*** free resource ***/
    anyka_config_destroy(CONFIG_FTP_UPDATE_NAME, config_handle);
	return 0;
}


/**
* @brief 	anyka_get_sys_video_setting
*			获取系统视频设置信息
* @date 	2015/3
* @param:	void
* @return 	video_setting *
* @retval
*/

video_setting *anyka_get_sys_video_setting(void)
{
    return pvideo_default_config;
}


/**
* @brief 	anyka_get_record_plan
			获取计划录像句柄
* @date 	2015/3
* @param:	void
* @return 	video_record_setting *
* @retval	
*/
video_record_setting *anyka_get_record_plan(void)
{
    return psys_record_plan_info;
}

/**
* @brief 	anyka_init_video_param
			初始化视频编码参数
* @date 	2015/3
* @param:	video_setting  *para
* @return 	int
* @retval 	
*/

int anyka_init_video_param(video_setting *para)
{
    char value[50];
    void *config_handle;
	
	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_VIDEO_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }
	
	/*** ??ȡvideo ?εĸ?????Ϣ????û????ʹ??Ĭ??ֵ ***/
    anyka_config_get_title(config_handle, "video", "minqp", value, "10");     
    para->minQp = atoi(value);
    anyka_config_get_title(config_handle, "video", "maxqp", value, "36");  
    para->maxQp = atoi(value);
    anyka_config_get_title(config_handle, "video", "v720pfps", value, "10");  
    para->V720Pfps = atoi(value);
    anyka_config_get_title(config_handle, "video", "v720pminkbps", value, "250");  
    para->V720Pminkbps = atoi(value);
    anyka_config_get_title(config_handle, "video", "v720pmaxkbps", value, "500");  
    para->V720Pmaxkbps = atoi(value);
    anyka_config_get_title(config_handle, "video", "save_cyc_flag", value, "1");  
    para->save_cyc_flag = atoi(value);
    anyka_config_get_title(config_handle, "video", "savefilefps", value, "25");  
    para->savefilefps = atoi(value);
    anyka_config_get_title(config_handle, "video", "savefilekbps", value, "1024");  
    para->savefilekbps = atoi(value);
    anyka_config_get_title(config_handle, "video", "recpath", value, "/mnt/CYC_DV/");  
    strcpy(para->recpath , value);
    anyka_config_get_title(config_handle, "video", "vgapfps", value, "15");  
    para->VGAPfps = atoi(value);
    anyka_config_get_title(config_handle, "video", "vgaminkbps", value, "250");  
    para->VGAminkbps = atoi(value);
    anyka_config_get_title(config_handle, "video", "vgamaxkbps", value, "500");  
    para->VGAmaxkbps = atoi(value);
    anyka_config_get_title(config_handle, "video", "goplen", value, "2");  
    para->gopLen = atoi(value);
    anyka_config_get_title(config_handle, "video", "quality", value, "80");  
    para->quality = atoi(value);
    anyka_config_get_title(config_handle, "video", "pic_ch", value, "1");  
    para->pic_ch = atoi(value);	
    anyka_config_get_title(config_handle, "video", "video_mode", value, "0");  
    para->video_mode = atoi(value);

	/*** free resource ***/
    anyka_config_destroy(CONFIG_VIDEO_FILE_NAME, config_handle);
	return 0;
}


/**
* @brief 	anyka_init_sys_alarm_status
			初始化侦测配置信息
* @date 	2015/3
* @param:	Psystem_alarm_set_info palarm_info net_info
* @return 	uint8
* @retval 	
*/

uint8 anyka_init_sys_alarm_status(Psystem_alarm_set_info palarm_info)
{

    
    char value[50];
    void *config_handle;

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_ALARM_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }

	/*** ??ȡalarm ?εĸ?????Ϣ????û????ʹ??Ĭ??ֵ ***/
    anyka_config_get_title(config_handle, "alarm", "motion_detection", value, "3");  
    palarm_info->motion_detection = atoi(value);
    anyka_config_get_title(config_handle, "alarm", "motion_detection_1", value, "450");  
    palarm_info->motion_detection_1 = atoi(value);
    anyka_config_get_title(config_handle, "alarm", "motion_detection_2", value, "300");  
    palarm_info->motion_detection_2 = atoi(value);
    anyka_config_get_title(config_handle, "alarm", "motion_detection_3", value, "200");  
    palarm_info->motion_detection_3 = atoi(value);
    anyka_config_get_title(config_handle, "alarm", "opensound_detection", value, "0");  
    palarm_info->opensound_detection = atoi(value);
    anyka_config_get_title(config_handle, "alarm", "opensound_detection_1", value, "10");  
    palarm_info->opensound_detection_1 = atoi(value);
    anyka_config_get_title(config_handle, "alarm", "opensound_detection_2", value, "20");  
    palarm_info->opensound_detection_2 = atoi(value);
    anyka_config_get_title(config_handle, "alarm", "opensound_detection_3", value, "30");  
    palarm_info->opensound_detection_3 = atoi(value);
    anyka_config_get_title(config_handle, "alarm", "openi2o_detection", value, "0");  
    palarm_info->openi2o_detection = atoi(value);
    anyka_config_get_title(config_handle, "alarm", "smoke_detection", value, "0");  
    palarm_info->smoke_detection = atoi(value);
    anyka_config_get_title(config_handle, "alarm", "shadow_detection", value, "0");  
    palarm_info->shadow_detection = atoi(value);
    anyka_config_get_title(config_handle, "alarm", "other_detection", value, "0");  
    palarm_info->other_detection = atoi(value);
    anyka_config_get_title(config_handle, "alarm", "alarm_send_type", value, "0");  
    palarm_info->alarm_send_type = atoi(value);	
    anyka_config_get_title(config_handle, "alarm", "alarm_interval_time", value, "500");  
    palarm_info->alarm_interval_time = atoi(value);
    anyka_config_get_title(config_handle, "alarm", "alarm_default_record_dir", value, "/mnt/alarm_record_dir/");  
	strcpy(palarm_info->alarm_default_record_dir, value);
    anyka_config_get_title(config_handle, "alarm", "alarm_move_over_time", value, "60");  
    palarm_info->alarm_move_over_time = atoi(value);
    anyka_config_get_title(config_handle, "alarm", "alarm_record_time", value, "300");  
    palarm_info->alarm_record_time = atoi(value);	
    anyka_config_get_title(config_handle, "alarm", "alarm_send_msg_time", value, "60");  
    palarm_info->alarm_send_msg_time = atoi(value);	
    anyka_config_get_title(config_handle, "alarm", "alarm_save_record", value, "0");  
    palarm_info->alarm_save_record = atoi(value);
    anyka_config_get_title(config_handle, "alarm", "motion_size_x", value, "4");  
    palarm_info->motion_size_x = atoi(value);
    anyka_config_get_title(config_handle, "alarm", "motion_size_y", value, "4");
    palarm_info->motion_size_y = atoi(value);

	
	/*** free resource ***/
    anyka_config_destroy(CONFIG_ALARM_FILE_NAME, config_handle);

    return 0;
}

/**
* @brief 	anyka_init_sys_net_status
			初始化有线网配置信息
* @date 	2015/3
* @param:	Psystem_net_set_info net_info
* @return 	uint8
* @retval
*/
uint8 anyka_init_sys_net_status(Psystem_net_set_info net_info)
{
    char value[50];
    void *config_handle;

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_ETH_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }
    
	/*** ??ȡethernet ?εĸ?????Ϣ????û????ʹ??Ĭ??ֵ ***/
    anyka_config_get_title(config_handle, "ethernet", "dhcp", value, "1");  
    net_info->dhcp = atoi(value);
    anyka_config_get_title(config_handle, "ethernet", "ipaddr", value, "192.168.1.88");  
    strcpy(net_info->ipaddr, value);
    anyka_config_get_title(config_handle, "ethernet", "netmask", value, "255.255.0.0");  
    strcpy(net_info->netmask, value);
    anyka_config_get_title(config_handle, "ethernet", "gateway", value, "192.168.1.1");  
    strcpy(net_info->gateway, value);
    anyka_config_get_title(config_handle, "ethernet", "firstdns", value, "8.8.8.8");  
    strcpy(net_info->firstdns, value);
    anyka_config_get_title(config_handle, "ethernet", "backdns", value, "108.108.108.108");  
    strcpy(net_info->backdns, value);

	/*** free resource ***/
    anyka_config_destroy(CONFIG_ETH_FILE_NAME, config_handle);

    return 0;
}


/**
* @brief 	anyka_save_sys_net_info
			????ϵͳ??????Ϣ
* @date 	2015/3
* @param:	void
* @return 	uint8
* @retval 	?ɹ?0?? ʧ??1
*/

uint8 anyka_save_sys_net_info()
{
    char value[50];
    void *config_handle;

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_ETH_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }

	/*** ????ethernet ?εĸ?????Ϣ ***/
    sprintf(value,"%d", psys_net_info->dhcp );
    anyka_config_set_title(config_handle, "ethernet", "dhcp", value);  
    sprintf(value,"%s", psys_net_info->ipaddr );
    anyka_config_set_title(config_handle, "ethernet", "ipaddr", value);  
    sprintf(value,"%s", psys_net_info->netmask );
    anyka_config_set_title(config_handle, "ethernet", "netmask", value);  
    sprintf(value,"%s", psys_net_info->gateway );
    anyka_config_set_title(config_handle, "ethernet", "gateway", value);  
    sprintf(value,"%s", psys_net_info->firstdns );
    anyka_config_set_title(config_handle, "ethernet", "firstdns", value);  
    sprintf(value,"%s", psys_net_info->backdns );
    anyka_config_set_title(config_handle, "ethernet", "backdns", value);  

	/*** free resource ***/
    anyka_config_destroy(CONFIG_ETH_FILE_NAME, config_handle);
    
    return 0;
}


/**
* @brief 	anyka_get_net_info
			获取系统实时IP/NM/GW
* @date 	2015/3
* @param:	void
* @return 	void
* @retval 
*/

static void anyka_get_net_info(void)
{ 
    system("ifconfig eth0 >> /tmp/eth.txt");	//save ethnet info 
    int eth0_file;
    char *net_info = (char *)malloc(4096), *find_start, *find_end;
	int i = 0;
	char gateway[64]={0};
				
    if(net_info == NULL)
    {
        return;
    }
    eth0_file = open("/tmp/eth.txt", O_RDWR);	//open the info file 
    if(eth0_file < 0)
    {
        free(net_info);
        return ;
    }
    memset(net_info, 0, 4096);
    read(eth0_file, net_info, 4096);
    if(NULL != (find_start = strstr(net_info, "inet addr:")))	//find lable
    {
        find_start += strlen("inet addr:");
        find_end= strstr(find_start , " ");
		/** get info we need **/
        memset(psys_net_info->ipaddr, 0, sizeof(psys_net_info->ipaddr));
        memcpy(psys_net_info->ipaddr, find_start, (unsigned long)find_end - (unsigned long)find_start);
    }
    
    if(NULL != (find_start = strstr(net_info, "Mask:")))
    {
        find_start += strlen("Mask:");	//find lable
        find_end= strstr(find_start , "\x0a");
		/** get info we need **/
        memset(psys_net_info->netmask, 0, sizeof(psys_net_info->netmask));
        memcpy(psys_net_info->netmask, find_start, (unsigned long)find_end - (unsigned long)find_start);
    }
    close(eth0_file);
    free(net_info);
    system("rm -rf /tmp/eth.txt");	//remove the info file 


	do_syscmd("route | grep 'default' | awk '{print $2}'", gateway);
    while(gateway[i])
    {
        if (gateway[i]=='\n' || gateway[i]=='\r' || gateway[i]==' ')
            gateway[i] = '\0';
        i++;
    }

    memset(psys_net_info->gateway, 0, sizeof(psys_net_info->gateway));
    strcpy(psys_net_info->gateway, gateway);

}  


/**
* @brief 	anyka_get_sys_net_setting
			获取系统网络配置信息
* @date 	2015/3
* @param:	void
* @return 	Psystem_net_set_info
* @retval
*/

Psystem_net_set_info anyka_get_sys_net_setting(void)
{
    if(psys_net_info->dhcp == 1)
    {
        anyka_get_net_info();
    }
    return psys_net_info;
}



/**
* @brief 	anyka_set_sys_net_setting
*			设置系统网络配置信息
* @date 	2015/3
* @param:	Psystem_net_set_info user
* @return 	void
* @retval 	
*/

void anyka_set_sys_net_setting(Psystem_net_set_info user)
{
    memcpy(psys_net_info, user, sizeof(system_net_set_info));    
    anyka_save_sys_net_info();
}


/**
* @brief 	anyka_save_sys_wifi_info
			????wifi??Ϣ
* @date 	2015/3
* @param:	void
* @return 	uint8
* @retval 	?ɹ?0?? ʧ??1
*/

uint8 anyka_save_sys_wifi_info(void)
{
    char value[50];
    void *config_handle;

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_WIFI_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }
    
	/*** ????wireless ?εĸ?????Ϣ ***/
	//the ssid save to /tmp/wireless/
    //sprintf(value,"%s", psys_wifi_info->ssid );
    //anyka_config_set_title(config_handle, "wireless", "ssid", value); 
    sprintf(value,"%s", psys_wifi_info->passwd );
    anyka_config_set_title(config_handle, "wireless", "password", value); 

	/*** free resource ***/
    anyka_config_destroy(CONFIG_WIFI_FILE_NAME, config_handle);

    return 0;
}


/**
* @brief 	anyka_init_sys_wifi_status
			??ʼ??wifi״̬
* @date 	2015/3
* @param:	Psystem_wifi_set_info net_info
			ָ??Ҫ???õ?״̬?ṹ??ָ??
* @return 	uint8
* @retval 	?ɹ?0?? ʧ??1
*/

uint8 anyka_init_sys_wifi_status(Psystem_wifi_set_info net_info)
{
    char value[50];
    void *config_handle;

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_WIFI_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }

	/*** ??ȡsoftap ?κ?wireless ?εĸ?????Ϣ????û????ʹ??Ĭ??ֵ ***/
	anyka_config_get_title(config_handle, "softap", "s_ssid", value, "AKIPC_XXX");	
	strcpy(net_info->ssid, value);
	anyka_config_get_title(config_handle, "softap", "s_password", value, "");	
	strcpy(net_info->ssid, value);
	
    anyka_config_get_title(config_handle, "wireless", "ssid", value, "");  	
    strcpy(net_info->ssid, value);
    anyka_config_get_title(config_handle, "wireless", "mode", value, "Infra");  
    strcpy(net_info->mode, value);
    anyka_config_get_title(config_handle, "wireless", "password", value, "123456789");  
    strcpy(net_info->passwd, value);
    anyka_config_get_title(config_handle, "wireless", "security", value, "WPA/WPA2 PSK");  
	if(strstr(value, "WPA"))
	{
		net_info->enctype = WIFI_ENCTYPE_WPA2_TKIP;
	}
	else if(strstr(value, "WEP"))
	{
		net_info->enctype = WIFI_ENCTYPE_WEP;
	}
	else
	{
		net_info->enctype = WIFI_ENCTYPE_NONE;
	}

	/*** free resource ***/
    anyka_config_destroy(CONFIG_WIFI_FILE_NAME, config_handle);

    return 0;
}

/**
* @brief 	anyka_get_sys_wifi_setting
			获取系统WiFi配置信息
* @date 	2015/3
* @param:	void
* @return 	Psystem_wifi_set_info
* @retval
*/

Psystem_wifi_set_info anyka_get_sys_wifi_setting(void)
{
    return psys_wifi_info;
}


/**
* @brief 	anyka_set_sys_wifi_setting
			设置系统WiFi信息
* @date 	2015/3
* @param:	Psystem_wifi_set_info pointer
* @return 	void
* @retval 	
*/
void anyka_set_sys_wifi_setting(Psystem_wifi_set_info user)
{
    memcpy(psys_wifi_info, user, sizeof(system_wifi_set_info));    
    anyka_save_sys_wifi_info();
}

/**
* @brief 	anyka_get_sys_onvif_setting
			??ȡϵͳonvif ??????Ϣ
* @date 	2015/3
* @param:	void
* @return 	Psystem_onvif_set_info
* @retval 	ָ?????õ???Ϣ?ṹ??ָ??
*/

uint8 anyka_init_sys_onvif_status(Psystem_onvif_set_info onvif)
{
    char value[50];
    void *config_handle;

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_ONVIF_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }

	/*** ??ȡonvif ?εĸ?????Ϣ????û????ʹ??Ĭ??ֵ ***/
    anyka_config_get_title(config_handle, "onvif", "fps1", value, "25");  
    onvif->fps1 = atoi(value);
    anyka_config_get_title(config_handle, "onvif", "kbps1", value, "2048");  
    onvif->kbps1 = atoi(value);
    anyka_config_get_title(config_handle, "onvif", "quality1", value, "50");  
    onvif->quality1 = atoi(value);
    anyka_config_get_title(config_handle, "onvif", "fps2", value, "25");  
    onvif->fps2 = atoi(value);
    anyka_config_get_title(config_handle, "onvif", "kbps2", value, "800");  
    onvif->kbps2 = atoi(value);
    anyka_config_get_title(config_handle, "onvif", "quality2", value, "50");  
    onvif->quality2 = atoi(value);
    
	/*** free resource ***/
    anyka_config_destroy(CONFIG_ONVIF_FILE_NAME, config_handle);

    return 0;
    
}


/**
* @brief 	anyka_save_sys_onvif_info
			????ϵͳonvif ??????Ϣ
* @date 	2015/3
* @param:	void
* @return 	uint8
* @retval 	?ɹ?0?? ʧ??1
*/

uint8 anyka_save_sys_onvif_info()
{
    char value[50];
    void *config_handle;

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_ONVIF_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }
    
	/*** ????onvif ?εĸ?????Ϣ ***/
    sprintf(value, "%d", psys_onvif_info->fps1 );
    anyka_config_set_title(config_handle, "onvif", "fps1", value);  
    sprintf(value, "%d", psys_onvif_info->kbps1);
    anyka_config_set_title(config_handle, "onvif", "kbps1", value);  
    sprintf(value, "%d", psys_onvif_info->quality1); 
    anyka_config_set_title(config_handle, "onvif", "quality1", value);  
    sprintf(value, "%d", psys_onvif_info->fps2 );
    anyka_config_set_title(config_handle, "onvif", "fps2", value);  
    sprintf(value, "%d", psys_onvif_info->kbps2);
    anyka_config_set_title(config_handle, "onvif", "kbps2", value);  
    sprintf(value, "%d", psys_onvif_info->quality2);
    anyka_config_set_title(config_handle, "onvif", "quality2", value);  

	/*** free resource ***/
    anyka_config_destroy(CONFIG_ONVIF_FILE_NAME, config_handle);

    return 0;
}

/**
* @brief 	anyka_get_sys_onvif_setting
			获取ONVIF 平台配置信息
* @date 	2015/3
* @param:	void
* @return 	Psystem_onvif_set_info
* @retval
*/

Psystem_onvif_set_info anyka_get_sys_onvif_setting(void )
{
    return psys_onvif_info;
}

/**
* @brief 	anyka_set_sys_onvif_setting
*           获取ONVIF 平台配置信息
* @date 	2015/3
* @param:	Psystem_onvif_set_info user
* @return 	void
* @retval 	
*/

void anyka_set_sys_onvif_setting(Psystem_onvif_set_info user)
{
    memcpy(psys_onvif_info, user, sizeof(system_onvif_set_info));
    anyka_save_sys_onvif_info();
}

/**
* @brief 	anyka_init_sys_cloud_setting
			初始化系统云平台配置信息
* @date 	2015/3
* @param:	Psystem_cloud_set_info cloud
* @return 	Psystem_cloud_set_info
* @retval 	
*/

int anyka_init_sys_cloud_setting(Psystem_cloud_set_info cloud)
{
    char value[50];
    void *config_handle;

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_CLOUD_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }
    
	/*** ??ȡcloud ?εĸ?????Ϣ????û????ʹ??Ĭ??ֵ ***/
#ifdef CONFIG_ONVIF_SUPPORT
    anyka_config_get_title(config_handle, "cloud", "onvif", value, "1");  
#else
	anyka_config_get_title(config_handle, "cloud", "onvif", value, "0");  
#endif
    cloud->onvif = atoi(value);

#ifdef CONFIG_DANA_SUPPORT
    anyka_config_get_title(config_handle, "cloud", "dana", value, "1");  
#else
	anyka_config_get_title(config_handle, "cloud", "dana", value, "0");  
#endif
    cloud->dana = atoi(value);
    anyka_config_get_title(config_handle, "cloud", "tutk", value, "0");  
    cloud->tutk = atoi(value);
#ifdef CONFIG_TENCENT_SUPPORT
    anyka_config_get_title(config_handle, "cloud", "tencent", value, "1");  
#else
    anyka_config_get_title(config_handle, "cloud", "tencent", value, "0");  
#endif
    cloud->tencent= atoi(value);
    
	/*** free resource ***/
    anyka_config_destroy(CONFIG_CLOUD_FILE_NAME, config_handle);

    return 0;

}


/**
* @brief 	anyka_get_sys_cloud_setting
*			获取系统云平台配置信息			
* @date 	2015/3
* @param:	void 
* @return 	Psystem_cloud_set_info
* @retval
*/

Psystem_cloud_set_info anyka_get_sys_cloud_setting(void)
{
    return psys_cloud_info;
}

/**
* @brief 	anyka_sys_alarm_info
*			获取侦测信息句柄
* @date 	2015/3
* @param:	void 
* @return 	Psystem_alarm_set_info
* @retval
*/

Psystem_alarm_set_info anyka_sys_alarm_info(void)
{
    return psys_alarm_info;
}


/**
* @brief 	anyka_set_sys_alarm_status
			设置侦测信息
* @date 	2015/3
* @param:	void 
			
* @return 	uint8
* @retval 	
*/

uint8 anyka_set_sys_alarm_status(void)
{
    char value[50];
    void *config_handle;

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_ALARM_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }

	/***????alarm ?εĸ?????Ϣ ***/
	sprintf(value,"%d", psys_alarm_info->motion_detection );
    anyka_config_set_title(config_handle, "alarm", "motion_detection", value);  
    sprintf(value,"%d", psys_alarm_info->motion_detection_1 );
    anyka_config_set_title(config_handle, "alarm", "motion_detection_1", value);  
    sprintf(value,"%d", psys_alarm_info->motion_detection_2 );
    anyka_config_set_title(config_handle, "alarm", "motion_detection_2", value);  
    sprintf(value,"%d", psys_alarm_info->motion_detection_3 );
    anyka_config_set_title(config_handle, "alarm", "motion_detection_3", value);  
    sprintf(value,"%d", psys_alarm_info->opensound_detection );
    anyka_config_set_title(config_handle, "alarm", "opensound_detection", value);  
    sprintf(value,"%d", psys_alarm_info->opensound_detection_1 );
    anyka_config_set_title(config_handle, "alarm", "opensound_detection_1", value);  
    sprintf(value,"%d", psys_alarm_info->opensound_detection_2 );
    anyka_config_set_title(config_handle, "alarm", "opensound_detection_2", value);  
    sprintf(value,"%d", psys_alarm_info->opensound_detection_3 );
    anyka_config_set_title(config_handle, "alarm", "opensound_detection_3", value);  
    sprintf(value,"%d", psys_alarm_info->openi2o_detection );
    anyka_config_set_title(config_handle, "alarm", "openi2o_detection", value);  
    sprintf(value,"%d", psys_alarm_info->shadow_detection );
    anyka_config_set_title(config_handle, "alarm", "shadow_detection", value);  
    sprintf(value,"%d", psys_alarm_info->shadow_detection );
    anyka_config_set_title(config_handle, "alarm", "shadow_detection", value);  
    sprintf(value,"%d", psys_alarm_info->other_detection );
    anyka_config_set_title(config_handle, "alarm", "other_detection", value);  
    sprintf(value,"%d", psys_alarm_info->alarm_send_type );
    anyka_config_set_title(config_handle, "alarm", "alarm_send_type", value);  

	/*** free resource ***/
    anyka_config_destroy(CONFIG_ALARM_FILE_NAME, config_handle);

    return 0;
}


/**
* @brief 	anyka_init_record_plan_info
			初始化计划录像参数
* @date 	2015/3
* @param:	video_record_setting *psys_record_plan_info
* @return 	int
* @retval 	
*/


int anyka_init_record_plan_info(video_record_setting *psys_record_plan_info)
{
    char value[50];
    void *config_handle;

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_RECORD_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }

	/***??ȡrecord ?εĸ?????Ϣ????û????ʹ??Ĭ??ֵ ***/
    anyka_config_get_title(config_handle, "record", "video_index", value, "1");  
    psys_record_plan_info->video_index = atoi(value);
    anyka_config_get_title(config_handle, "record", "record_start_run1", value, "0");  
    psys_record_plan_info->record_plan[0].active = atoi(value);
    anyka_config_get_title(config_handle, "record", "record_start_time1", value, "0");  
    psys_record_plan_info->record_plan[0].start_time = atoi(value);
    anyka_config_get_title(config_handle, "record", "record_end_time1", value, "86399");  
    psys_record_plan_info->record_plan[0].end_time = atoi(value);
    anyka_config_get_title(config_handle, "record", "record_cyc1", value, "127");  
    psys_record_plan_info->record_plan[0].day_flag = atoi(value);
    anyka_config_get_title(config_handle, "record", "record_time", value, "5");  
    psys_record_plan_info->record_plan[0].record_time = atoi(value);
	
    anyka_config_get_title(config_handle, "record", "record_start_run2", value, "0");  
    psys_record_plan_info->record_plan[1].active = atoi(value);
    anyka_config_get_title(config_handle, "record", "record_start_time2", value, "68760");  
    psys_record_plan_info->record_plan[1].start_time = atoi(value);
    anyka_config_get_title(config_handle, "record", "record_end_time2", value, "83160");  
    psys_record_plan_info->record_plan[1].end_time = atoi(value);
    anyka_config_get_title(config_handle, "record", "record_cyc2", value, "1");  
    psys_record_plan_info->record_plan[1].day_flag = atoi(value);
	
    anyka_config_get_title(config_handle, "record", "record_start_run3", value, "0");  
    psys_record_plan_info->record_plan[2].active = atoi(value);
    anyka_config_get_title(config_handle, "record", "record_start_time3", value, "0");  
    psys_record_plan_info->record_plan[2].start_time = atoi(value);
    anyka_config_get_title(config_handle, "record", "record_end_time3", value, "33780");  
    psys_record_plan_info->record_plan[2].end_time = atoi(value);
    anyka_config_get_title(config_handle, "record", "record_cyc3", value, "16");  
    psys_record_plan_info->record_plan[2].day_flag = atoi(value);
	
	/*** free resource ***/
    anyka_config_destroy(CONFIG_RECORD_FILE_NAME, config_handle);

    return 0;
}


/**
* @brief 	anyka_set_record_plan_info
			设置计划录像参数
* @date 	2015/3
* @param:	video_record_setting *psys_record_plan_info
* @return 	int
* @retval 	
*/

int anyka_set_record_plan_info(video_record_setting *psys_record_plan_info)
{    
    char value[50];
    void *config_handle;

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_RECORD_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }

	/***????record ?εĸ?????Ϣ ***/
    sprintf(value, "%d", psys_record_plan_info->video_index );     
    anyka_config_set_title(config_handle, "record", "video_index", value);  
    sprintf(value, "%d", psys_record_plan_info->record_plan[0].active );
    anyka_config_set_title(config_handle, "record", "record_start_run1", value);  
    sprintf(value, "%d", psys_record_plan_info->record_plan[0].start_time);
    anyka_config_set_title(config_handle, "record", "record_start_time1", value);  
    sprintf(value, "%d", psys_record_plan_info->record_plan[0].end_time);
    anyka_config_set_title(config_handle, "record", "record_end_time1", value);  
    sprintf(value, "%d", psys_record_plan_info->record_plan[0].day_flag);
    anyka_config_set_title(config_handle, "record", "record_cyc1", value);   
    sprintf(value, "%d", psys_record_plan_info->record_plan[1].active );
    anyka_config_set_title(config_handle, "record", "record_start_run2", value);  
    sprintf(value, "%d", psys_record_plan_info->record_plan[1].start_time);
    anyka_config_set_title(config_handle, "record", "record_start_time2", value);  
    sprintf(value, "%d", psys_record_plan_info->record_plan[1].end_time);
    anyka_config_set_title(config_handle, "record", "record_end_time2", value);  
    sprintf(value, "%d", psys_record_plan_info->record_plan[1].day_flag);
    anyka_config_set_title(config_handle, "record", "record_cyc2", value);  
    sprintf(value, "%d", psys_record_plan_info->record_plan[2].active );
    anyka_config_set_title(config_handle, "record", "record_start_run3", value);  
    sprintf(value, "%d", psys_record_plan_info->record_plan[2].start_time);
    anyka_config_set_title(config_handle, "record", "record_start_time3", value);  
    sprintf(value, "%d", psys_record_plan_info->record_plan[2].end_time);
    anyka_config_set_title(config_handle, "record", "record_end_time3", value);  
    sprintf(value, "%d", psys_record_plan_info->record_plan[2].day_flag);
    anyka_config_set_title(config_handle, "record", "record_cyc3", value);  

	/*** free resource ***/
    anyka_config_destroy(CONFIG_RECORD_FILE_NAME, config_handle);

    return 0;
}


/**
* @brief 	anyka_init_camera_info
* 			初始化摄像头采集/显示参数
* @date 	2015/3
* @param:	Pcamera_disp_setting pcamera_disp_setting
* @return 	int
* @retval
*/

int anyka_init_camera_info(Pcamera_disp_setting pcamera_disp_setting)
{
    char value[50];
    void *config_handle;

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_CAMERA_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }

	/*** ??ȡcamera ?εĸ?????Ϣ????û????ʹ??Ĭ??ֵ ***/
    anyka_config_get_title(config_handle, "camera", "width", value, "1280");  
    pcamera_disp_setting->width = atoi(value); 
    anyka_config_get_title(config_handle, "camera", "height", value, "720");  
    pcamera_disp_setting->height = atoi(value); 
    anyka_config_get_title(config_handle, "camera", "osd_position", value, "1");  
    pcamera_disp_setting->osd_position = atoi(value);
    anyka_config_get_title(config_handle, "camera", "osd_switch", value, "1");  
    pcamera_disp_setting->osd_switch = atoi(value);
    anyka_config_get_title(config_handle, "camera", "osd_name", value, "\xe5\x8a\x9e\xe5\x85\xac\xe5\xae\xa4");  
    strcpy(pcamera_disp_setting->osd_name, value);
    pcamera_disp_setting->osd_unicode_name_len = font_utf8_to_unicode((char *)pcamera_disp_setting->osd_name, (char *)pcamera_disp_setting->osd_unicode_name);
    pcamera_disp_setting->osd_unicode_name_len >>= 1;
    font_lib_init(pcamera_disp_setting->osd_unicode_name, pcamera_disp_setting->osd_unicode_name_len);
    anyka_print("anyka_init_camera_info: %d, %s\n", pcamera_disp_setting->osd_unicode_name_len, pcamera_disp_setting->osd_name);
    anyka_config_get_title(config_handle, "camera", "time_switch", value, "1");  
    pcamera_disp_setting->time_switch = atoi(value);    
    anyka_config_get_title(config_handle, "camera", "date_format", value, "1");  
    pcamera_disp_setting->date_format = atoi(value);
    anyka_config_get_title(config_handle, "camera", "hour_format", value, "1");  
    pcamera_disp_setting->hour_format = atoi(value);
    anyka_config_get_title(config_handle, "camera", "week_format", value, "1");  
    pcamera_disp_setting->week_format = atoi(value);    
	anyka_config_get_title(config_handle, "camera", "ircut_mode", value, "0");  
    pcamera_disp_setting->ircut_mode = atoi(value);   
    
	/*** free resource ***/
    anyka_config_destroy(CONFIG_CAMERA_FILE_NAME, config_handle);
    return 0;

}


/**
* @brief 	anyka_set_camera_info
* 			设置摄像头采集/显示参数
* @date 	2015/3
* @param:	Pcamera_disp_setting pcamera_disp_setting
* @return 	int
* @retval
*/
int anyka_set_camera_info(Pcamera_disp_setting pcamera_disp_setting)
{
    void *config_handle;
	char value[50];

	anyka_debug("osd_name len : %d\n", strlen(pcamera_disp_setting->osd_name));

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_CAMERA_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }


	/***????camera ?εĸ?????Ϣ ***/
    sprintf(value, "%d", pcamera_disp_setting->width);    
    anyka_config_set_title(config_handle, "camera", "width", value);  
    sprintf(value, "%d", pcamera_disp_setting->height);
    anyka_config_set_title(config_handle, "camera", "height", value);
    sprintf(value, "%d", pcamera_disp_setting->osd_position);
    anyka_config_set_title(config_handle, "camera", "osd_position", value);
    sprintf(value, "%d", pcamera_disp_setting->osd_switch);
    anyka_config_set_title(config_handle, "camera", "osd_switch", value);
    sprintf(value, "%s", pcamera_disp_setting->osd_name);
    anyka_config_set_title(config_handle, "camera", "osd_name", value);  
    sprintf(value, "%d", pcamera_disp_setting->time_switch);
    anyka_config_set_title(config_handle, "camera", "time_switch", value);  
    sprintf(value, "%d", pcamera_disp_setting->date_format);
    anyka_config_set_title(config_handle, "camera", "date_format", value);  
    sprintf(value, "%d", pcamera_disp_setting->hour_format);
    anyka_config_set_title(config_handle, "camera", "hour_format", value);  
    sprintf(value, "%d", pcamera_disp_setting->week_format);
    anyka_config_set_title(config_handle, "camera", "week_format", value);  
	sprintf(value, "%d", pcamera_disp_setting->ircut_mode);
    anyka_config_set_title(config_handle, "camera", "ircut_mode", value); 
	
	/*** free resource ***/
    anyka_config_destroy(CONFIG_CAMERA_FILE_NAME, config_handle);
    
    return 0;
}

/**
* @brief    anyka_get_camera_info
*			获取摄像头采集/显示参数	
* @date 	2015/3
* @param:	void *con
			int para1
* @return 	Pcamera_disp_setting
* @retval 	
*/

Pcamera_disp_setting anyka_get_camera_info(void)
{
    return psys_camera_info;
}


/**
* @brief 	anyka_net_set_record_plan
* 			通过网络设置计划录像信息
* @date 	2015/3
* @param:	void *con
			int para1
* @return 	int
* @retval 	
*/

void anyka_net_set_record_plan(record_plan_info *plan)
{
    int index = 0;
    int record_time;

    if(plan->active)
    {
        for(index = 0; index < MAX_RECORD_PLAN_NUM; index ++)
        {
            if(psys_record_plan_info->record_plan[index].active == 0)
            {
                break;
            }
        }
        if(index != MAX_RECORD_PLAN_NUM)
        {
            record_time = psys_record_plan_info->record_plan[index].record_time;
            memcpy(&psys_record_plan_info->record_plan[index], plan, sizeof(record_plan_info));
            psys_record_plan_info->record_plan[index].record_time = record_time;
        }
    }
    else
    {
        for(index = 0; index < MAX_RECORD_PLAN_NUM; index ++)
        {
            if((psys_record_plan_info->record_plan[index].start_time == plan->start_time) &&
                (psys_record_plan_info->record_plan[index].end_time == plan->end_time) &&
                (psys_record_plan_info->record_plan[index].day_flag== plan->day_flag))
            {
                psys_record_plan_info->record_plan[index].active = 0;
                break;
            }
        }
    }
    anyka_set_record_plan_info(psys_record_plan_info);
}


/**
* @brief 	anyka_set_ptz_info
* 			设置云台转动信息 
* @date 	2015/3
* @param:	void *con
			int para1
* @return 	int
* @retval 	
*/
int anyka_set_ptz_info(void *con, int para1)
{
    struct ptz_pos *ptz_contrl = (struct ptz_pos *)con;

	char *title = "ptz";
	char name[50];
    char value[50];
    void *config_handle;
	
	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_USER_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }

	/*** ????ptz  ?εĸ?????Ϣ ***/
    sprintf(name, "p%dL", para1);
    sprintf(value, "%d", ptz_contrl[para1-1].left);
    anyka_config_set_title(config_handle, title, name, value);
	sprintf(name, "p%dU", para1);
    sprintf(value, "%d", ptz_contrl[para1-1].up);
    anyka_config_set_title(config_handle, title, name, value);
    
	/*** free resource ***/
    anyka_config_destroy(CONFIG_USER_FILE_NAME, config_handle);

	return 0;
}


/**
* @brief 	anyka_get_ptz_info
* 			获取云台转动信息 
* @date 	2015/3
* @param:	void *con
			int para1
* @return 	int
* @retval 	
*/

int anyka_get_ptz_info(void *con, int para1)
{
	struct ptz_pos * ptz_contrl = (struct ptz_pos *)con;
	
	char *title = "ptz";
	char name[50];
    char value[50];
    void *config_handle;

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_USER_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }
	
	/*** ??ȡptz  ?εĸ?????Ϣ????û????ʹ??Ĭ??ֵ ***/
    sprintf(name, "p%dL", para1);
    anyka_config_get_title(config_handle, title, name, value, "999");
	ptz_contrl[para1-1].left = atoi(value);
    sprintf(name, "p%dU", para1);
    anyka_config_get_title(config_handle, title, name, value, "999");
    ptz_contrl[para1-1].up = atoi(value);
    
	/*** free resource ***/
    anyka_config_destroy(CONFIG_USER_FILE_NAME, config_handle);

	return 0;
}


/**
* @brief 	anyka_set_ptz_unhit_info
* 			设置云台非撞击信息
* @date 	2015/3
* @param:	void *con
* @return 	int
* @retval 	
*/
int anyka_set_ptz_unhit_info(void *con)
{
    struct ptz_pos *ptz_contrl = (struct ptz_pos *)con;

	char *title = "ptz";
    char value[50];
    void *config_handle;

	if (access(CONFIG_PTZ_UNHIT_NAME, F_OK) != 0) {
		sprintf(value, "echo \"[%s]\" > %s", title, CONFIG_PTZ_UNHIT_NAME);
		system(value);
	}

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_PTZ_UNHIT_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }

	/*** ????ptz  ?εĸ?????Ϣ ***/
    sprintf(value, "%d", ptz_contrl[0].left);
    anyka_config_set_title(config_handle, title, "hitL", value);
    sprintf(value, "%d", ptz_contrl[0].up);
    anyka_config_set_title(config_handle, title, "hitU", value);
    
	/*** free resource ***/
    anyka_config_destroy(CONFIG_PTZ_UNHIT_NAME, config_handle);

	return 0;
}


/**
* @brief 	anyka_get_ptz_unhit_info
*			获取云台非撞击信息
* @date 	2015/3
* @param:	void *con
* @return 	int
* @retval 	
*/

int anyka_get_ptz_unhit_info(void *con)
{
	struct ptz_pos * ptz_contrl = (struct ptz_pos *)con;
	
	char *title = "ptz";
    char value[50];
    void *config_handle;

	if (access(CONFIG_PTZ_UNHIT_NAME, F_OK) != 0) {
		sprintf(value, "echo \"[%s]\" > %s", title, CONFIG_PTZ_UNHIT_NAME);
		system(value);
	}

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_PTZ_UNHIT_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }

	/*** ??ȡptz  ?εĸ?????Ϣ????û????ʹ??Ĭ??ֵ ***/
    anyka_config_get_title(config_handle, title, "hitL", value, "999");
	ptz_contrl[0].left = atoi(value);
    anyka_config_get_title(config_handle, title, "hitU", value, "999");
    ptz_contrl[0].up = atoi(value);
    
	/*** free resource ***/
    anyka_config_destroy(CONFIG_PTZ_UNHIT_NAME, config_handle);

	return 0;
}



/**
* @brief 	anyka_init_system_user_info
* 			初始化系统用户配置信息
* @date 	2015/3
* @param:   system_user_info pointer
* @return 	int
* @retval
*/

int anyka_init_system_user_info(system_user_info *psys_user_info)
{
    char value[50];
    void *config_handle;

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_USER_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return 1;
    }

	/*** ??ȡglobal  ?εĸ?????Ϣ????û????ʹ??Ĭ??ֵ ***/
    anyka_config_get_title(config_handle, "global", "user", value, "admin");  
    strcpy(psys_user_info->user, value);
    anyka_config_get_title(config_handle, "global", "secret", value, "admin");  
    strcpy(psys_user_info->secret, value);
    anyka_config_get_title(config_handle, "global", "debuglog", value, "0");  
    psys_user_info->debuglog =  atoi(value);
    anyka_config_get_title(config_handle, "global", "rtsp_support", value, "0");  
    psys_user_info->rtsp_support =  atoi(value);
    anyka_config_get_title(config_handle, "global", "firmware_update", value, "0");  
    psys_user_info->firmware_update =  atoi(value);
    anyka_config_get_title(config_handle, "global", "dev_name", value, "ak_ipc_dev_name");
	strcpy(psys_user_info->dev_name, value);
	anyka_config_get_title(config_handle, "global", "soft_version", value, "1000");
	psys_user_info->soft_version = atoi(value);
	
	/*** free resource ***/
    anyka_config_destroy(CONFIG_USER_FILE_NAME, config_handle);

    return 0;

}


/**
* @brief 	anyka_get_system_user_info
* 			获取系统用户配置信息
* @date 	2015/3
* @param:	void
* @param:   system_user_info pointer
* @retval 	
*/

system_user_info *anyka_get_system_user_info(void)
{
	return psystem_user_info;
}

/**
* @brief 	anyka_set_system_user_info
* 			设置系统用户配置信息
* @date 	2015/3
* @param:   system_user_info pointer
* @return 	int
* @retval
*/

int anyka_set_system_user_info(system_user_info * user_info)
{
	char *title = "global";
    char value[100];
    void *config_handle;

	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_USER_FILE_NAME);
    if(config_handle == NULL)
	{
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
		return -1;
	}
	
	/*** ??ptz  ???????????????? ***/
    sprintf(value, "%s", user_info->user);
	anyka_config_set_title(config_handle, title, "user", value);
    sprintf(value, "%s", user_info->secret);
	anyka_config_set_title(config_handle, title, "secret", value);
    sprintf(value, "%s", user_info->dev_name);
	anyka_config_set_title(config_handle, title, "dev_name", value);
    sprintf(value, "%d", user_info->firmware_update);
	anyka_config_set_title(config_handle, title, "firmware_update", value);
    sprintf(value, "%d", user_info->rtsp_support);
	anyka_config_set_title(config_handle, title, "rtsp_support", value);
    sprintf(value, "%u", user_info->soft_version);
	anyka_config_set_title(config_handle, title, "soft_version", value);

	/*** free resource ***/
    anyka_config_destroy(CONFIG_USER_FILE_NAME, config_handle);

	return 0;
}

/**
* @brief 	anyka_get_sys_ftp_update_info
* 			获取FTP升级配置
* @date 	2015/3
* @param:	void
* @return   system_ftp_update_info pointer
* @retval
*/
system_ftp_update_info *anyka_get_sys_ftp_update_info(void)
{
	return psys_ftp_update_info;
}

/**
* @brief 	anyka_init_update_config 	
* 		 	according soft version, modify some configure
* @date  	2016/4
* @param:	void
* @return  	void
* @retval 	void
*/
void anyka_init_update_config(unsigned int sys_version)
{
    char value[50];
    void *config_handle;
	
	/*** open config file ***/
    config_handle = anyka_config_init(CONFIG_VIDEO_FILE_NAME);
    if(config_handle == NULL)
    {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
        return ;
    }
	
	//???ݰ汾????????Ϣ, ע???汾??һ??Ҫд??, ??ʽ??Ҫ??
	if(psystem_user_info)
	{
		/** do update config file things if current version is new than the old **/
		if (sys_version > psystem_user_info->soft_version  && psys_cloud_info->tencent)
		{
			anyka_print("sys_version = %u\n", psystem_user_info->soft_version);
			if (psys_record_plan_info->record_plan[0].active != 1)
			{
				psys_record_plan_info->record_plan[0].active = 1;
				sprintf(value, "%d",psys_record_plan_info->record_plan[0].active);
				anyka_config_set_title(config_handle, "record", "record_start_run1", value);  
			}
			
			if (pvideo_default_config->savefilefps != 10)
			{
				pvideo_default_config->savefilefps = 10;
				sprintf(value, "%d",pvideo_default_config->savefilefps);
				anyka_config_set_title(config_handle, "video", "savefilefps", value);  
			}
			
			if (pvideo_default_config->savefilekbps != 256)
			{
				pvideo_default_config->savefilekbps = 256;
				sprintf(value, "%d",pvideo_default_config->savefilekbps);
				anyka_config_set_title(config_handle, "video", "savefilekbps", value);	
			}
	
			if (psys_alarm_info->alarm_save_record != 0)
			{
				psys_alarm_info->alarm_save_record = 0;
				sprintf(value, "%d",psys_alarm_info->alarm_save_record);
				anyka_config_set_title(config_handle, "alarm", "alarm_save_record", value);  
			}
			/** save the current system software version to config file **/
			sprintf(value, "%d", sys_version);
			anyka_config_set_title(config_handle, "global", "soft_version", value);
		}
	}
	else
	{
		anyka_print("[%s:%d] config file is newst.\n", __func__, __LINE__);
	}
	
	anyka_config_destroy(CONFIG_VIDEO_FILE_NAME, config_handle);
}
	
/**
* @brief 	anyka_init_setting
* 			初始化系统ini文件配置信息
* @date 	2015/3
* @param:	unsigned int  sys_version : 当前软件最新版本
* @return 	void
* @retval 	
*/

void anyka_init_setting(unsigned int sys_version)
{
	void *config_handle;

	anyka_config_init_lock();

	config_handle = anyka_config_init(CONFIG_ANYKA_FILE_NAME);
    if(NULL == config_handle)
    {
        anyka_print("[%s:%d] open config file failed, create it.\n", __func__, __LINE__);
		system("cp /usr/local/factory_cfg.ini /etc/jffs2/anyka_cfg.ini");
    }
	else
	{
		anyka_print("[%s:%d] anyka config file check ok.\n", __func__, __LINE__);
		anyka_config_destroy(CONFIG_ANYKA_FILE_NAME, config_handle);
	}

    pvideo_default_config = (video_setting *)malloc(sizeof(video_setting));
    psys_record_plan_info = (video_record_setting *)malloc(sizeof(video_record_setting)); 
    psys_alarm_info 	  = (Psystem_alarm_set_info)malloc(sizeof(system_alarm_set_info));
    psys_onvif_info 	  = (Psystem_onvif_set_info)malloc(sizeof(system_onvif_set_info));
    psys_cloud_info 	  = (Psystem_cloud_set_info)malloc(sizeof(system_cloud_set_info));
    psys_net_info 		  = (Psystem_net_set_info)malloc(sizeof(system_net_set_info));
    psys_wifi_info 		  = (Psystem_wifi_set_info)malloc(sizeof(system_wifi_set_info));
    psys_camera_info 	  = (Pcamera_disp_setting)malloc(sizeof(camera_disp_setting));
    psystem_user_info 	  = (system_user_info *)malloc(sizeof(system_user_info)); 
	psys_ftp_update_info  = (system_ftp_update_info*)malloc(sizeof(system_ftp_update_info));
	
	anyka_init_video_param(pvideo_default_config);			//init video 
    anyka_init_record_plan_info(psys_record_plan_info);		//init record plan 
    anyka_init_sys_alarm_status(psys_alarm_info);			//init alarm status 
    anyka_init_sys_onvif_status(psys_onvif_info);			//init onvif message
    anyka_init_sys_cloud_setting(psys_cloud_info);			//init cloud message
    anyka_init_sys_net_status(psys_net_info);				//init net status
    anyka_init_sys_wifi_status(psys_wifi_info);				//init wifi status
    anyka_init_camera_info(psys_camera_info);				//init camera info
    anyka_init_system_user_info(psystem_user_info);			//init user info
	anyka_init_ftp_update_info(psys_ftp_update_info);		//init ftp update info 

	anyka_init_update_config(sys_version);
}


