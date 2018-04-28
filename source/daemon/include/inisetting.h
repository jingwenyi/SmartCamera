/**
* @brief  ini file head file
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval 
*/


#ifdef __cplusplus
extern "C" {
#endif


#include "iniparser.h"
#include "anyka_ini.h"

int IniSetting_init();
void IniSetting_destroy();
int IniSetting_save();
//get sys info
//get usr info
//get rec info
struct recoder_info* IniSetting_GetRecInfo();
//get wifi info
struct wireless_info* IniSetting_GetWilInfo();
//get Eth info
struct ethernet_info* IniSetting_GetEthInfo();
// get video info
struct video_info * IniSetting_GetVideoInfo();

struct recoder_info * IniSetting_GetRecordInfo(void);
struct picture_info *IniSetting_GetPictureInfo(void);
struct position_info *IniSetting_Getpositioninfo(void);
struct ini_alarm_info *IniSetting_GetAlarmInfo( void );
struct ini_update_info *IniSetting_GetupdateInfo(void);
struct ini_cloud_info *IniSetting_GetCloudInfo( void );
struct onvif_info * IniSetting_GetOnvifInfo(void);
struct ethernet_info* IniSetting_GetNetInfo(void);
struct camera_disp_info * IniSetting_GetCameraDispInfo(void);



// set wifi info
int IniSetting_SetWilInfo(struct wireless_info* wilinfo);
int IniSetting_SetEthInfo(struct ethernet_info* ethinfo);
//set video info
int IniSetting_SetVideoInfo(struct video_info * video);
int IniSetting_SetPictureInfo(struct picture_info * picture);
int IniSetting_SetupdateInfo(struct ini_update_info *info);
int IniSetting_SetAlarmInfo( struct ini_alarm_info * info );
//set rec info
int IniSetting_SetRecordInfo(struct recoder_info * precord);
int IniSetting_SetOnvifInfo(struct onvif_info * onvif);
int IniSetting_SetNetInfo(struct ethernet_info* net_info);
int IniSetting_SetCamerDispInfo(struct camera_disp_info * pcamera);

#ifdef __cplusplus
} /* end extern "C" */
#endif

