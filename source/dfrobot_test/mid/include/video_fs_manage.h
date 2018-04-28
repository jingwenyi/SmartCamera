#ifndef _video_fs_manage_h_
#define _video_fs_manage_h_
#include "basetype.h"

enum RECORD_APP
{
	RECORD_APP_PLAN,
	RECORD_APP_ALARM,
	RECORD_APP_NUM
};

typedef struct
{
	char prefix[32];
	char posfix[32];
}video_file_name_info;

typedef void video_find_record_callback(char *file_name, int file_size,int type);

/**
* @brief  video_fs_exist
* 		检测改文件(  路径)是否已经存在
* @date 	2015/3
* @param:	char * pstrFilePath，要检测的路径
* @return 	uint8
* @retval 	存在-->1，不存在-->0
*/

int video_fs_create_dir(char * pstrRecPath );

/**
* @brief  video_fs_get_free_size
* 			获取指定目录的剩余空间
* @date 	2015/3
* @param:	char * path， 指向要获取信息的路径的指针
* @return 	uint32 
* @retval 	success--> 指向当前路径的剩余空间， failed--> -1
*/

uint32 video_fs_get_free_size(char * path);

/**
* @brief  video_fs_init_record_list
* 			 初始化录像列表
* @date 	2015/3
* @param:	char * prec_path， 存放录像路径
			unsigned short type，RECORD_REPLAY_TYPE 计划录像累型，否则为移动侦测类型
			video_find_record_callback *pcallback， 用于排序的回调
* @return 	uint64 
* @retval 	success-->指定目录下符合需求的文件大小, 否则-->-1
*/
uint64 video_fs_init_record_list(char * prec_path, video_find_record_callback *pcallback, unsigned short type);


/**
* @brief  video_fs_get_audio_record_name
* 			根据参数构造音频录音文件名
* @date 	2015/3
* @param:	char * path，路径
			char * file_name ，文件名
			char *ext_name， 文件后缀名
* @return 	uint8 
* @retval 	success -->1, failed --> 0
*/

uint8 video_fs_get_audio_record_name(char * path, char *file_name, char *ext_name);

/**
* @brief  video_fs_get_video_record_name
* 			根据参数构造视频录像文件名
* @date 	2015/3
* @param:	char * path，路径
			char * file_name ，文件名
			char *ext_name， 文件后缀名
			time_t file_time ,     文件创建时间
* @return 	uint8 
* @retval 	success -->1, failed --> 0
*/

uint8 video_fs_get_video_record_name(char * path, char *file_name, char *ext_name,time_t file_time);


/**
 * NAME     	video_fs_get_alarm_record_name
 * @BRIEF	according to param alarm_type, create respectively file name.
 * @PARAM	void *, now no use
 * @RETURN	void *
 * @RETVAL	
 */
uint8 video_fs_get_alarm_record_name(int alarm_type,char * path, char *file_name, char *ext_name);

/**
* @brief  video_fs_get_audio_record_name
* 			根据参数构造拍照文件的文件名
* @date 	2015/3
* @param:	char * path，路径
			char * file_name ，文件名
* @return 	uint8 
* @retval 	success -->1, failed --> 0
*/
uint8 video_fs_get_photo_name(char * path, char *file_name);


/**
* @brief  video_fs_find_record_file
* 			 查找最终目录下类型匹配的文件 
* @date 	2015/3
* @param:	char * prec_path， 待比较的目录
			unsigned short type，RECORD_REPLAY_TYPE 计划录像累型，否则为移动侦测类型
			video_find_record_callback *pcallback， 用于排序的回调
* @return 	uint32 
* @retval 	success-->指定目录下符合需求的文件大小, 否则-->-1
*/
uint32 video_fs_find_record_file(char* prec_path , video_find_record_callback *pcallback, unsigned short type);


/**
* @brief  anyka_fs_get_system
* 			获取系统时间
* @date 	2015/3
* @param:	void
* @return 	struct tm *
* @retval 	success--> 指向当前系统时间的结构体指针， failed--> NULL
*/

struct tm * anyka_fs_get_system(void);


/**
* @brief  video_fs_get_file_name_cfg
* 			获取关于录像文件名的配置信息
* @date 	2015/9
* @param:	void
* @return 	 video_file_name_info * 
* @retval 	指向配置信息的一个数组, RECORD_APP作为索引
*/
const video_file_name_info * video_fs_get_file_name_cfg();

#endif


