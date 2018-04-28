
/**
 * N1 设备实现用例。\n
 * 用例中通过调用 N1 设备模块（下简称模块）相关接口，\n
 * 实现局域网下 N1 设备与 N1 客户端（下简称客户端）交互的过程。\n
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <NkUtils/log.h>
#include <NkUtils/macro.h>
#include "n1_version.h"
#include "n1_device.h"
#include "script.h"


#include "basetype.h"
#include "common.h"
#include "anyka_config.h"
#include "isp_module.h"
#include "ak_n1_interface.h"
#include "anyka_interface.h"


#include "onvif.h"
#include "minirtsp.h"
#include "env_common.h"

#include "convert.h"


#define N1_AUDIO

/**
 * 字符串赋值。
 */
#define SET_TEXT(__buf, __text) do{ snprintf(__buf, sizeof(__buf), "%s", __text); } while(0)

/**
 * 无线网卡名称。
 */
#define WIFI_IF_NAME			"wlan0"

/**
 * 有线网卡名称。
 */
#define WIRED_IF_NAME			"eth0:1"



/**
 * 监听端口。
 */
#define LISTEN_PORT 		(80)
#define ONVIF_LISTEN_PORT	(8080)

#define USR_NAME_SIZE_MAX	(64)
#define USR_PASSWORD_SIZE_MAX	(64)



typedef struct 
{
	uint32_t	conn_id;
    uint32_t	audio_ts_ms;
	uint32_t	video_ts_ms;
	BYTE 		*buf;
}N1_USR_STRUCT;


typedef struct 
{
    NK_PByte	pData;
	NK_UInt32	size;
	bool		bFinish;
}N1_IMG;


#define N1_MD_FILE_PATH	"/etc/jffs2/md.conf"
#define N1_USR_FILE_PATH	"/etc/jffs2/usr.conf"


N1_CTRL *pn1_ctrl = NULL;

T_IMG_SET_VALUE g_img_val = {0};

T_VIDEO_SET_VALUE g_video_set = {0};
extern char* anyka_get_verion(void);

extern int WEBS_set_resource_dir(const char *resource_dir);

/**
* @brief   n1_video_move_queue_copy
*       将参数的数据另存为一份新的使用
* @date   2015/4
* @param: T_STM_STRUCT *pstream: 指向参数结构的指针
* @return   T_STM_STRUCT * 
* @retval   返回新的数据的指针
*/
T_STM_STRUCT * n1_stream_move_queue_copy(T_STM_STRUCT *pstream)
{
    T_STM_STRUCT *new_stream;

    new_stream = (T_STM_STRUCT *)malloc(sizeof(T_STM_STRUCT));
    if(new_stream == NULL)
    {
        return NULL;
    }
    memcpy(new_stream, pstream, sizeof(T_STM_STRUCT));

    new_stream->buf = (T_pDATA)malloc(pstream->size);
    if(new_stream->buf == NULL)
    {
        free(new_stream);
        return NULL;
    }
    memcpy(new_stream->buf, pstream->buf, pstream->size);
    return new_stream;
}


/**
* @brief   n1_get_vga_video_data
*       获取vga 数据的callback
* @date   2015/10
* @param: void *param: now no used; T_STM_STRUCT *pstream: 指向参数结构的指针
* @return   void
* @retval   
*/
void n1_get_vga_video_data(void *param, T_STM_STRUCT *pstream)
{
    T_STM_STRUCT *pvideo;
	int i = 0;

	for (i=0; i<CONNECT_NUM_MAX; i++)
	{
	    pthread_mutex_lock(&pn1_ctrl->conn[i].lock);
		if (pn1_ctrl->conn[i].bValid && !pn1_ctrl->conn[i].bMainStream && pn1_ctrl->conn[i].video_run_flag)
		{
		    pvideo = n1_stream_move_queue_copy(pstream);
		    if(pvideo)
		    {
		        if(0 == anyka_queue_push(pn1_ctrl->conn[i].video_queue, pvideo))
		        {
		            anyka_free_stream_ram(pvideo);
		            anyka_print("vga queue full, ch=%d\n", i);
		            usleep(10*1000);
		        }
		    }
		}
	    pthread_mutex_unlock(&pn1_ctrl->conn[i].lock);
	}
}


/**
* @brief   n1_get_720_video_data
*       获取720p 数据的callback
* @date   2015/4
* @param: void *param: now no used; T_STM_STRUCT *pstream: 指向参数结构的指针
* @return   void
* @retval   
*/
void n1_get_720_video_data(void *param, T_STM_STRUCT *pstream)
{
    T_STM_STRUCT *pvideo;
	int i = 0;

	for (i=0; i<CONNECT_NUM_MAX; i++)
	{
	   pthread_mutex_lock(&pn1_ctrl->conn[i].lock);
		if (pn1_ctrl->conn[i].bValid && pn1_ctrl->conn[i].bMainStream && pn1_ctrl->conn[i].video_run_flag)
		{
		    pvideo = n1_stream_move_queue_copy(pstream);
		    if(pvideo)
		    {
		        if(0 == anyka_queue_push(pn1_ctrl->conn[i].video_queue, pvideo))
		        {
		            anyka_free_stream_ram(pvideo);
		            usleep(10*1000);
		            anyka_print("720p queue full, ch=%d\n", i);
		        }
		    }
		}
	    pthread_mutex_unlock(&pn1_ctrl->conn[i].lock);
	}
}


/**
* @brief   n1_get_audio_data
*       获取audio数据的callback
* @date   2015/4
* @param: void *param: now no used; T_STM_STRUCT *pstream: 指向参数结构的指针
* @return   void
* @retval   
*/
void n1_get_audio_data(void *param, T_STM_STRUCT *pstream)
{
    T_STM_STRUCT *paudio;
	int i = 0;

	for (i=0; i<CONNECT_NUM_MAX; i++)
	{
		pthread_mutex_lock(&pn1_ctrl->conn[i].lock);
		if (pn1_ctrl->conn[i].bValid && pn1_ctrl->conn[i].audio_run_flag)
		{
		    paudio = n1_stream_move_queue_copy(pstream);
		    if(paudio)
		    {
		        if(0 == anyka_queue_push(pn1_ctrl->conn[i].audio_queue, paudio))
		        {
		            anyka_free_stream_ram(paudio);
		            usleep(10*1000);
		        }
		    }
		}
	    pthread_mutex_unlock(&pn1_ctrl->conn[i].lock);
	}
}


unsigned int n1_get_dhcp_status(void)
{
	char buf[10] = {0};
	
	int fd = open("/tmp/dhcp_status", O_RDONLY);
	if(fd < 0) {
		perror("open");
		return 0;
	}
	read(fd, buf, 1);

	close(fd);
	NK_Log()->debug("[%s:%d] dhcp: %d\n", __func__, __LINE__, atoi(buf));

	return atoi(buf);
}

T_BOOL n1_MD_Load(struct NK_MotionDetection* pmotion)
{
	T_BOOL ret = AK_FALSE;
	FILE *fd = NULL;
	int len = 0;
	
	if (NULL == pmotion)
	{
		anyka_print("n1_MD_Load param NULL!\n");
		return ret;
	}

	pthread_mutex_lock(&pn1_ctrl->mdFilelock);

	fd = fopen (N1_MD_FILE_PATH, "r+b");

	if (NULL == fd)
	{
		anyka_print("n1_MD_Load fd open failed!\n");
		pthread_mutex_unlock(&pn1_ctrl->mdFilelock);
		return ret;
	}

	len = fread(pmotion, 1, sizeof(struct NK_MotionDetection), fd);

	if (len == sizeof(struct NK_MotionDetection))
	{
		ret = NK_True;
	}
	else
	{
		anyka_print("n1_MD_Load read len err : %d, %d!\n", len, sizeof(struct NK_MotionDetection));
	}

	fclose(fd);

	pthread_mutex_unlock(&pn1_ctrl->mdFilelock);

	return ret;
}



T_BOOL n1_MD_Store(struct NK_MotionDetection* pmotion)
{
	T_BOOL ret = AK_FALSE;
	FILE *fd = NULL;
	int len = 0;
	
	if (NULL == pmotion)
	{
		anyka_print("n1_MD_Store param NULL!\n");
		return ret;
	}

	pthread_mutex_lock(&pn1_ctrl->mdFilelock);

	fd = fopen (N1_MD_FILE_PATH, "w+b");

	if (NULL == fd)
	{
		anyka_print("n1_MD_Store fd open failed!\n");
		pthread_mutex_unlock(&pn1_ctrl->mdFilelock);
		return ret;
	}

	len = fwrite(pmotion, 1, sizeof(struct NK_MotionDetection), fd);


	if (len == sizeof(struct NK_MotionDetection))
	{
		ret = NK_True;
	}
	else
	{
		anyka_print("n1_MD_Load write len err : %d, %d!\n", len, sizeof(struct NK_MotionDetection));
	}

	fclose(fd);

	pthread_mutex_unlock(&pn1_ctrl->mdFilelock);


	return ret;
}

void n1_usr_load(void)
{
	NK_Size user_count = 0;
	NK_Int i = 0;
	NK_Char username[USR_NAME_SIZE_MAX+1], password[USR_PASSWORD_SIZE_MAX+1];
	NK_Char *buf = NULL;
	FILE *fd = NULL;
	NK_Int size = 0;

	pthread_mutex_lock(&pn1_ctrl->usrFilelock);

	fd = fopen (N1_USR_FILE_PATH, "r+b");

	if (NULL == fd)
	{
		pthread_mutex_unlock(&pn1_ctrl->usrFilelock);
		anyka_print("n1 usr fd open failed!\n");
		NK_N1Device_AddUser("admin", "", 0);
	}
	else
	{
		fseek(fd, 0, SEEK_END);
		size = ftell(fd);
		user_count = size / (USR_NAME_SIZE_MAX + USR_PASSWORD_SIZE_MAX);

		anyka_print("n1 user_count = %d!\n", user_count);

		buf = (NK_Char *)malloc(size);

		if (NULL == buf)
		{
			fclose(fd);
			pthread_mutex_unlock(&pn1_ctrl->usrFilelock);
			anyka_print("n1 usr buf malloc failed!\n");
			NK_N1Device_AddUser("admin", "", 0);
		}
		else
		{
			fseek(fd, 0, SEEK_SET);
			fread(buf, 1, size, fd);
			fclose(fd);
			pthread_mutex_unlock(&pn1_ctrl->usrFilelock);

			for (i=0; i<user_count; i++)
			{
				memset(username, 0, USR_NAME_SIZE_MAX+1);
				memset(password, 0, USR_PASSWORD_SIZE_MAX+1);
				memcpy(username, buf + i * (USR_NAME_SIZE_MAX + USR_PASSWORD_SIZE_MAX), USR_NAME_SIZE_MAX);
				memcpy(password, buf + i * (USR_NAME_SIZE_MAX + USR_PASSWORD_SIZE_MAX) + USR_NAME_SIZE_MAX, USR_PASSWORD_SIZE_MAX);
				NK_N1Device_AddUser(username, password, 0);
			}
			
			free(buf);
		}
	}
}



/**
 * 判断当前是否连接目标 ESSID 的热点。
 */
static inline NK_Boolean
CHECK_WIFI_CONNECTION(NK_PChar essid)
{
	NK_Char result[1024 * 4];
	NK_SSize result_len = 0;

	/**
	 * 判断是否存在运行命令。
	 */
	result_len = SCRIPT2(result, sizeof(result), "pidof wpa_supplicant");
	if (result_len > 0)  {

		anyka_print("%s\r\n", result);
		/**
		 * 判断无线配置是否匹配的 ESSID。
		 */
		result_len = SCRIPT2(result, sizeof(result), "cat /tmp/wpa_supplicant.conf | grep \"%s\"", essid);
		if (result_len > 0) {
			NK_Log()->info("Test: Hot Spot( ESSID = %s ) Already Connected.", essid);
			return NK_True;
		}
	}

	return NK_False;
}


/**
 * 配置网卡 IP 地址。
 */
static inline NK_Void
SETUP_IPADDR(NK_PChar if_name, NK_PChar ipaddr, NK_PChar netmask, NK_PChar gateway, NK_PChar dns)
{
	SCRIPT("ifconfig %s %s netmask %s", WIFI_IF_NAME, ipaddr, netmask);
	SCRIPT("route del default > /dev/null");
	SCRIPT("route add default gateway %s %s", gateway, WIFI_IF_NAME);
	SCRIPT("ifconfig %s", WIFI_IF_NAME);
	SCRIPT("route -e | grep default | grep %s", WIFI_IF_NAME);

	if (NK_Nil != dns) {
		SCRIPT("echo \"nameserver %s\" > /etc/resolv.conf", dns);
	}
}


#define LIVE_IMG_FILE_PATH "/tmp/"

void n1_get_img(void *para, char *file_name)
{
	FILE *fID = NK_Nil;
	NK_SSize readn = 0;
	N1_IMG *img = (N1_IMG*)para;
	
	fID = fopen(file_name, "rb");
	if (!fID) {
		NK_Log()->info("Test: Open File (Path = %s) Failed.", file_name);
		return;
	}

	NK_Log()->info("Test: Snapshot  (img->pData = 0x%x).", img->pData);

	readn = fread(img->pData, 1, img->size, fID);
	fclose(fID);
	fID = NK_Nil;

	img->size = readn;
	img->bFinish = AK_TRUE;
}


/**
 * 现场抓图事件。
 *
 */
static NK_N1Ret
n1_onLiveSnapshot(NK_Int channel_id, NK_Size width, NK_Size height, NK_PByte pic, NK_Size *size)
{
	N1_IMG img = {0};
	int i = 5;	//timeout sec
	
	NK_Log()->info("Test: Snapshot (Channel = %d, Size = %dx%d, Stack = %d).", channel_id, width, height, *size);
	NK_Log()->info("Test: Snapshot  (pic = 0x%x).", pic);

	img.pData = pic;
	img.size = *size;
	img.bFinish = AK_FALSE;

	anyka_photo_start(1, 1, LIVE_IMG_FILE_PATH, n1_get_img, &img);

	
	while(i--) 
	{
		sleep(1);
		
		if(img.bFinish)
		{
			NK_Log()->info("Test: Snapshot OK.");
			break;
		}
	}

	*size = img.size;

	NK_Log()->info("Test: Snapshot Completed (Size = %d).", *size);
	return NK_N1_ERR_NONE;
}


/**
 * 流媒体直播相关事件。\n
 * 当客户端发现到设备时，由于两者网络配置本身存在差异而无法交互，\n
 * 客户端会通过配置设备方式使两者在局域网下满足交互条件。\n
 * 客户端配置时会触发以下事件。\n
 */
static NK_N1Ret
n1_onLiveConnected(NK_N1LiveSession *Session, NK_PVoid ctx)
{
	/**
	 *
	 * 主码流分辨率不建议大于 1920x1080 像素，帧率不建议大于 30 帧\n
	 * 辅码流分辨率不建议大于 704x480 像素，帧率不建议大于 30 帧，\n
	 * 否则使客户端处理器资源消耗过大影响体验。
	 *
	 */

	N1_USR_STRUCT *usr = NULL;
	int i = 0;

	if (CONNECT_NUM_MAX == pn1_ctrl->conn_cnt)
	{
		anyka_print("************************conn_cnt is %d! ******************\n", pn1_ctrl->conn_cnt);
		return NK_N1_ERR_DEVICE_BUSY;
	}
	
	usr = (N1_USR_STRUCT*)calloc(1, sizeof(N1_USR_STRUCT));

	if (NULL == usr)
	{
		anyka_print("usr malloc failed!\n");
		return NK_N1_ERR_UNDEF;
	}
	
	memset(usr, 0, sizeof(N1_USR_STRUCT));

	pthread_mutex_lock(&pn1_ctrl->lock);

	for (i=0; i<CONNECT_NUM_MAX; i++)
	{
	    pthread_mutex_lock(&pn1_ctrl->conn[i].lock);
	    
		if (AK_FALSE == pn1_ctrl->conn[i].bValid)
		{
			usr->conn_id = i;
			pn1_ctrl->conn[i].bValid = AK_TRUE;
			pn1_ctrl->conn_cnt++;
			pthread_mutex_unlock(&pn1_ctrl->conn[i].lock);
			break;
		}

		pthread_mutex_unlock(&pn1_ctrl->conn[i].lock);
	}

	pthread_mutex_unlock(&pn1_ctrl->lock);

	Session->Video.payload_type = NK_N1_DATA_PT_H264_NALUS;
	if (0 == Session->stream_id) {
		Session->Video.width = 1280;
		Session->Video.heigth = 720;
	} else {
		Session->Video.width = 640;
		Session->Video.heigth = 480;
	}
	Session->Audio.payload_type = NK_N1_DATA_PT_G711A;
	Session->Audio.sample_rate = 8000;
	Session->Audio.sample_bitwidth = 16;
	Session->Audio.stereo = NK_False;
	Session->user_session = usr;

	

	anyka_print("************************n1_onLiveConnected *******conn : %d,*****stream : %d******\n", usr->conn_id, Session->stream_id);


	pthread_mutex_lock(&pn1_ctrl->lock);
	pthread_mutex_lock(&pn1_ctrl->conn[i].lock);

	if (0 == Session->stream_id)
	{
		pn1_ctrl->conn[i].bMainStream = AK_TRUE;
		pn1_ctrl->conn[i].video_queue = anyka_queue_init(100);
		pn1_ctrl->conn[i].video_run_flag = 1;		
	}
	else if (1 == Session->stream_id)
	{
		pn1_ctrl->conn[i].bMainStream = AK_FALSE;
		pn1_ctrl->conn[i].video_queue = anyka_queue_init(110);
        pn1_ctrl->conn[i].video_run_flag = 1;
	}

	pn1_ctrl->conn[i].biFrameflag = AK_FALSE;

#ifdef N1_AUDIO

	pn1_ctrl->conn[i].audio_queue = anyka_queue_init(200);
	pn1_ctrl->conn[i].audio_run_flag = 1;

	anyka_print("conn_cnt = %d\n", pn1_ctrl->conn_cnt);
#endif
	

	pthread_mutex_unlock(&pn1_ctrl->conn[i].lock);
	pthread_mutex_unlock(&pn1_ctrl->lock);

	NK_Log()->info("Test: Live Media (Channel = %d,  Stream = %d,  Session = %08x) Connected.",
				Session->channel_id, Session->stream_id, Session->session_id);

	return NK_N1_ERR_NONE;
}


static NK_N1Ret
n1_onLiveDisconnected(NK_N1LiveSession *Session, NK_PVoid ctx)
{
	N1_USR_STRUCT *usr = Session->user_session;
	int i = 0;

	if (0 == pn1_ctrl->conn_cnt)
	{
		return NK_N1_ERR_UNDEF;
	}

	if (usr) 
	{
		i = usr->conn_id;
	}
	else
	{
        anyka_print("n1_onLiveDisconnected invalid\n");    
		return NK_N1_ERR_UNDEF;
	}

	
	anyka_print("************************n1_onLiveDisconnected *******conn : %d,*****stream : %d******\n", usr->conn_id, Session->stream_id);

	pthread_mutex_lock(&pn1_ctrl->lock);
	pthread_mutex_lock(&pn1_ctrl->conn[i].lock);

#ifdef N1_AUDIO
    if (NULL != pn1_ctrl->conn[i].audio_queue)
    {
		anyka_print("[%s:%d]#####  N1 stop audio %d #####\n", __func__, __LINE__, i);
	    pn1_ctrl->conn[i].audio_run_flag = 0;
	    anyka_queue_destroy(pn1_ctrl->conn[i].audio_queue, anyka_free_stream_ram);
	    pn1_ctrl->conn[i].audio_queue = NULL;
	}

		
#endif

    if (NULL != pn1_ctrl->conn[i].video_queue)
    {
        pn1_ctrl->conn[i].video_run_flag = 0;
        
        if (0 == Session->stream_id)
    	{
    	    anyka_print("[%s:%d]#####  N1 stop 720p %d #####\n", __func__, __LINE__, i);
    	}
    	else if (1 == Session->stream_id)
    	{
    		anyka_print("[%s:%d]#####  N1 stop  vga %d #####\n", __func__, __LINE__, i);
    	}

    	anyka_queue_destroy(pn1_ctrl->conn[i].video_queue, anyka_free_stream_ram);
        pn1_ctrl->conn[i].video_queue = NULL;

    	pn1_ctrl->conn_cnt--;
		pn1_ctrl->conn[i].bValid = AK_FALSE;
    }

	if (NULL != usr->buf)
	{
		free(usr->buf);
		usr->buf = NULL;
	}	

	free(Session->user_session);
	Session->user_session = NULL;
	
    pthread_mutex_unlock(&pn1_ctrl->conn[i].lock);
	pthread_mutex_unlock(&pn1_ctrl->lock);
		
	NK_Log()->info("Test: Live on Channel: %d Stream: %d Disconnected.",
			Session->channel_id, Session->stream_id, Session->session_id);

	return NK_N1_ERR_NONE;
}


static NK_SSize
n1_onLiveReadFrame(NK_N1LiveSession *Session, NK_PVoid ctx,
		NK_N1DataPayload *payload_type, NK_UInt32 *ts_ms, NK_PByte *data)
{
	/**
	 *
	 * 主码流单数据包总大小不建议大于 1M 字节，\n
	 * 辅码流单数据包总大小不建议大于 300k 字节，\n
	 * 否则客户端可能由于瞬时内存不足造成解码异常。
	 *
	 */

	T_STM_STRUCT *pstream;
	NK_SSize len = 0;

	N1_USR_STRUCT *usr = Session->user_session;
	int i = 0;

	if (usr) 
	{
		i = usr->conn_id;
	}
	else
	{
        anyka_print("n1_onLiveReadFrame invalid\n");    
		return -1;
	}
  
#ifdef N1_AUDIO
	if (usr->video_ts_ms < usr->audio_ts_ms)
	{
#endif
		if(pn1_ctrl->conn[i].video_run_flag)
	    {
	        if((pstream = (T_STM_STRUCT *)anyka_queue_pop(pn1_ctrl->conn[i].video_queue)) == NULL)
	        {
	            //anyka_print("pop fail\n");
	            return 0;
	        }

			usr->video_ts_ms = pstream->timestamp;
			len = pstream->size;
	    }
		else
		{
			anyka_print("run_flag 0\n");
			return 0;
		}

		if (!pn1_ctrl->conn[i].biFrameflag)
		{
			if (0 == pstream->iFrame)
			{
				anyka_free_stream_ram(pstream);
				return 0;
			}
			else
			{
				pn1_ctrl->conn[i].biFrameflag = AK_TRUE;
			}
		}

		*payload_type = NK_N1_DATA_PT_H264_NALUS;
		
#ifdef N1_AUDIO
	}
	else
	{
		if(pn1_ctrl->conn[i].audio_run_flag)
        {
            if((pstream = (T_STM_STRUCT *)anyka_queue_pop(pn1_ctrl->conn[i].audio_queue)) == NULL)
            {
                return 0;
            }

			usr->audio_ts_ms = pstream->timestamp;
			len = pstream->size;
        }
		else
		{
			anyka_print("run_flag 0\n");
			return 0;
		}

		*payload_type = NK_N1_DATA_PT_G711A;
		
	}
#endif

	

	if (0 == len) 
	{
		NK_Log()->info("len = 0!\n");
		anyka_free_stream_ram(pstream);
		return 0;
	}

	if (NULL != usr->buf)
	{
		free(usr->buf);
		usr->buf = NULL;
	}

	usr->buf = (BYTE*)malloc(len);
	
	if (NULL == usr->buf)
	{
		NK_Log()->info("usr->buf malloc failed!\n");
		anyka_free_stream_ram(pstream);
		return 0;
	}

    memcpy(usr->buf, pstream->buf, len);
	*data = usr->buf;
	
	/// 返回参数。
	*ts_ms = pstream->timestamp;

	anyka_free_stream_ram(pstream);


	return len;
}

static NK_N1Error
n1_onLiveAfterReadFrame(NK_N1LiveSession *Session, NK_PVoid ctx,
				NK_PByte *data_r, NK_Size size)
{
	//NK_Log()->debug("Test: Read Frame( Size = %u ) Finished.", size);
	return NK_N1_ERR_NONE;
}


static NK_N1Error
n1_onScanWiFiHotSpot(NK_PVoid ctx, NK_WiFiHotSpot *HotSpots, NK_Size *hotSpot_count)
{
	/**
	 * 下面以 RTL8188EUS 驱动为例。
	 */
	NK_SSize survey_info_size = 0;
	NK_Char survey_info[1024 * 8];
	NK_Int count = 0;

	/**
	 * 扫描附近的无线热点。
	 */
	SCRIPT("iwlist %s scanning > /tmp/iwlist.log", WIFI_IF_NAME);
//	SCRIPT("cat /proc/net/rtl8188eu/%s/survey_info", WIFI_IF_NAME);

	/**
	 * 读取 survey_info 文件信息。
	 */
	survey_info_size = SCRIPT2(survey_info, sizeof(survey_info), "cat /proc/net/rtl8188eu/%s/survey_info", WIFI_IF_NAME);
	if (survey_info_size > 0) {
		survey_info[survey_info_size] = '\0';
	} else {
		return NK_N1_ERR_DEVICE_NOT_SUPPORT;
	}

	/**
	 * 解析 survey_info 文件。
	 */
	do {
		NK_Char each_info[1024];
		NK_PChar line_begin = NK_Nil;
		NK_PChar line_end = NK_Nil;

		line_begin = survey_info;
		while (NK_Nil != (line_end = strchr(line_begin, '\n'))) {

			/**
			 * 检查有没有超出栈区长度。
			 * 这个很重要，超出栈区长度后仍然读写会产生潜在的问题。
			 */
			if (!(count <= *hotSpot_count)) {
				break;
			}

			/**
			 * 获取一行信息用于解析。
			 */
			strncpy(each_info, line_begin, line_end - line_begin);
			line_begin += strlen(each_info) + 1;

			if (NK_PREFIX_STRCMP(each_info, "index")) {
				/**
				 * 跳过标题。
				 */
				continue;
			} else {
				/**
				 * 解析单个热点信息。
				 */
				NK_WiFiHotSpot *const HotSpot = &HotSpots[count];
				NK_Int index = 0, ch = 0, dbm = 0, sdbm = 0, noise = 0, age = 0;
				NK_Char bssid[32], flag[16], essid[128];

				NK_BZERO(HotSpot, sizeof(HotSpot[0]));
				NK_BZERO(bssid, sizeof(bssid));
				NK_BZERO(essid, sizeof(essid));
				if (9 == sscanf(each_info, "%d  %s  %d  %d  %d  %d %d  %s  %s",
							&index, bssid, &ch, &dbm, &sdbm, &noise, &age, flag, essid)) {

					NK_STR_TRIM(bssid, HotSpot->bssid, sizeof(HotSpot->bssid));
					HotSpot->channel = ch;
					HotSpot->dBm = dbm;
					HotSpot->sdBm = sdbm;
					HotSpot->age = age;
					NK_STR_TRIM(essid, HotSpot->essid, sizeof(HotSpot->essid));

					++count;
				}
			}
		}
	} while (0);

	/**
	 * 扫描成功。
	 */
	*hotSpot_count = count;
	return NK_N1_ERR_NONE;
}

static NK_N1Error
n1_onConnectWiFiHotSpot(NK_PVoid ctx, NK_N1LanSetup *LanSetup)
{
	NK_Char conf[1024 * 4];
	NK_SSize len = 0;
	FILE *fID = NK_Nil;
	NK_Char ipaddr[64], netmask[64], gateway[64], dns[64];

	/**
	 * 为了提高无线配对成功率，无线重连时先检测当前连接的无线热点时是否与正在连接的相同，\n
	 * 如果相同则不许要重新连接。
	 */
	if (!CHECK_WIFI_CONNECTION(LanSetup->NetWiFi.ESSID.val)) {
		/**
		 * 连接无线热点。
		 */
		len = snprintf(conf, sizeof(conf),
				"ctrl_interface=/tmp/wpa_supplicant\n"
				"update_config=1\n"
				"network={\n"
				"    ssid=\"%s\"\n"
				"    psk=\"%s\"\n"
				"    scan_ssid=1\n"
				"}\n",
				LanSetup->NetWiFi.ESSID.val, LanSetup->NetWiFi.PSK.val);

		fID = fopen("/tmp/wpa_supplicant.conf", "wb");
		if (!fID) {
			return NK_N1_ERR_DEVICE_NOT_SUPPORT;
		}
		fwrite(conf, 1, len, fID);
		fclose(fID);

		SCRIPT("kill -9 `pidof wpa_supplicant`");
		SCRIPT("ifconfig %s down", WIFI_IF_NAME);
		usleep(200000);
		SCRIPT("ifconfig wlan0 up");
		usleep(200000);
		SCRIPT("wpa_supplicant -Dwext -i %s -c /tmp/wpa_supplicant.conf -B", WIFI_IF_NAME);

		NK_Log()->info("Test: Connect Wi-Fi NVR( ESSID = %s).", LanSetup->NetWiFi.ESSID.val);
	}

	/**
	 * 配置 IP 地址。
	 */
	if (LanSetup->NetWiFi.EnableDHCP.val) {

		SCRIPT("udhcpc -i %s", WIFI_IF_NAME);

	} else {

		NK_N1_PROP_IPV4_STR(&LanSetup->NetWiFi.IPAddress, ipaddr, sizeof(ipaddr));
		NK_N1_PROP_IPV4_STR(&LanSetup->NetWiFi.Netmask, netmask, sizeof(netmask));
		NK_N1_PROP_IPV4_STR(&LanSetup->NetWiFi.Gateway, gateway, sizeof(gateway));
		NK_N1_PROP_IPV4_STR(&LanSetup->NetWiFi.DomainNameServer, dns, sizeof(dns));

		/**
		 * 注意：
		 * 在 Wi-Fi 配对过程中，网关设置非常关键，用户实现时须要根据协议提供的网关地址去设置对应无线网卡上的网关地址。
		 * 如果设备同时希望可以独立工作于互联网，此处须要设置 DNS 地址。
		 */
		SETUP_IPADDR(WIFI_IF_NAME, ipaddr, netmask, gateway, dns);
	}

	/**
	 * 扫描成功。
	 */
	return NK_N1_ERR_NONE;
}


static NK_N1Error
n1_onUserChanged(NK_PVoid ctx)
{
	NK_TermTable Table;
	NK_Size user_count = 0;
	NK_Int i = 0;
	NK_Char username[USR_NAME_SIZE_MAX+1], password[USR_PASSWORD_SIZE_MAX+1];
	FILE *fd = NULL;

	pthread_mutex_lock(&pn1_ctrl->usrFilelock);

	fd = fopen (N1_USR_FILE_PATH, "w+b");

	if (NULL == fd)
	{
		anyka_print("n1 usr fd open failed!\n");
	}	

	user_count = NK_N1Device_CountUser();

	NK_CHECK_POINT();
	NK_TermTbl_BeginDraw(&Table, "N1 User Set", 64, 4);
	NK_TermTbl_PutKeyValue(&Table, NK_True, "Count", "%u", user_count);
	for (i = 0; i < user_count; ++i) 
	{
		memset(username, 0, USR_NAME_SIZE_MAX+1);
		memset(password, 0, USR_PASSWORD_SIZE_MAX+1);
				
		if (0 == NK_N1Device_IndexOfUser(i, username, password, NK_Nil)) 
		{
			NK_TermTbl_PutKeyValue(&Table, NK_False, username, "%s", password);

			if (NULL != fd)
			{
				fwrite(username, 1, USR_NAME_SIZE_MAX, fd);
				fwrite(password, 1, USR_PASSWORD_SIZE_MAX, fd);
			}
		}
	}
	NK_TermTbl_EndDraw(&Table);

	if (NULL != fd)
	{
		fclose(fd);
	}
	
	pthread_mutex_unlock(&pn1_ctrl->usrFilelock);

	return NK_N1_ERR_NONE;
}



/**
 * 设备局域网配置相关事件。\n
 *
 */
NK_N1Ret
n1_onLanSetup(NK_PVoid ctx, NK_Boolean is_set, NK_N1LanSetup *LanSetup)
{
	NK_Char ipaddr[32], netmask[32], gateway[32];
	NK_Int i = 0, ii = 0;


	/**
	 * 设置的时候打印设置信息。
	 */
	if (is_set) {
		NK_N1_LAN_SETUP_DUMP(LanSetup);
	}

	/**
	 * 参考配置选项。
	 * 根据各个平台的具体工作能力，配置各个参数的能力集。
	 * 客户端会根据能力集生成用户界面。
	 * 以下用静态变量的方式模拟设备保存配置的过程。
	 */

	Psystem_onvif_set_info onvif = anyka_get_sys_onvif_setting();

	NK_Log()->info("n1_onLanSetup is_set = %d, LanSetup->classify = %d\n", is_set, LanSetup->classify);

	switch (LanSetup->classify) {
		
		case NK_N1_LAN_SETUP_INFO: {

			if (is_set) {
				NK_Log()->error("Test: N1 Device Info is Read Only.");
			} else {
			
				NK_Log()->info("Test: Get Server Information.");

				//strcpy(LanSetup->Info.code, "N1TEST");
				//strcpy(LanSetup->Info.model, "N1 Developer");
				//strcpy(LanSetup->Info.cloud_id, "N10123456789");
				strcpy(LanSetup->Info.version, anyka_get_verion());
				
				/**
				 * 当前设备只有一个直播通道，每个直播通道由两个码流。
				 */
				LanSetup->Info.live_channels = 1;
				LanSetup->Info.LiveChannels[0].stream_channels = 2;
			}

			//NK_Log()->debug("Test: Get / Set on NK_N1LanSetup::Info.");
		}
		break;

		case NK_N1_LAN_SETUP_TIME:
		{
			/**
			 * 配置设备时间。
			 */
			struct tm setup_tm;
			if (is_set) {
				/**
				 * 配置设备时间参数。
				 */

				/// 转换成可阅读的时间格式。
				gmtime_r((time_t *)(&LanSetup->Time.utc), &setup_tm);

				NK_Log()->info("Test: Set Time %04d:%02d:%02d %02d:%02d:%02d GMT",
						setup_tm.tm_year + 1900, setup_tm.tm_mon + 1, setup_tm.tm_mday,
						setup_tm.tm_hour, setup_tm.tm_min, setup_tm.tm_sec);
				//settime(LanSetup->Time.utc);

				
				setenv("TZ", "GMT-08:00", 1);
				tzset();
				 
				struct timeval tv;
				tv.tv_sec = LanSetup->Time.utc;
				tv.tv_usec = 0;
				settimeofday(&tv, NULL);

			} else {
				/**
				 * 获取设备时间参数配置。
				 */
				/// TODO
				LanSetup->Time.utc = gettime();
				LanSetup->Time.gmt = NK_TZ_GMT_E0800;
				LanSetup->Time.dst = NK_False;
			}
			NK_Log()->debug("Test: Get / Set on NK_N1LanSetup::Time.");
		}
		break;
		

		/**
		 * @name 配置 IRCut 滤光片相关用例。
		 */
		case NK_N1_LAN_SETUP_IRCUT:
		{
			/**
			 * 模拟本地缓存。
			 */

			Pcamera_disp_setting pcamera_info = anyka_get_camera_info();
	
			static NK_N1LanSetup _LanSetupIRCutFilter, *LanSetupIRCutFilter;
			
			if (!LanSetupIRCutFilter) 
			{
				LanSetupIRCutFilter = &_LanSetupIRCutFilter;
				LanSetupIRCutFilter->classify = NK_N1_LAN_SETUP_IRCUT;
				LanSetupIRCutFilter->IRCutFilter.DayToNightFilterTime.min = 2;
				LanSetupIRCutFilter->IRCutFilter.DayToNightFilterTime.max = 8;
				LanSetupIRCutFilter->IRCutFilter.NightToDayFilterTime.min = 2;
				LanSetupIRCutFilter->IRCutFilter.NightToDayFilterTime.max = 8;
				NK_N1_PROP_ADD_ENUM(&LanSetupIRCutFilter->IRCutFilter.Mode, N1IRCutFilterMode, NK_N1_IRCUT_MODE_AUTO);
				NK_N1_PROP_ADD_ENUM(&LanSetupIRCutFilter->IRCutFilter.Mode, N1IRCutFilterMode, NK_N1_IRCUT_MODE_DAYLIGHT);
				NK_N1_PROP_ADD_ENUM(&LanSetupIRCutFilter->IRCutFilter.Mode, N1IRCutFilterMode, NK_N1_IRCUT_MODE_NIGHT);

				LanSetupIRCutFilter->IRCutFilter.DayToNightFilterTime.val = 4;
				LanSetupIRCutFilter->IRCutFilter.NightToDayFilterTime.val = 4;
				LanSetupIRCutFilter->IRCutFilter.Mode.val = pcamera_info->ircut_mode;
			}

			if (is_set) 
			{
				memcpy(LanSetupIRCutFilter, LanSetup, sizeof(NK_N1LanSetup));

				if (NK_N1_IRCUT_MODE_DAYLIGHT == LanSetup->IRCutFilter.Mode.val)
				{
					system("echo 1 > /tmp/ircut_test");
				}
				else if (NK_N1_IRCUT_MODE_NIGHT == LanSetup->IRCutFilter.Mode.val)
				{
					system("echo 0 > /tmp/ircut_test");
				}
				else
				{
					system("rm /tmp/ircut_test -f");
				}

				Pcamera_disp_setting pcamera_info = anyka_get_camera_info();

				pcamera_info->ircut_mode = LanSetup->IRCutFilter.Mode.val;
				anyka_set_camera_info(pcamera_info);
			} 
			else 
			{
				memcpy(LanSetup, LanSetupIRCutFilter, sizeof(NK_N1LanSetup));
			}

			break;
		}

		/**
		 * @name 配置视频编码相关用例。
		 */
		case NK_N1_LAN_SETUP_VENC:
		{
			struct NK_VideoEncoder *VideoEncoder = &LanSetup->VideoEncoder;

			static NK_N1LanSetup _LanSetupVideoEncoder[2];
			static NK_N1LanSetup *LanSetupVideoEncoder[2];

			for (i = 0; i < sizeof(LanSetupVideoEncoder) / sizeof(LanSetupVideoEncoder[0]); ++i) {
				if (!LanSetupVideoEncoder[i]) {
					LanSetupVideoEncoder[i] = &_LanSetupVideoEncoder[i];
					LanSetupVideoEncoder[i]->classify = NK_N1_LAN_SETUP_VENC;
					LanSetupVideoEncoder[i]->channel_id = 0;
					LanSetupVideoEncoder[i]->stream_id = i;

					LanSetupVideoEncoder[i]->VideoEncoder.Codec.val = NK_N1_VENC_CODEC_H264;
					LanSetupVideoEncoder[i]->VideoEncoder.Codec.def = NK_N1_VENC_CODEC_H264;
					NK_N1_PROP_ADD_ENUM(&LanSetupVideoEncoder[i]->VideoEncoder.Codec, N1VideoEncCodec, NK_N1_VENC_CODEC_H264);

					LanSetupVideoEncoder[i]->VideoEncoder.H264.KeyFrameInterval.val = 60;
					LanSetupVideoEncoder[i]->VideoEncoder.H264.KeyFrameInterval.def = 60;
					LanSetupVideoEncoder[i]->VideoEncoder.H264.KeyFrameInterval.min = 30;
					LanSetupVideoEncoder[i]->VideoEncoder.H264.KeyFrameInterval.max = 60;

					LanSetupVideoEncoder[i]->VideoEncoder.H264.BitRateCtrlMode.val = g_video_set.videomode;
					LanSetupVideoEncoder[i]->VideoEncoder.H264.BitRateCtrlMode.def = NK_N1_BR_CTRL_MODE_CBR;
					NK_N1_PROP_ADD_ENUM(&LanSetupVideoEncoder[i]->VideoEncoder.H264.BitRateCtrlMode, N1BitRateCtrlMode, NK_N1_BR_CTRL_MODE_CBR);
					NK_N1_PROP_ADD_ENUM(&LanSetupVideoEncoder[i]->VideoEncoder.H264.BitRateCtrlMode, N1BitRateCtrlMode, NK_N1_BR_CTRL_MODE_VBR);

					if (0 == i) {
						LanSetupVideoEncoder[i]->VideoEncoder.H264.Resolution.val = NK_N1_IMG_SZ_1280X720;
						LanSetupVideoEncoder[i]->VideoEncoder.H264.Resolution.def = NK_N1_IMG_SZ_1280X720;
						NK_N1_PROP_ADD_ENUM(&LanSetupVideoEncoder[i]->VideoEncoder.H264.Resolution, N1ImageSize, NK_N1_IMG_SZ_1280X720);
						LanSetupVideoEncoder[i]->VideoEncoder.H264.BitRate.val = g_video_set.mainbps;
						LanSetupVideoEncoder[i]->VideoEncoder.H264.BitRate.def = 2048;
						LanSetupVideoEncoder[i]->VideoEncoder.H264.BitRate.min = 256;
						LanSetupVideoEncoder[i]->VideoEncoder.H264.BitRate.max = 4096;

						LanSetupVideoEncoder[i]->VideoEncoder.H264.FrameRate.val = onvif->fps1;
						LanSetupVideoEncoder[i]->VideoEncoder.H264.FrameRate.def = onvif->fps1;
						LanSetupVideoEncoder[i]->VideoEncoder.H264.FrameRate.min = onvif->fps1-1;
						LanSetupVideoEncoder[i]->VideoEncoder.H264.FrameRate.max = onvif->fps1;
					} else {
						LanSetupVideoEncoder[i]->VideoEncoder.H264.Resolution.val = NK_N1_IMG_SZ_640X480;
						LanSetupVideoEncoder[i]->VideoEncoder.H264.Resolution.def = NK_N1_IMG_SZ_640X480;
						NK_N1_PROP_ADD_ENUM(&LanSetupVideoEncoder[i]->VideoEncoder.H264.Resolution, N1ImageSize, NK_N1_IMG_SZ_640X480);
						LanSetupVideoEncoder[i]->VideoEncoder.H264.BitRate.val = g_video_set.subbps;
						LanSetupVideoEncoder[i]->VideoEncoder.H264.BitRate.def = 768;
						LanSetupVideoEncoder[i]->VideoEncoder.H264.BitRate.min = 128;
						LanSetupVideoEncoder[i]->VideoEncoder.H264.BitRate.max = 1024;

						LanSetupVideoEncoder[i]->VideoEncoder.H264.FrameRate.val = onvif->fps2;
						LanSetupVideoEncoder[i]->VideoEncoder.H264.FrameRate.def = onvif->fps2;
						LanSetupVideoEncoder[i]->VideoEncoder.H264.FrameRate.min = onvif->fps2-1;
						LanSetupVideoEncoder[i]->VideoEncoder.H264.FrameRate.max = onvif->fps2;
					}
				}
			}

			/**
			 * 配置视频编码参数。
			 */
			if (is_set) {

				/**
				 * 配置视频编码参数。
				 */
				NK_Log()->info("Test: Set Video Encoder Attributes.");

				i = LanSetup->stream_id;

				//LanSetupVideoEncoder[i]->VideoEncoder.H264.FrameRate.val = VideoEncoder->H264.FrameRate.val;
				LanSetupVideoEncoder[i]->VideoEncoder.H264.BitRate.val = VideoEncoder->H264.BitRate.val;
				
				if (0 == i)
				{
					//video_set_encode_fps(FRAMES_ENCODE_RECORD, LanSetupVideoEncoder[i]->VideoEncoder.H264.FrameRate.val);
    				video_set_encode_bps(FRAMES_ENCODE_RECORD, LanSetupVideoEncoder[i]->VideoEncoder.H264.BitRate.val);
					g_video_set.mainbps = VideoEncoder->H264.BitRate.val;
				}
				else
				{
					//video_set_encode_fps(FRAMES_ENCODE_VGA_NET, LanSetupVideoEncoder[i]->VideoEncoder.H264.FrameRate.val);
    				video_set_encode_bps(FRAMES_ENCODE_VGA_NET, LanSetupVideoEncoder[i]->VideoEncoder.H264.BitRate.val);
					g_video_set.subbps = VideoEncoder->H264.BitRate.val;
				}

				T_BOOL bNeedRestart = AK_FALSE;

				if (g_video_set.videomode != VideoEncoder->H264.BitRateCtrlMode.val)
				{
					g_video_set.videomode = VideoEncoder->H264.BitRateCtrlMode.val;
					bNeedRestart = AK_TRUE;
				}

				ak_n1_set_video_setting(&g_video_set);

				if (bNeedRestart)
				{
					NK_Log()->info("video mode changed, restart anyka_ipc!");
					exit(0);
				}

			} else {

				/**
				 * 获取视频编码参数配置。
				 */
				 
				NK_Log()->debug("Test: Get Video Encoder Attributes.");
				LanSetupVideoEncoder[0]->VideoEncoder.H264.BitRate.val = g_video_set.mainbps;
				LanSetupVideoEncoder[1]->VideoEncoder.H264.BitRate.val = g_video_set.subbps;
				LanSetupVideoEncoder[0]->VideoEncoder.H264.BitRateCtrlMode.val = g_video_set.videomode;
				LanSetupVideoEncoder[1]->VideoEncoder.H264.BitRateCtrlMode.val = g_video_set.videomode;
				memcpy(LanSetup, LanSetupVideoEncoder[LanSetup->stream_id],
						sizeof(LanSetup[0]));
			}
		}
		break;

		/**
		 * @name 配置视频图像相关用例。
		 */
		case NK_N1_LAN_SETUP_VIMG:
		{
			struct 	NK_VideoImage *VideoImage = &LanSetup->VideoImage;

			static NK_N1LanSetup _LanSetupVideoImage, *LanSetupVideoImage = NK_Nil;
			
			if (!LanSetupVideoImage) {
				LanSetupVideoImage = &_LanSetupVideoImage;
				LanSetupVideoImage->classify = NK_N1_LAN_SETUP_VIMG;
				LanSetupVideoImage->channel_id = 0;
				LanSetupVideoImage->stream_id = 0;

				Pcamera_disp_setting pcamera_info = anyka_get_camera_info();

				LanSetupVideoImage->VideoImage.Title.Show.val = pcamera_info->osd_switch;
				LanSetupVideoImage->VideoImage.Title.TextEncoding.val = NK_TEXT_ENC_UTF8;
				LanSetupVideoImage->VideoImage.Title.Text.max_len = OSD_TITLE_LEN_MAX;
	
				NK_N1_PROP_STR_SET(&LanSetupVideoImage->VideoImage.Title.Text, pcamera_info->osd_name);

				LanSetupVideoImage->VideoImage.PowerLineFrequenceMode.val = 60;
				LanSetupVideoImage->VideoImage.PowerLineFrequenceMode.def = 60;
				NK_N1_PROP_ADD_OPT(&LanSetupVideoImage->VideoImage.PowerLineFrequenceMode, 50);
				NK_N1_PROP_ADD_OPT(&LanSetupVideoImage->VideoImage.PowerLineFrequenceMode, 60);
				NK_N1_PROP_ADD_OPT(&LanSetupVideoImage->VideoImage.PowerLineFrequenceMode, 100);
				NK_N1_PROP_ADD_OPT(&LanSetupVideoImage->VideoImage.PowerLineFrequenceMode, 120);

				LanSetupVideoImage->VideoImage.CaptureResolution.val = NK_N1_IMG_SZ_1280X720;
				LanSetupVideoImage->VideoImage.CaptureResolution.def = NK_N1_IMG_SZ_1280X720;
				NK_N1_PROP_ADD_OPT(&LanSetupVideoImage->VideoImage.CaptureResolution, NK_N1_IMG_SZ_1280X720);

				LanSetupVideoImage->VideoImage.CaptureFrameRate.val = 30;
				LanSetupVideoImage->VideoImage.CaptureFrameRate.def = 30;
				NK_N1_PROP_ADD_OPT(&LanSetupVideoImage->VideoImage.CaptureFrameRate, 25);
				NK_N1_PROP_ADD_OPT(&LanSetupVideoImage->VideoImage.CaptureFrameRate, 30);

				LanSetupVideoImage->VideoImage.HueLevel.min = LanSetupVideoImage->VideoImage.BrightnessLevel.min \
						= LanSetupVideoImage->VideoImage.SharpnessLevel.min \
						= LanSetupVideoImage->VideoImage.ContrastLevel.min \
						= LanSetupVideoImage->VideoImage.SaturationLevel.min = 0;
				LanSetupVideoImage->VideoImage.HueLevel.max = LanSetupVideoImage->VideoImage.BrightnessLevel.max \
						= LanSetupVideoImage->VideoImage.SharpnessLevel.max \
						= LanSetupVideoImage->VideoImage.ContrastLevel.max \
						= LanSetupVideoImage->VideoImage.SaturationLevel.max = 100;

				LanSetupVideoImage->VideoImage.HueLevel.def = IMG_EFFECT_DEF_VAL;
				LanSetupVideoImage->VideoImage.BrightnessLevel.def = IMG_EFFECT_DEF_VAL;
				LanSetupVideoImage->VideoImage.ContrastLevel.def = IMG_EFFECT_DEF_VAL;
				LanSetupVideoImage->VideoImage.SaturationLevel.def = IMG_EFFECT_DEF_VAL;
				LanSetupVideoImage->VideoImage.SharpnessLevel.def = IMG_EFFECT_DEF_VAL;

				LanSetupVideoImage->VideoImage.HueLevel.val = g_img_val.hue;
				LanSetupVideoImage->VideoImage.BrightnessLevel.val = g_img_val.brightness;
				LanSetupVideoImage->VideoImage.ContrastLevel.val = g_img_val.contrast;
				LanSetupVideoImage->VideoImage.SaturationLevel.val = g_img_val.saturation;
				LanSetupVideoImage->VideoImage.SharpnessLevel.val = g_img_val.sharp;

				LanSetupVideoImage->VideoImage.Flip.val = g_img_val.flip;
				LanSetupVideoImage->VideoImage.Mirror.val = g_img_val.mirror;

				/**
				 * 运动侦测。
				 */
				LanSetupVideoImage->VideoImage.MotionDetection.Enabled.val = NK_True;
				LanSetupVideoImage->VideoImage.MotionDetection.SensitivityLevel.val = 90;
				LanSetupVideoImage->VideoImage.MotionDetection.SensitivityLevel.min = 0;
				LanSetupVideoImage->VideoImage.MotionDetection.SensitivityLevel.max = 100;
				LanSetupVideoImage->VideoImage.MotionDetection.Mask.width = 32;
				LanSetupVideoImage->VideoImage.MotionDetection.Mask.height = 24;
				for (i = 0; i < LanSetupVideoImage->VideoImage.MotionDetection.Mask.height; ++i) {
					for (ii = 0; ii < LanSetupVideoImage->VideoImage.MotionDetection.Mask.width; ++ii) {
						LanSetupVideoImage->VideoImage.MotionDetection.Mask.matrix[i][ii] = NK_True;
					}
				}

				struct NK_MotionDetection motion;
				
				if (n1_MD_Load(&motion))
				{
					memcpy(&LanSetupVideoImage->VideoImage.MotionDetection, &motion, sizeof(motion));
				}
			}

			if (is_set) {

				/**
				 * 配置图像参数。
				 */

				NK_Log()->info("Test: Set Video Image Attributes.");
				NK_N1_LAN_SETUP_DUMP(LanSetup);

				memcpy(LanSetupVideoImage, LanSetup, sizeof(NK_N1LanSetup));

				Isp_Effect_Set(EFFECT_HUE, VideoImage->HueLevel.val - VideoImage->HueLevel.def);
				Isp_Effect_Set(EFFECT_BRIGHTNESS, VideoImage->BrightnessLevel.val - VideoImage->BrightnessLevel.def);
				Isp_Effect_Set(EFFECT_SATURATION, VideoImage->SaturationLevel.val - VideoImage->SaturationLevel.def);
				Isp_Effect_Set(EFFECT_CONTRAST, VideoImage->ContrastLevel.val - VideoImage->ContrastLevel.def);


				g_img_val.hue = VideoImage->HueLevel.val;
				g_img_val.brightness = VideoImage->BrightnessLevel.val;
				g_img_val.contrast = VideoImage->ContrastLevel.val;
				g_img_val.saturation = VideoImage->SaturationLevel.val;
				g_img_val.sharp = VideoImage->SharpnessLevel.val;

				if (VideoImage->Title.Show.val)
				{
					anyka_print("set Title.TextEncoding.val = %d\n", VideoImage->Title.TextEncoding.val);
					
					if (NK_TEXT_ENC_GB2312 == VideoImage->Title.TextEncoding.val
						|| NK_TEXT_ENC_GBK == VideoImage->Title.TextEncoding.val)
					{
						char utf8[OSD_TITLE_LEN_MAX+1] = {0};

						g2u(VideoImage->Title.Text.val, strlen(VideoImage->Title.Text.val), utf8, OSD_TITLE_LEN_MAX);
						ak_n1_update_osd_font(utf8);
						NK_N1_PROP_STR_SET(&LanSetupVideoImage->VideoImage.Title.Text, utf8);
						LanSetupVideoImage->VideoImage.Title.TextEncoding.val = NK_TEXT_ENC_UTF8;
					}
					else
					{
						ak_n1_update_osd_font(VideoImage->Title.Text.val);
					}
				}
				else
				{
					ak_n1_update_osd_off();
				}
 
				ak_n1_save_video_image_setting(&g_img_val);

				n1_MD_Store(&VideoImage->MotionDetection);

				int flipstate = S_Upright;

				g_img_val.flip = VideoImage->Flip.val;
				g_img_val.mirror = VideoImage->Mirror.val;

				if (g_img_val.flip && g_img_val.mirror)
				{
					flipstate = S_FLIP;
				}
				else if (g_img_val.flip)
				{
					flipstate = S_Vertical;
				}
				else if (g_img_val.mirror)
				{
					flipstate = S_Horizontal;
				}
				
				SetCamerFlip(flipstate);

			} else {

				/**
				 * 获取图像参数配置。
				 */
				NK_Log()->info("Test: Get Video Image Attributes.");

				LanSetupVideoImage->VideoImage.HueLevel.val = g_img_val.hue;
				LanSetupVideoImage->VideoImage.BrightnessLevel.val = g_img_val.brightness;
				LanSetupVideoImage->VideoImage.ContrastLevel.val = g_img_val.contrast;
				LanSetupVideoImage->VideoImage.SaturationLevel.val = g_img_val.saturation;
				LanSetupVideoImage->VideoImage.SharpnessLevel.val = g_img_val.sharp;

				T_BOOL bNeedgbk = AK_FALSE;

				anyka_print("get Title.TextEncoding.val = %d\n", VideoImage->Title.TextEncoding.val);
				
				if (NK_TEXT_ENC_GB2312 == VideoImage->Title.TextEncoding.val
					|| NK_TEXT_ENC_GBK == VideoImage->Title.TextEncoding.val)
				{
					bNeedgbk = AK_TRUE;
				}
				
				memcpy(LanSetup, LanSetupVideoImage, sizeof(NK_N1LanSetup));

				if (bNeedgbk)
				{
					char gbk[OSD_TITLE_LEN_MAX+1] = {0};

					u2g(VideoImage->Title.Text.val, strlen(VideoImage->Title.Text.val), gbk, OSD_TITLE_LEN_MAX);
					strcpy(VideoImage->Title.Text.val, gbk);

					VideoImage->Title.TextEncoding.val = NK_TEXT_ENC_GB2312;
				}

			}

			NK_Log()->debug("Test: Get / Set on NK_N1LanSetup::VideoImage.");
		}
		break;

		/**
		 * @name 云台控制配置相关用例。
		 */
		case NK_N1_LAN_SETUP_PTZ:
		{
			NK_N1_LAN_SETUP_DUMP(LanSetup);

			NK_Log()->debug("Test: Get / Set on NK_N1LanSetup::PanTiltZoom.");
		}
		break;

		/**
		 * @name 有线网络配置。
		 */
		case NK_N1_LAN_SETUP_NET_WIRED:
		case NK_N1_LAN_SETUP_NET_WIRED_NVR: {
			/**
			 * 模拟本地缓存。
			 */
			static NK_N1LanSetup _LanSetupWired, *LanSetupWired = NK_Nil;

			if (!LanSetupWired) {
				/**
				 * 初始化本地配置。
				 */

				int i = 0;
				char ip[64]={0};
				char mac[64]={0};
				
				//ifconfig eth0
			    do_syscmd("ifconfig eth0 | grep 'inet addr' | awk '{print $2}'", ip);
			    while(ip[i])
			    {
			        if (ip[i]=='\n' || ip[i]=='\r' || ip[i]==' ')
			            ip[i] = '\0';
			        i++;
			    }

				NK_Log()->debug("_get_interface: ip  = %s.\n", &ip[5]);

				do_syscmd("ifconfig eth0 | grep 'HWaddr' | awk '{print $5}'", mac);
			    while(mac[i])
			    {
			        if (mac[i]=='\n' || mac[i]=='\r' || mac[i]==' ')
			            mac[i] = '\0';
			        i++;
			    }

				NK_Log()->debug("_get_interface: mac  = %s.\n", mac);
			    
			    Psystem_net_set_info sys_net_info = anyka_get_sys_net_setting();

				LanSetupWired = &_LanSetupWired;
				LanSetupWired->classify = NK_N1_LAN_SETUP_NET_WIRED;
				LanSetupWired->channel_id = 0;
				LanSetupWired->stream_id = 0;
				LanSetupWired->NetWired.EnableDHCP.val = n1_get_dhcp_status();

				NVP_IP_INIT_FROM_STRING(LanSetupWired->NetWired.IPAddress.val, &ip[5]);
				NVP_IP_INIT_FROM_STRING(LanSetupWired->NetWired.Netmask.val, sys_net_info->netmask);
				NVP_IP_INIT_FROM_STRING(LanSetupWired->NetWired.Gateway.val, sys_net_info->gateway);
				NVP_IP_INIT_FROM_STRING(LanSetupWired->NetWired.DomainNameServer.val, sys_net_info->gateway);
			    NVP_MAC_INIT_FROM_STRING(LanSetupWired->NetWired.HwAddr.val, mac);

			}

			if (is_set) {
				/**
				 * 设置 IP 地址。
				 */
				memcpy(LanSetupWired, LanSetup, sizeof(NK_N1LanSetup));

				/**
				 * 以下以 Linux 系统中使用 eth0:1 网卡模拟。
				 */
				NK_N1_PROP_IPV4_STR(&LanSetupWired->NetWired.IPAddress, ipaddr, sizeof(ipaddr));
				NK_N1_PROP_IPV4_STR(&LanSetupWired->NetWired.Netmask, netmask, sizeof(netmask));
				NK_N1_PROP_IPV4_STR(&LanSetupWired->NetWired.Gateway, gateway, sizeof(gateway));

				//SETUP_IPADDR(WIRED_IF_NAME, ipaddr, netmask, gateway, NK_Nil);

				
				char cmd[128] = {0};

				sprintf(cmd, "ifconfig eth0 %s netmask %s", ipaddr, netmask);
				system(cmd);

				sprintf(cmd, "route add default gw %s", gateway);
				system(cmd);

				Psystem_net_set_info net_info = anyka_get_sys_net_setting();
    
				net_info->dhcp = LanSetupWired->NetWired.EnableDHCP.val;
				strcpy(net_info->ipaddr, ipaddr);
				strcpy(net_info->netmask, netmask);
				strcpy(net_info->gateway, gateway);

				anyka_set_sys_net_setting(net_info);

				if (0 == net_info->dhcp)
				{
					system("echo 0 > /tmp/dhcp_status");
				}
				else
				{
					system("echo 1 > /tmp/dhcp_status");
				}

				NK_Log()->debug("Test: Set Wired Network. Restart anyka_ipc!");
				
				exit(0);

			} else {
				memcpy(LanSetup, LanSetupWired, sizeof(NK_N1LanSetup));				
				NK_Log()->debug("Test: Get Wired Network.");
			}

		}
		break;


		/**
		 * @name 无线 NVR 网络配对。
		 */
		case NK_N1_LAN_SETUP_NET_WIFI:
		case NK_N1_LAN_SETUP_NET_WIFI_NVR: {
		

			/**
			 * 模拟本地缓存。
			 */
			static NK_N1LanSetup _LanSetupWiFi, *LanSetupWiFi = NK_Nil;

			if (!LanSetupWiFi) {
				/**
				 * 初始化本地配置。
				 */
				LanSetupWiFi = &_LanSetupWiFi;
				LanSetupWiFi->classify = NK_N1_LAN_SETUP_NET_WIFI_NVR;
				LanSetupWiFi->channel_id = 0;
				LanSetupWiFi->stream_id = 0;
				LanSetupWiFi->NetWiFi.EnableDHCP.val = NK_False;
				NK_N1_PROP_IPV4_SET(&LanSetupWiFi->NetWiFi.IPAddress, 172, 168, 40, 110);
				NK_N1_PROP_IPV4_SET(&LanSetupWiFi->NetWiFi.Netmask, 255, 255, 0, 0);
				NK_N1_PROP_IPV4_SET(&LanSetupWiFi->NetWiFi.Gateway, 172, 168, 1, 1);
				NK_N1_PROP_IPV4_SET(&LanSetupWiFi->NetWiFi.DomainNameServer, 172, 168, 1, 1);
				NK_N1_PROP_HWADDR_SET(&LanSetupWiFi->NetWiFi.HwAddr, 0x01, 0xaa, 0xbb, 0xcc, 0xdd, 0xee);
			}

			if (is_set) {
				/**
				 * 设置 IP 地址。
				 */
				memcpy(LanSetupWiFi, LanSetup, sizeof(NK_N1LanSetup));

				/**
				 * 调用 onCnnectWiFiHotSpot 事件尝试连接 NVR。
				 */
				n1_onConnectWiFiHotSpot(ctx, LanSetupWiFi);
				NK_Log()->debug("Test: Set WiFi Network.");

			} else {
				memcpy(LanSetup, LanSetupWiFi, sizeof(NK_N1LanSetup));
				NK_Log()->debug("Test: Get WiFi Network.");
			}
		}
		break;
		

		default:
		break;
	}

	if (NK_False) {
		/**
		 * 如果设置命令中存在错误参数返回以下数值。
		 */
		return NK_N1_ERR_INVALID_PARAM;
	}
	

	/**
	 * 配置成功。
	 */
	return NK_N1_ERR_NONE;
}


/**
 * 日志擦写回调。
 */
static NK_Void
n1_onFlushLog(NK_PByte bytes, NK_Size len)
{
	/**
	 * 注意：此回调接口内不能调用 NK_Log() 相关接口。
	 */

	#if 0
	
	FILE *fID = NK_Nil;
	fID = fopen("/tmp/n1.log", "ab");
	if (!fID) {
		anyka_print("    Open Log Failed.\r\n");
		return;
	}
	if (len != fwrite(bytes, 1, len, fID)) {
		anyka_print("    Write Log Failed.\r\n");
	}
	fclose(fID);
	fID = NK_Nil;
	#endif
}


int onvif_wsdd_event_hook(char *type, char *xaddr, char *scopes ,int event, void *customCtx)
{
	if (event == WSDD_EVENT_PROBE) {
		anyka_print("got wsdd probe from:%s type:%s\n", xaddr, type);
		return 1;  // probe response
		//return 0; // no probe response
	} else {
		anyka_print("wsdd hook %s, %s event:%d\n\t\tscope:%s\n", type ? type : "", xaddr, event, scopes ? scopes : "");
		return 0;
	}
}

void n1_check_movement(void *user, uint8 notice)
{
    static uint32 check_time = 0;
    uint32 cur_time = time(0);
    NK_N1Notification Notif;
    
    if (cur_time - check_time > 10)
    {
        anyka_print("[%s] pass the moving notify!\n", __func__);   
        check_time = cur_time;
        Notif.type = NK_N1_NOTF_MOTION_DETECTION;
        NK_N1Device_Notify(&Notif);
    }
    
}


void n1_start_movedetection()
{
    uint16 i, ratios=100, Sensitivity[65];
    T_MOTION_DETECTOR_DIMENSION detection_pos;
    Psystem_alarm_set_info alarm = anyka_sys_alarm_info();	//get system alamr info
    struct NK_MotionDetection motion;
    
    n1_MD_Load(&motion);
    anyka_print("[%s]enable=%d, SensitivityLevel=%d\n", __func__, motion.Enabled.val, motion.SensitivityLevel.val);               

    if (motion.Enabled.val == 0)
    {
        anyka_print("[%s:%d] motion detection disabled!!\n", __func__, __LINE__);               
        return;
    }
    
    ratios = alarm->motion_detection_2;

    Sensitivity[0] = ratios;
    for(i = 0; i < 64; i++)
    {                
        Sensitivity[i+1] = ratios;
    }

    
    
	//分割图像为motion_size_x * motion_size_y 块
    detection_pos.m_uHoriNum = alarm->motion_size_x;
    detection_pos.m_uVeriNum = alarm->motion_size_y;
	/** start video move check func **/
	if(video_start_move_check(VIDEO_HEIGHT_VGA, VIDEO_WIDTH_VGA, &detection_pos, Sensitivity,
			   	alarm->alarm_interval_time, n1_check_movement, NULL, NULL) == 1)
    {
        anyka_print("[%s:%d] start success!!\n", __func__, __LINE__);       
    }
}

void n1_init(void)
{
	NK_N1Device N1Device;
	T_IMG_SET_VALUE *pImgset = NULL;

	pn1_ctrl = (N1_CTRL*)malloc(sizeof(N1_CTRL));
    if(NULL == pn1_ctrl)
    {
        return ;
    }
    memset(pn1_ctrl, 0, sizeof(N1_CTRL));

	pthread_mutex_init(&pn1_ctrl->lock, NULL);
	pthread_mutex_init(&pn1_ctrl->mdFilelock, NULL);
	pthread_mutex_init(&pn1_ctrl->usrFilelock, NULL);

	for (int i=0; i<CONNECT_NUM_MAX; i++)
	{
		pthread_mutex_init(&pn1_ctrl->conn[i].lock, NULL);
		pthread_mutex_init(&pn1_ctrl->conn[i].streamlock, NULL);
	}

	pImgset = ak_n1_get_video_image_setting();

	if (NULL != pImgset)
	{
		g_img_val.hue = pImgset->hue;
		g_img_val.brightness = pImgset->brightness;
		g_img_val.saturation = pImgset->saturation;
		g_img_val.contrast = pImgset->contrast;
		g_img_val.sharp = pImgset->sharp;
		g_img_val.flip = pImgset->flip;
		g_img_val.mirror = pImgset->mirror;
	}
	else
	{
		g_img_val.hue = IMG_EFFECT_DEF_VAL;
		g_img_val.brightness = IMG_EFFECT_DEF_VAL;
		g_img_val.saturation = IMG_EFFECT_DEF_VAL;
		g_img_val.contrast = IMG_EFFECT_DEF_VAL;
		g_img_val.sharp = IMG_EFFECT_DEF_VAL;
		g_img_val.flip = 0;
		g_img_val.mirror = 0;
	}

	ak_n1_get_video_setting(&g_video_set);

	Isp_Effect_Set(EFFECT_HUE, g_img_val.hue - IMG_EFFECT_DEF_VAL);
	Isp_Effect_Set(EFFECT_BRIGHTNESS, g_img_val.brightness - IMG_EFFECT_DEF_VAL);
	Isp_Effect_Set(EFFECT_SATURATION, g_img_val.saturation - IMG_EFFECT_DEF_VAL);
	Isp_Effect_Set(EFFECT_CONTRAST, g_img_val.contrast - IMG_EFFECT_DEF_VAL);

	int flipstate = S_Upright;

	if (g_img_val.flip && g_img_val.mirror)
	{
		flipstate = S_FLIP;
	}
	else if (g_img_val.flip)
	{
		flipstate = S_Vertical;
	}
	else if (g_img_val.mirror)
	{
		flipstate = S_Horizontal;
	}
	
	SetCamerFlip(flipstate);

	Pcamera_disp_setting pcamera_info = anyka_get_camera_info();

	if (NK_N1_IRCUT_MODE_DAYLIGHT == pcamera_info->ircut_mode)
	{
		system("echo 1 > /tmp/ircut_test");
	}
	else if (NK_N1_IRCUT_MODE_NIGHT == pcamera_info->ircut_mode)
	{
		system("echo 0 > /tmp/ircut_test");
	}


	rtsp_server_run();


	setenv("DEF_ETHER", "eth0", AK_TRUE);
	setenv("ONVIF_PORT", "8080", AK_TRUE);

	
	anyka_print("hello onvif, run as server, listen port : %d ...\n", LISTEN_PORT);
	ONVIF_SERVER_init(ONVIF_DEV_NVT, "IPC");
	//ONVIF_SERVER_start(ONVIF_LISTEN_PORT);
	ONVIF_search_daemon_start(onvif_wsdd_event_hook, NULL);
	
	NK_UInt32 ver_maj = 0, ver_min = 0, ver_rev = 0, ver_num = 0;

	/**
	 * 配置随机数种子。
	 * 建议用户在外部调用此接口，以确保库内部调用 rand 产生随机数重复性降低。
	 */
	//srand(time(NULL));

	usleep(10000);

	struct timeval tv;
	gettimeofday(&tv, NULL);

	srand(tv.tv_sec * tv.tv_usec);

	/**
	 * 通过版本号判断一下二进制库与头文件是否匹配。
	 * 避免由于数据结构定义差异产生潜在的隐患。
	 */
	NK_N1Device_Version(&ver_maj, &ver_min, &ver_rev, &ver_num);
	if (NK_N1_VER_MAJ != ver_maj
			|| NK_N1_VER_MIN != ver_min
			|| NK_N1_VER_REV != ver_rev
			|| NK_N1_VER_NUM != ver_num) {
		anyka_print("Unmatched Library( %u.%u.%u-%u ) and C-Headers( %u.%u.%u-%u ) !!!\r\n",
				ver_maj, ver_min, ver_rev, ver_num,
				NK_N1_VER_MAJ, NK_N1_VER_MIN, NK_N1_VER_REV, NK_N1_VER_NUM);
		exit(1);
	}

	/**
	 * 配置日志终端输出等级。
	 */
	NK_Log()->setTerminalLevel(NK_LOG_ALL);
	NK_Log()->setLogLevel(NK_LOG_ALL);
	NK_Log()->onFlush = n1_onFlushLog;


	/**
	 * 清空数据结构，可以有效避免各种内存错误。
	 */
	NK_BZERO(&N1Device, sizeof(N1Device));

	/**
	 * 设置设备的设备号，此设备号用于在局域网中作为设备的唯一标识。\n
	 * 因此每一个设备必须不一样。
	 */
	
	NK_BZERO(N1Device.device_id, sizeof(N1Device.device_id));
	//	snprintf(N1Device.device_id, sizeof(N1Device.device_id), "N101234567890123456789");
	

	/**
	 * 协议使用端口。
	 * 模块通过使用此端口进行 TCP/IP 数据请求监听，
	 * 因此在设计上应该避免此端口与模块以外的 TCP/IP 端口冲突。
	 */
	N1Device.port = LISTEN_PORT;
	N1Device.user_ctx = NULL;

	
	N1Device.EventSet.onLiveSnapshot = n1_onLiveSnapshot;
	N1Device.EventSet.onLiveConnected = n1_onLiveConnected;
	N1Device.EventSet.onLiveDisconnected = n1_onLiveDisconnected;
	N1Device.EventSet.onLiveReadFrame = n1_onLiveReadFrame;
	N1Device.EventSet.onLiveAfterReadFrame = n1_onLiveAfterReadFrame;
	N1Device.EventSet.onLanSetup  = n1_onLanSetup;
	N1Device.EventSet.onScanWiFiHotSpot = n1_onScanWiFiHotSpot;
	N1Device.EventSet.onConnectWiFiHotSpot = n1_onConnectWiFiHotSpot;
	N1Device.EventSet.onUserChanged = n1_onUserChanged;

	/**
	 * 初始化 N1 设备环境。
	 */

#if 1
	if (0 != NK_N1Device_Init(&N1Device)) 
	{
		anyka_print("NK_N1Device_Init failed!\n");
		//exit(EXIT_FAILURE);
	}
	else
	{
		WEBS_set_resource_dir("/usr/share/www");

		n1_usr_load();
	}

	

	


	Psystem_onvif_set_info onvif = anyka_get_sys_onvif_setting();
	
	anyka_print("video_add 0.264\n");
	video_add(n1_get_720_video_data, "n1_0.264", FRAMES_ENCODE_RECORD, g_video_set.mainbps);
	video_set_encode_fps(FRAMES_ENCODE_RECORD, onvif->fps1);
	video_set_encode_bps(FRAMES_ENCODE_RECORD, g_video_set.mainbps);

	
	anyka_print("video_add 1.264\n");
	video_add(n1_get_vga_video_data, "n1_1.264", FRAMES_ENCODE_VGA_NET, g_video_set.subbps);
	video_set_encode_fps(FRAMES_ENCODE_VGA_NET, onvif->fps2);
	video_set_encode_bps(FRAMES_ENCODE_VGA_NET, g_video_set.subbps);

	
	anyka_print("audio add G711A\n");
	audio_add(SYS_AUDIO_ENCODE_G711, n1_get_audio_data, "n1_G711A");
#endif

    //start movement detection
    n1_start_movedetection();
}


