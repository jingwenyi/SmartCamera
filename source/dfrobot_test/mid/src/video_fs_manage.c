
#include "common.h"
#include "video_fs_manage.h"
#include "record_replay.h"


#define VIDEO_FILE_PREFIX				"CYC_DV_"
#define ALARM_DIR_PREFIX				"alarm_"
#define OUR_PHOTO_PREFIX				"PHOTO_"
#define OUR_VIDEO_ALARM_FILE_PREFIX		"VIDEO_ALARM_"
#define OUR_AUDIO_ALARM_FILE_PREFIX		"AUDIO_ALARM_"
#define AUDIO_FILE_PREFIX				"AUDIO_"
#define RECORD_SAVE_SUF_NAME            ".mp4"
#define PHOTO_SAVE_SUF_NAME            	".jpg"


//所有录像应用文件名的配置
static video_file_name_info  g_video_file_name_cfg[RECORD_APP_NUM]=
{
	{"CYC_DV_",".mp4"}
		
}; 

/**
* @brief  video_fs_exist
* 		检测改文件(  路径)是否已经存在
* @date 	2015/3
* @param:	char * pstrFilePath，要检测的路径
* @return 	uint8
* @retval 	存在-->1，不存在-->0
*/

uint8 video_fs_exist(char *pstrFilePath)
{
	if (NULL == pstrFilePath)
		return 0;

	if (access(pstrFilePath, F_OK) == 0)
		return 1;

	return 0;
}


/**
* @brief  video_fs_create_dir
* 		创建目录
* @date 	2015/3
* @param:	char * pstrFilePath，要创建的路径
* @return 	int
* @retval 	0 -->success , -1--> failed
*/

int video_fs_create_dir(char * pstrRecPath )
{
	int iRet = 0;
	char *pstrTemp = NULL, *pszBackSlash = NULL;
	char *pstrPath = (char *)malloc((strlen(pstrRecPath) + 1));

	/** 检查内存分配 **/
	if (NULL == pstrRecPath) {
		anyka_print( "CompleteCreateDirectory::Out of memory!\n" );
		return -1;
	}

	strcpy(pstrPath, pstrRecPath);
	pstrTemp = strchr(pstrPath, '/');

	/*** 根据目录分割符: /，逐级创建目录 ***/
	while (1) {
		pszBackSlash = strchr(pstrTemp + 1, '/');
		if (NULL == pszBackSlash)
			break;
		
		*pszBackSlash= '\0';

		if (video_fs_exist(pstrPath)) {
			*pszBackSlash = '/';
			pstrTemp = pszBackSlash;
			continue;
		}

		if (mkdir(pstrPath, S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
			anyka_print("[%s:%d] mkdir %s error, %s\n", 
					__func__, __LINE__, pstrPath, strerror(errno));
			iRet = -1;
			goto Exit;
		}

		*pszBackSlash = '/';
        pstrTemp = pszBackSlash;
	}

	if ((mkdir(pstrPath, S_IRWXU | S_IRWXG | S_IRWXO) != 0) && (errno != EEXIST)) {
		anyka_print("[%s:%d] can't complete create dir %s! error = %s!\n",
			   	__func__, __LINE__, pstrPath, strerror(errno));
		iRet = -1;
	}
	/** 退出时释放资源 **/
Exit:
	free(pstrPath);
	return iRet;
}


/**
* @brief  anyka_fs_get_system
* 			获取系统时间
* @date 	2015/3
* @param:	void
* @return 	struct tm *
* @retval 	success--> 指向当前系统时间的结构体指针， failed--> NULL
*/

struct tm * anyka_fs_get_system(void)
{
	time_t now = {0};
	now = time(0);
//	char * pstrTime= NULL;

	struct tm *tnow = localtime(&now);
	if (NULL == tnow) {
		anyka_print("GetCurTime::get local time error!\n");
		return NULL;
	}

	return tnow;
}


/**
* @brief  video_fs_get_free_size
* 			获取指定目录的剩余空间
* @date 	2015/3
* @param:	char * path， 指向要获取信息的路径的指针
* @return 	uint32 
* @retval 	success--> 指向当前路径的剩余空间， failed--> -1
*/

uint32 video_fs_get_free_size(char * path)
{
	struct statfs disk_statfs;
    uint32 free_size;

	bzero(&disk_statfs, sizeof(struct statfs));
	while (statfs(path, &disk_statfs) == -1) {
		if (errno != EINTR) {
			anyka_print("statfs: %s Last error == %s\n", path, strerror(errno));
			return -1;
		}
	}
    free_size = disk_statfs.f_bsize;
    free_size = free_size / 512;
    free_size = free_size * disk_statfs.f_bavail / 2;

	return free_size;
}


/**
* @brief  video_fs_get_total_record_size
* 			获取指定目录的录像空间
* @date 	2015/3
* @param:	char * path， 指向要获取信息的路径的指针
* @return 	uint64 
* @retval 	当前返回0
*/

uint64 video_fs_get_total_record_size(char * path)
{
	return (uint64)0;
}

/**
* @brief  video_fs_check_record_file_name
* 			根据文件名和类型比较录像文件是否为当前系统的录像文件
* @date 	2015/3
* @param:	char * file_name， 待比较的文件名
			unsigned short type，RECORD_REPLAY_TYPE 计划录像累型，否则为移动侦测类型
* @return 	uint8 
* @retval 	是当前系统录制的文件-->1, 否则-->0
*/

uint8 video_fs_check_record_file_name(char * file_name, unsigned short type)
{
	video_file_name_info * pNameCfg;
	int file_len, cmp_len, record_file_len;
	
	pNameCfg = &g_video_file_name_cfg[type];
	cmp_len = strlen(pNameCfg->posfix);
    file_len = strlen(file_name);
    record_file_len = 6 + strlen(pNameCfg->prefix) + cmp_len;

    if (file_len < record_file_len) {
        anyka_print("[%s:%d] %s isn't anyka video record!, (%d,%d)\n",
			   		__func__, __LINE__,	file_name, file_len, record_file_len);
        return 0;
    }
	
	/**** first compare prefix ****/
    if (memcmp(file_name, pNameCfg->prefix, strlen(pNameCfg->prefix))) {
        anyka_print("[%s:%d] %s isn't anyka video record!, %s\n",
			   		__func__, __LINE__, file_name, pNameCfg->prefix);
        return 0;
    }

	/**** then compare suffix ****/
    if (memcmp(file_name + file_len - cmp_len, pNameCfg->posfix, cmp_len)) {
        anyka_print("[%s:%d] %s isn't anyka video record!, %s\n", 
					__func__, __LINE__, file_name, pNameCfg->posfix);
        return 0;
    }
		
    return 1;
}

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

/**** 查找最终目录下类型匹配的文件 ****/
/*** prec_path 绝对路径 ***/
uint32 video_fs_find_record_file(char* prec_path , video_find_record_callback *pcallback, unsigned short type)
{
	char astrFile[300];
    DIR *dirp = NULL;
	struct stat 	statbuf;
    struct dirent   *direntp = NULL; 
    uint32 totalSize = 0, cur_size;
	
	/** 打开目录 **/
    if ((dirp = opendir(prec_path)) == NULL){  
        anyka_print("[%s:%d] it fails to open directory %s\n",
			   	__func__, __LINE__, prec_path);
        return -1;
    }

	/** 逐个读取目录内容，进行判断 **/
    while ((direntp = readdir(dirp)) != NULL) {
        if(direntp->d_name == NULL)
        	continue;

        //如果是文件夹直接查找下一个 //if current file is directory,  go to find next one
        if ((direntp->d_type & 8) == 0)	
            continue;
        
		//调用比较函数比较文件
       	if (video_fs_check_record_file_name(direntp->d_name, type) == 0)		
            continue;

		//获取文件名的绝对路径
		sprintf(astrFile, "%s%s", prec_path, direntp->d_name); 

		/** get file information **/
		if (stat(astrFile, &statbuf) == -1) {
			anyka_print("[%s:%d] %d stat %s error, %s\n ",
				   	__func__, __LINE__,errno, astrFile, strerror(errno)); 
			if(2 == errno) { /* No such file or directory . means card is unplug*/
				closedir(dirp);
				return -1;
			}
			usleep(1000); /*for other thread to be schedule. such watchdog*/
			continue; 
		}

		/** 不是普通文件直接找下一个 **/
		if (!S_ISREG(statbuf.st_mode))
			continue;

		cur_size = (statbuf.st_size >> 10) + ((statbuf.st_size& 1023)?1:0);	//单位转换成k
		pcallback(astrFile, cur_size, type);		//调用callback，将数据写入队列
		totalSize += cur_size;				//更新总大小
	}

	/** close directory **/
    if (closedir(dirp) != 0)
		anyka_print("[%s:%d] close dir %s error, %s\n ",
			   	__func__, __LINE__, prec_path, strerror(errno));

	/** 返回当前指定的目录下符合要求的文件的总大小 **/
    return totalSize;
}

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

/*********************************************************************
查找所有的录像文件，并将其加入列表，统计文件大小
以便进行循环录像
**********************************************************************/
uint64 video_fs_init_record_list(char * prec_path, video_find_record_callback *pcallback, unsigned short type)
{
    video_fs_create_dir(prec_path);	//没有目录则创建
    
	//调用查找函数查找匹配的文件
	return video_fs_find_record_file(prec_path, pcallback, type);	
}



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

uint8 video_fs_get_video_record_name(char * path, char *file_name, char *ext_name,time_t file_time)
{
	int 	file_index = 0;
	char 	file[100];
	struct tm *tnow = localtime(&file_time);		
	
	/** format  **/
	sprintf(file, "%s%04d%02d%02d-%02d%02d%02d", VIDEO_FILE_PREFIX,
			1900 + tnow->tm_year, tnow->tm_mon + 1, tnow->tm_mday, 
			tnow->tm_hour, tnow->tm_min, tnow->tm_sec);	
	
    sprintf(file_name, "%s%s%s", path, file, ext_name);
	/** create by loop **/
	while (1) {
		/* check whether the file was exist */
		if (video_fs_exist(file_name)) {
			++file_index;
			if (file_index > 999) {
				anyka_print("[%s:%d] it fail to get record name!\n", __func__, __LINE__);
				return 0;
			}
			sprintf(file_name, "%s%s_%03d%s", path, file, file_index, RECORD_SAVE_SUF_NAME);			
			continue;
		}
		break;
	}

    return 1;
}

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
uint8 video_fs_get_audio_record_name(char * path, char *file_name, char *ext_name)
{
	int 	file_index = 0;
	char 	file[100];
	struct tm *tnow = anyka_fs_get_system();	//get system times

	/** format  **/
	sprintf(file, "%s%02d%02d%02d", AUDIO_FILE_PREFIX, tnow->tm_hour, tnow->tm_min, tnow->tm_sec);	
    sprintf(file_name, "%s%s%s", path, file, ext_name);

	/** create by loop **/
	while (1) {
		if (video_fs_exist(file_name)) {
			++file_index;
			if (file_index > 999) {
				anyka_print("[%s] it fail to get record name!\n", __func__);
				return 0;
			}
			sprintf(file_name, "%s%s_%03d%s", path, file, file_index, RECORD_SAVE_SUF_NAME);			
			continue;
		}
		break;
	}

    return 1;
}


/**
* @brief  video_fs_get_audio_record_name
* 			根据参数构造拍照文件的文件名
* @date 	2015/3
* @param:	char * path，路径
			char * file_name ，文件名
* @return 	uint8 
* @retval 	success -->1, failed --> 0
*/

uint8 video_fs_get_photo_name(char * path, char *file_name)
{
	int 	file_index = 0;
	char 	file[100];
	struct tm *tnow = anyka_fs_get_system();	//get system times
    /** format  **/
	sprintf(file, "%s%4d%02d%02d-%02d%02d%02d", OUR_PHOTO_PREFIX,
			1900 + tnow->tm_year, tnow->tm_mon + 1, tnow->tm_mday,
		   	tnow->tm_hour, tnow->tm_min, tnow->tm_sec);	
    sprintf(file_name, "%s%s%s", path, file, PHOTO_SAVE_SUF_NAME);
	/** create by loop **/
	while (1) {
		if (video_fs_exist(file_name)) {
			++file_index;
			if (file_index > 999) {
				anyka_print("[%s]it fail to get record name!\n", __func__);
				return 0;
			}
			sprintf(file_name, "%s%s_%03d%s", path, file, file_index, PHOTO_SAVE_SUF_NAME);
			continue;
		}
		break;
	}

    return 1;
}


/**
 * NAME     	video_fs_get_alarm_record_name
 * @BRIEF	according to param alarm_type, create respectively file name.
 * @PARAM	void *, now no use
 * @RETURN	void *
 * @RETVAL	
 */

uint8 video_fs_get_alarm_record_name(int alarm_type, char * path, char *file_name, char *ext_name)
{
	int 	file_index = 0;
	char 	file[100];
	struct tm *tnow = anyka_fs_get_system();	//get system times
	 /** format  **/
    if (alarm_type == SYS_CHECK_VIDEO_ALARM) {
	    sprintf(file, "%s%s%04d%02d%02d-%02d%02d%02d", 
				OUR_VIDEO_ALARM_FILE_PREFIX, VIDEO_FILE_PREFIX, 
				1900 + tnow->tm_year, tnow->tm_mon + 1, tnow->tm_mday, 
				tnow->tm_hour, tnow->tm_min, tnow->tm_sec);	
    } else {
	    sprintf(file, "%s%s%04d%02d%02d-%02d%02d%02d", 
				OUR_AUDIO_ALARM_FILE_PREFIX, VIDEO_FILE_PREFIX,
			   	1900 + tnow->tm_year, tnow->tm_mon + 1, tnow->tm_mday,
			  	tnow->tm_hour, tnow->tm_min, tnow->tm_sec);	
    }
    sprintf(file_name, "%s%s%s", path, file, ext_name);

	/** create by loop **/
	while (1) {
		if (video_fs_exist(file_name)) {
			++file_index;
			if (file_index > 999) {
				anyka_print("[%s] it fail to get record name!\n", __func__);
				return 0;
			}
			sprintf(file_name, "%s%s_%03d%s", path, file, file_index, RECORD_SAVE_SUF_NAME);			
			continue;
		}
		break;
	}

    return 1;
}

/**
* @brief  video_fs_get_file_name_cfg
* 			获取关于录像文件名的配置信息
* @date 	2015/9
* @param:	void
* @return 	 video_file_name_info * 
* @retval 	指向配置信息的一个数组, RECORD_APP作为索引
*/
const video_file_name_info * video_fs_get_file_name_cfg()
{
	return g_video_file_name_cfg;
}

