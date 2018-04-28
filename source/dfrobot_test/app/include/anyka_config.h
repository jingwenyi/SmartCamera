#ifndef _anyka_config_h_
#define _anyka_config_h_

#define LEN				81
#define LEN32			32
#define LEN64			64
#define LEN96			96


//ap info
struct ap {
	int index;
	int security;
	char address[LEN];
	char ssid[LEN];
	char password[LEN];
	char protocol[LEN];
	char mode[LEN];
	char frequency[LEN];
	char en_key[LEN];
	char bit_rates[LEN];
	char sig_level[LEN];
};


//ap shop
struct ap_shop {
	int ap_num;
	struct ap ap_list[LEN32];
};


typedef struct _video_setting {
    int minQp;
    int maxQp;
    int V720Pfps;
    int V720PDefaultkbps;
    int V720Pminkbps;
    int V720Pmaxkbps;
    int save_cyc_flag;
    int savefilefps;
    int savefilekbps;
    int VGAPfps;
    int VGADefaultkbps;
    int VGAminkbps;
    int VGAmaxkbps;
    int gopLen;
	int	quality;
	int	pic_ch;
	int video_mode;
    char recpath[200];
}video_setting;

typedef struct _system_ftp_update_info{

    char ftp_server[20];	
    char user_name[20];
    char password[20];
    char ftp_file_path[50];
    int update_start_time;
    int update_end_time;
}system_ftp_update_info;

typedef struct _system_user_info{

    char user[20];
    char secret[20];
    char dev_name[100];
    unsigned short firmware_update;
    int debuglog;
	int rtsp_support;
	unsigned int soft_version;
}system_user_info;


typedef struct _pictrue_setting {

    int chanel;
}pictrue_setting;


typedef struct _system_net_set_info
{
	int dhcp;//0:Fixed IP  1:dynamic ip
	char ipaddr[16];//local ip
	char netmask[16];//net mask
	char gateway[16];
	char firstdns[16];//main dns
	char backdns[16];//second dns
}system_net_set_info, *Psystem_net_set_info;

#define MAX_RECORD_PLAN_NUM 3
typedef struct _record_plan_info
{
    int active;
    int day_flag;
    int start_time;
    int end_time;
	int record_time;
}record_plan_info, *Precord_plan_info;

typedef struct _video_record_setting
{
    int video_index;
    record_plan_info record_plan[MAX_RECORD_PLAN_NUM];
}video_record_setting;

typedef struct _camera_disp_setting
{
    int width;
    int height;
	int osd_position;
    int osd_switch;
    char osd_name[50];
    unsigned short osd_unicode_name[50];
    unsigned short osd_unicode_name_len;  
    int time_switch;
    int date_format;
    int hour_format;
    int week_format;    
	int ircut_mode;
}camera_disp_setting, *Pcamera_disp_setting;


typedef struct _system_alarm_set_info
{
    int motion_detection;	/* 移动侦测等级，取值 1/2/3/0, 1/2/3对应下面的3组值，0位关闭*/
    int motion_detection_1;
    int motion_detection_2;
    int motion_detection_3;
    int opensound_detection; /* 声音侦测等级，取值 1/2/3/0, 1/2/3对应下面的3组值，0位关闭*/
    int opensound_detection_1;
    int opensound_detection_2;
    int opensound_detection_3;
    int openi2o_detection; 
    int smoke_detection;
    int shadow_detection;
    int other_detection;
    int alarm_send_type;
    int alarm_save_record;  /* 是否保存录像标志 */
	int alarm_interval_time; /* 侦测间隔, ms */
	int alarm_move_over_time; /* 无动作后停止录像时间 */
	int alarm_record_time;	/* 单个录像文件时长 */
	int alarm_send_msg_time; /* 记录发送信息时间 */
    int motion_size_x;		/* 侦测时画面横向切割份数 */
    int motion_size_y;		/* 侦测时画面纵向切割份数 */
	char alarm_default_record_dir[200]; 	/* 录像文件名 */
}system_alarm_set_info, *Psystem_alarm_set_info;

typedef struct _system_onvif_set_info
{
    int fps1;
    int kbps1;
    int quality1;
    int fps2;
    int kbps2;
    int quality2;
}system_onvif_set_info, *Psystem_onvif_set_info;


typedef struct _system_cloud_set_info
{
    int onvif;
    int dana;
    int tutk;
    int tencent;
}system_cloud_set_info, *Psystem_cloud_set_info;


typedef enum {
	WIFI_ENCTYPE_INVALID,
	WIFI_ENCTYPE_NONE,
	WIFI_ENCTYPE_WEP,
	WIFI_ENCTYPE_WPA_TKIP,
	WIFI_ENCTYPE_WPA_AES,
	WIFI_ENCTYPE_WPA2_TKIP,
	WIFI_ENCTYPE_WPA2_AES
} WIFI_ENCTYPE;//the type of wifi

typedef struct _system_wifi_set_info {
	char ssid[32];
	char mode[32];
	char passwd[32];
	WIFI_ENCTYPE enctype;
}system_wifi_set_info, *Psystem_wifi_set_info;


typedef struct _CloudAlarm_INFO
{
	uint8 IsOpenMotionDetection;//close:1 open:0
	uint8 MontionDetectionLevel;//level: 0  1  2    2 is Highest

	uint8 IsOpenSoundDetection;
	uint8 SoundDetectionLevel;

	uint8 IsOpenI2ODetection;
	uint8 I2ODetectionLevel;

	uint8 IsOpenShadowDetection;
	uint8 ShadowDetectionLevel;
    
	uint8 IsOpenSmokeDetection;
	uint8 SmokeetectionLevel;

	uint8 IsOpenOther;
	uint8 OtherDetectionLevel;
}anyka_cloud_alarm_info, *Panyka_cloud_alarm_info;


/**
* @brief 	anyka_get_record_plan
			获取计划录像句柄
* @date 	2015/3
* @param:	void
* @return 	video_record_setting *
* @retval	
*/

video_record_setting *anyka_get_record_plan(void);

/**
* @brief 	anyka_init_video_param
			初始化视频编码参数
* @date 	2015/3
* @param:	video_setting  *para
* @return 	int
* @retval 	
*/

int anyka_init_video_param(video_setting *para);

/**
* @brief 	anyka_init_record_plan_info
			初始化计划录像参数
* @date 	2015/3
* @param:	video_record_setting *psys_record_plan_info
* @return 	int
* @retval 	
*/

int anyka_init_record_plan_info(video_record_setting *psys_record_plan_info);


/**
* @brief 	anyka_set_record_plan_info
			设置计划录像参数
* @date 	2015/3
* @param:	video_record_setting *psys_record_plan_info
* @return 	int
* @retval 	
*/

int anyka_set_record_plan_info(video_record_setting *psys_record_plan_info);


/**
* @brief 	anyka_get_record_plan
			获取计划录像参数句柄
* @date 	2015/3
* @param:	void
* @return 	video_record_setting *
* @retval
*/

video_record_setting * anyka_get_record_plan();

/**
* @brief 	anyka_init_sys_alarm_status
			初始化侦测配置信息
* @date 	2015/3
* @param:	Psystem_alarm_set_info palarm_info net_info
* @return 	uint8
* @retval 	
*/

uint8 anyka_init_sys_alarm_status(Psystem_alarm_set_info palarm_info);

/**
* @brief 	anyka_sys_alarm_info
*			获取侦测信息句柄
* @date 	2015/3
* @param:	void 
* @return 	Psystem_alarm_set_info
* @retval	
*/

Psystem_alarm_set_info anyka_sys_alarm_info(void);

/**
* @brief 	anyka_set_sys_alarm_status
			设置侦测信息
* @date 	2015/3
* @param:	void 
			
* @return 	uint8
* @retval 	
*/

uint8 anyka_set_sys_alarm_status(void);

/**
* @brief 	anyka_init_sys_net_status
			初始化有线网配置信息
* @date 	2015/3
* @param:	Psystem_net_set_info net_info
* @return 	uint8
* @retval
*/

uint8 anyka_init_sys_net_status(Psystem_net_set_info net_info);

/**
* @brief 	anyka_net_set_record_plan
* 			通过网络设置计划录像信息
* @date 	2015/3
* @param:	void *con
			int para1
* @return 	int
* @retval 	
*/

void anyka_net_set_record_plan(record_plan_info *plan);

/**
* @brief 	anyka_init_setting
* 			初始化系统ini文件配置信息
* @date 	2015/3
* @param:	unsigned int  sys_version : 当前软件最新版本
* @return 	void
* @retval 	
*/

void anyka_init_setting(unsigned int sys_version);

/**
* @brief 	anyka_set_ptz_info
* 			设置云台转动信息 
* @date 	2015/3
* @param:	void *con
			int para1
* @return 	int
* @retval 	
*/
int anyka_set_ptz_info(void *con, int para1);

/**
* @brief 	anyka_get_ptz_info
* 			获取云台转动信息 
* @date 	2015/3
* @param:	void *con
			int para1
* @return 	int
* @retval 	
*/

int anyka_get_ptz_info(void *con, int para1);

/**
* @brief 	anyka_set_ptz_unhit_info
* 			设置云台非撞击信息
* @date 	2015/3
* @param:	void *con
* @return 	int
* @retval 	
*/

int anyka_set_ptz_unhit_info(void *con);


/**
* @brief 	anyka_get_ptz_unhit_info
*			获取云台非撞击信息
* @date 	2015/3
* @param:	void *con
* @return 	int
* @retval 	
*/

int anyka_get_ptz_unhit_info(void *con);

/**
* @brief 	anyka_get_sys_video_setting
*			获取系统视频设置信息
* @date 	2015/3
* @param:	void
* @return 	video_setting *
* @retval	
*/

video_setting *anyka_get_sys_video_setting(void);

/**
* @brief 	anyka_init_sys_cloud_setting
			初始化系统云平台配置信息
* @date 	2015/3
* @param:	Psystem_cloud_set_info cloud
* @return 	Psystem_cloud_set_info
* @retval 	
*/

int anyka_init_sys_cloud_setting(Psystem_cloud_set_info cloud);

/**
* @brief 	anyka_get_sys_cloud_setting
*			获取系统云平台配置信息			
* @date 	2015/3
* @param:	void 
* @return 	Psystem_cloud_set_info
* @retval
*/

Psystem_cloud_set_info anyka_get_sys_cloud_setting(void);


/**
* @brief 	anyka_get_sys_onvif_setting
			获取ONVIF 平台配置信息
* @date 	2015/3
* @param:	void
* @return 	Psystem_onvif_set_info
* @retval
*/

Psystem_onvif_set_info anyka_get_sys_onvif_setting(void );

/**
* @brief 	anyka_set_sys_onvif_setting
*           获取ONVIF 平台配置信息
* @date 	2015/3
* @param:	Psystem_onvif_set_info user
* @return 	void
* @retval 	
*/

void anyka_set_sys_onvif_setting(Psystem_onvif_set_info user);

/**
* @brief 	anyka_get_sys_net_setting
			获取系统网络配置信息
* @date 	2015/3
* @param:	void
* @return 	Psystem_net_set_info
* @retval
*/

Psystem_net_set_info anyka_get_sys_net_setting(void);

/**
* @brief 	anyka_set_sys_net_setting
*			设置系统网络配置信息
* @date 	2015/3
* @param:	Psystem_net_set_info user
* @return 	void
* @retval 	
*/

void anyka_set_sys_net_setting(Psystem_net_set_info user);


/**
* @brief 	anyka_get_sys_wifi_setting
			获取系统WiFi配置信息
* @date 	2015/3
* @param:	void
* @return 	Psystem_wifi_set_info
* @retval
*/

Psystem_wifi_set_info anyka_get_sys_wifi_setting(void);

/**
* @brief 	anyka_set_sys_wifi_setting
			设置系统WiFi信息
* @date 	2015/3
* @param:	Psystem_wifi_set_info pointer
* @return 	void
* @retval 	
*/

void anyka_set_sys_wifi_setting(Psystem_wifi_set_info user);

/**
* @brief 	anyka_init_camera_info
* 			初始化摄像头采集/显示参数
* @date 	2015/3
* @param:	Pcamera_disp_setting pcamera_disp_setting
* @return 	int
* @retval
*/

int anyka_init_camera_info(Pcamera_disp_setting pcamera_disp_setting);


/**
* @brief 	anyka_set_camera_info
* 			设置摄像头采集/显示参数
* @date 	2015/3
* @param:	Pcamera_disp_setting pcamera_disp_setting
* @return 	int
* @retval
*/

int anyka_set_camera_info(Pcamera_disp_setting pcamera_disp_setting);

/**
* @brief    anyka_get_camera_info
*			获取摄像头采集/显示参数	
* @date 	2015/3
* @param:	void *con
			int para1
* @return 	Pcamera_disp_setting
* @retval 	
*/

Pcamera_disp_setting anyka_get_camera_info(void);

/**
* @brief 	anyka_init_system_user_info
* 			初始化系统用户配置信息
* @date 	2015/3
* @param:   system_user_info pointer
* @return 	int
* @retval
*/

int anyka_init_system_user_info(system_user_info *psys_user_info);

/**
* @brief 	anyka_set_system_user_info
* 			设置系统用户配置信息
* @date 	2015/3
* @param:   system_user_info pointer
* @return 	int
* @retval
*/

int anyka_set_system_user_info(system_user_info *psys_user_info);

/**
* @brief 	anyka_get_system_user_info
* 			获取系统用户配置信息
* @date 	2015/3
* @param:	void
* @param:   system_user_info pointer
* @retval 	
*/

system_user_info *anyka_get_system_user_info(void);

/**
* @brief 	anyka_init_ftp_update_info
* 			初始化FTP升级配置
* @date 	2015/3
* @param:	system_ftp_update_info *psys_ftp_update_info
* @return 	int
* @retval
*/

int anyka_init_ftp_update_info(system_ftp_update_info *psys_ftp_update_info);

/**
* @brief 	anyka_get_sys_ftp_update_info
* 			获取FTP升级配置
* @date 	2015/3
* @param:	void
* @return   system_ftp_update_info pointer
* @retval
*/
system_ftp_update_info *anyka_get_sys_ftp_update_info(void);

#endif

