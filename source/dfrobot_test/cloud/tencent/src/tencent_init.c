/*
 * @filename tencent_init.c
 * @brief: implement QQCloud interface callback
 */
#include "common.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "pthread.h"
#include "TXDeviceSDK.h"
#include "TXOTA.h"
#include "TXIPCAM.h"
#include "ak_tencent.h"
#include "tencent_init.h"
#include "voice_tips.h"
#include <sys/socket.h> 
#include <netinet/in.h>  
#include <arpa/inet.h>   

#define SYS_OFFLINE			"/usr/share/anyka_camera_off_line.mp3"
#define SYS_ONLINE			"/usr/share/anyka_camera_online.mp3"
#define UPDATE_TIMEOUT 	10*60	//download 10 minutes, update 5 minutes
#define UPDATE_DOWNLOAD_PATH "/tmp/update_pkg.tar" //download path, change this name you must change ak_tencent.c

#if 1
#define TENCENT_CONFIG_FILE "/etc/jffs2/tencent.conf"

typedef struct _tencent_config_info
{
    int product_id;
    char *product_secret;
    char *license;
    char guid[17];
}tencent_config_info;
tencent_config_info tencent_config;

typedef struct _global_info
{
	char dev_name[50];
	unsigned short firmware_update;
}global_info;

global_info tx_global_info;

/**
 * NAME         tencent_get_config
 * @BRIEF		得到腾讯的配置文件，目前配置文件保存的四个信息，详情请见结构体tencent_config_info
 * @PARAM       NONE	
                  
 * @RETURN	
 * @RETVAL	
 */

void tencent_get_config()
{
    int item_len, i;
    char *file_buf, *info_ptr, item_buf[200];
    FILE *config_id;

    config_id = fopen(TENCENT_CONFIG_FILE, "r");

    if(config_id == NULL)
    {
        anyka_print("it can't open the config file!:%s\n", TENCENT_CONFIG_FILE);
        return ;
    }
    file_buf = (char *)malloc(4096);
    fread(file_buf, 1, 4096, config_id);
    info_ptr = file_buf;
    item_len = (int)info_ptr[1] * 256 + info_ptr[0];
    info_ptr += 2;
    if(item_len == 16)
    {
        memcpy(tencent_config.guid, info_ptr, item_len);
        tencent_config.guid[item_len] = 0;

        anyka_print("sn:%s\n", tencent_config.guid);
        info_ptr += item_len;
        item_len = (int)info_ptr[1] * 256 + info_ptr[0];
        info_ptr += 2;
        memcpy(item_buf, info_ptr, item_len);
        item_buf[item_len] = 0;
        tencent_config.product_id = atoi(item_buf);
        anyka_print("pid:%d\n", tencent_config.product_id);
        info_ptr += item_len;
        item_len = (int)info_ptr[1] * 256 + info_ptr[0];
        info_ptr += 2;
        tencent_config.product_secret = (char *)malloc(item_len + 1);
        memcpy(tencent_config.product_secret, info_ptr, item_len);
        tencent_config.product_secret[item_len] = 0;
        anyka_print("pub_key:%s\n", tencent_config.product_secret);
        info_ptr += item_len;
        item_len = (int)info_ptr[1] * 256 + info_ptr[0];
        tencent_config.license = (char *)malloc(item_len*2 + 1);
        info_ptr += 2;
        memcpy(item_buf, info_ptr, item_len);
        for(i = 0; i < item_len; i++)
        {
            sprintf(tencent_config.license + i * 2, "%02x", item_buf[i]);
        }
        tencent_config.license[i * 2] = 0;
        anyka_print("license:%s\n", tencent_config.license);
    }
    
    fclose(config_id);
    free(file_buf);
}
#endif

void log_func(int level, const char* module, int line, const char* message)
{
    system_user_info *set_info = anyka_get_system_user_info();

    if(set_info->debuglog)
    {
	    anyka_print("%s\t%d\t%s\n", module, line, message);	
    }
}


void OnReortDataPoint(int err_code, unsigned int cookie, const char* api_name, 
	int api_len, tx_data_point data_points[], int data_points_count)
{
    anyka_print("ret[%d] api[%s] api_len[%d]\n", err_code, api_name, api_len);
}


void OnReceiveDataPoint(unsigned long long from_client, tx_data_point * data_points, int data_points_count)
{
    anyka_print("data point api:data_points_count %u from %u\n",  data_points_count,(unsigned int)from_client);
    int i=0;
    while(i<data_points_count)
    {
        anyka_print("\tdp%d.id:%u\n", i, data_points[i].id);
        anyka_print("\tdp%d.value:%s\n", i, data_points[i].value);
        anyka_print("\tdp%d.seq:%u\n", i, data_points[i].seq);
        anyka_print("\tdp%d.ret:%u\n", i, data_points[i].ret_code);

        ++i;
    }
}

void send_report_msg()
{
#if 0
//    tx_test(); return;

	tx_api_data_point data_points[10];

    memset(data_points, 0, sizeof(data_points));
	data_points[0].property_id = 10003;
	data_points[0].point_val = "{\"msg_time\":1410419831,\"file_key\":\"test file key\"}";
    data_points[0].val_length = strlen(data_points[0].point_val);
	data_points[0].val_type = "json";
    data_points[0].type_length = strlen(data_points[0].val_type);
    int i =0;
    for(i=0; i<data_points[0].val_length; ++i)
    {
        anyka_print("%c", data_points[0].point_val[i]);
    }
    anyka_print("\n\tlen:%d, point_val:[%s]\n", data_points[0].val_length, data_points[0].point_val);

    data_points[1].property_id = 10001;
    data_points[1].point_val = "{\"msg_time\":1410419831,\"title\":\"小Q机器人\",\"guidewords\":\"点击查看图像\",\"appear_time\":1410419800,\"cover_key\":\"cover_key test1\",\"digest\":\"家里有异常情况1\",\"media_key\":\"124badfsdf1agsgsdf\",\"dskey\":\"dskey111\"}";
    data_points[1].val_length = strlen(data_points[1].point_val);
    data_points[1].val_type = "json";
    data_points[1].type_length = strlen(data_points[1].val_type);

    data_points[2].property_id = 10005;
    data_points[2].point_val = "{\"msg_time\":1410419831,\"title\":\"小Q机器人\",\"appear_time\":1410419831,\"cover_key\":\"这是 cover key\",\"digest\":\"这是digest,小伙计 点击查看图像\",\"guidewords2\":\"这是guidewords\",\"media_key\":\"uuid-xxx2x-xx-x2xx\",\"to_tinyid\":144115197715841001}";
    data_points[2].val_length = strlen(data_points[1].point_val);
    data_points[2].val_type = "json";
    data_points[2].type_length = strlen(data_points[1].val_type);

    data_points[3].property_id = 10002;
    data_points[3].point_val = "{\"msg_time\":1410419831,\"title\":\"小Q机器人2\",\"appear_time\":1410419831,\"cover_key\":\"这是 cover key\",\"digest\":\"33点击查看图像\",\"guidewords\":\"这是guidewords33\",\"media_key\":\"uuid-xxx2x-33-x2xx\",\"to_tinyid\":144115197715841222}";
    data_points[3].val_length = strlen(data_points[1].point_val);
    data_points[3].val_type = "json";
    data_points[3].type_length = strlen(data_points[1].val_type);
	//tx_send_report_msg2("report_datapoint", data_points, 1); return;
    tx_report_data_point(0, "report_datapoint", strlen("report_datapoint"), data_points, 1, OnReortDataPoint);
//    tx_send_cc_data_point(144115197715841041, "report_datapoint", strlen("report_datapoint"), data_points, 1, 0, 0);
#endif
}
/**
 * NAME         on_login_complete
 * @BRIEF		当设备登录成功后的回调函数	
 * @PARAM       errcode   错误代码 
                  
  @RETURN	
 * @RETVAL	
 */

void on_login_complete(int errcode)
{
    anyka_print("on_login_complete | code[%d]\n", errcode);

   // send_report_msg();
}


/**
 * NAME         on_online_status
 * @BRIEF	当设备的在线状态发生变化时，腾讯SDK会调用这个函数来通知我们	
 * @PARAM        old   之前的状态
 		new 	当前的状态 
                  
  @RETURN	
 * @RETVAL	
 */
int qq_online_status_info = 0;
void on_online_status(int old, int new)
{
    static int start_av_service = 0;
    anyka_print("on_online_status: old[%d] new[%d]\n", old, new);

    //第一次上线成功后，启动视频服务
    if(11 == new)
    {
        anyka_print("[%s,%d]we is online\n", __FUNCTION__, __LINE__);
     //   voice_tips_add_music(SYS_ONLINE);
	    qq_online_status_info = 1;
        if(!start_av_service)
        {
    		tx_av_callback avcallback = {0};
    		avcallback.on_start_camera = qq_start_video;
    		avcallback.on_stop_camera  = qq_stop_video;
    		avcallback.on_set_bitrate  = qq_set_video;
    		avcallback.on_start_mic    = qq_start_mic;
    		avcallback.on_stop_mic     = qq_stop_mic;
            avcallback.on_recv_audiodata = ak_qq_get_audio_data;
    		int ret = tx_start_av_service(&avcallback);
    		if (err_null != ret)
    		{
    			anyka_print("tx_start_av_service failed [%d]\n", ret);
    		}
    		else
    		{
    			anyka_print("tx_start_av_service successed\n");
    		}

			if(anyka_check_first_run()) {
				voice_tips_add_music(SYS_ONLINE);
			}
    		start_av_service = true;
            anyka_print("[%s,%d]we will init av service\n", __func__, __LINE__);
            //第一次在线时，将获取时间信息，并同步到系统
		    time_t server_time = tx_get_server_time() + 8 * 3600; 
		    char cmd[100];
		    struct tm *dest_time; 
			dest_time = localtime(&server_time);
			sprintf(cmd, "date \"%d-%d-%d %d:%d:%d\"", dest_time->tm_year + 1900, dest_time->tm_mon + 1, 
					dest_time->tm_mday, dest_time->tm_hour, dest_time->tm_min, dest_time->tm_sec);
			anyka_print("[%s:%d]set system time :%s\n", __func__, __LINE__, cmd);
			system(cmd);
        }

		if(tx_global_info.firmware_update == 1) {
			/*ota update ack result */
			tx_ack_ota_result(0, "update success!!!"); //test, result always ok
			anyka_print("tx_ack_ota_result successed\n");
			ak_qq_change_firmware_update_status(0);
		}
    }
    else
    {
		qq_online_status_info = 0;
        anyka_print("[%s,%d]we is offline\n", __func__, __LINE__);
       // voice_tips_add_music(SYS_OFFLINE);
    }
}

// 绑定列表变化通知
void on_binder_list_change(int error_code, tx_binder_info * pBinderList, int nCount)
{
	anyka_print("on_binder_list_change, %d binder: \n", nCount);
	int i = 0;
	for (i = 0; i < nCount; ++i )
	{
		anyka_print("binder uin[%llu], nick_name[%s]\n", pBinderList[i].uin, pBinderList[i].nick_name);
	}
}

#if 0
void on_send_ccmsg(int err_code, unsigned long long to_client, const tx_msg * msg)
{
    anyka_print("ret[%d] to[%llu] content[%s]\n", err_code, to_client, msg->msg_content);
}

void on_send_ccmsg_ex(int err_code, const tx_ccmsg_inst_info * to_client, const tx_msg * msg)
{
    anyka_print("ret[%d] to[%llu] content[%s]\n", err_code, to_client->target_id, msg->msg_content);
}

void on_receive_ccmsg(const tx_ccmsg_inst_info * from_client, const tx_msg * msg)
{
    if(!from_client || !msg) return;

    anyka_print("from[%llu] content[%s]\n", from_client->target_id, msg->msg_content);

    tx_msg newMsg = {0};
    char msgBuf[1024] = {0};
    char tmpStr[1024] = {0};
    memcpy(tmpStr, msg->msg_content, msg->msg_length);
    snprintf(msgBuf, 1024, "来自设备的应答：%s", tmpStr);
    newMsg.msg_content = msgBuf;
    newMsg.msg_length = strlen(msgBuf);

    if(msg->msg_content && !strncmp("system", msg->msg_content,strlen("system")))
    {
    	//anyka_print("系统消息测试：%d\n",ipcamera_send_alarm_msg(NULL,NULL, NULL, NULL) );
    }


    //tx_send_ccmsg(from_client->target_id, &newMsg, on_send_ccmsg);

//    tx_send_ccmsg_ex(from_client, &newMsg, on_send_ccmsg_ex);

    return;
}
#endif
void ontransferprogress(unsigned long long transfer_cookie, unsigned long long transfer_progress, unsigned long long max_transfer_progress)
{
    anyka_print("========> on file progress %f%%\n", transfer_progress * 100.0 / max_transfer_progress);
}

    // 传输结果
void ontransfercomplete(unsigned long long transfer_cookie, int err_code, tx_file_transfer_info* tran_info)
{
    anyka_print("==========================ontransfercomplete==============================\n");
    anyka_print("errcode %d, bussiness_name [%s], file path [%s]\n", err_code, tran_info->bussiness_name, tran_info->file_path);
    anyka_print("==========================================================================\n");
    if(err_code == 0)
    {
        if(strcmp(tran_info->bussiness_name, BUSINESS_NAME_AUDIO_MSG) == 0)
        {
            audio_play_start(tran_info->file_path, 1);
        }
        else
        {
            remove(tran_info->file_path);
        }
    }
}

    // 收到C2C transfer通知
void onrecvfile(unsigned long long transfer_cookie, const tx_ccmsg_inst_info * inst_info, const tx_file_transfer_info * tran_info)
{
}

void on_shutdown(int errcode)
{
    anyka_print("on_shutdown\n");

}

/**
 * NAME         on_set_definition
 * @BRIEF	码率调整的回调.	
 * @PARAM        definition	期望的码率
                  
  @RETURN	
 * @RETVAL	
 */
int on_set_definition(int definition ,char*  cur_definition,int cur_definition_len)
{
	anyka_print("on_set_definition:%d-------------------------------------------------------------\n",definition);
	strcpy(cur_definition,"hello");
	anyka_print("%s!!\n",cur_definition);
	ak_qq_set_video_para(definition);
	return  1;
}


bool readBufferFromFile(char *pPath, unsigned char *pBuffer, int nInSize, int *pSizeUsed)
{
	if (!pPath || !pBuffer)
	{
		anyka_print("[%s,%d]the param is error\n ", __FUNCTION__, __LINE__);
		return false;
	}

	int uLen = 0;
	FILE * file = fopen(pPath, "rb");
	if (!file)
	{
        anyka_print("[%s,%d]it fail to open the file\n ", __FUNCTION__, __LINE__);
	    return false;
	}

	do
	{
	    fseek(file, 0L, SEEK_END);
	    uLen = ftell(file);
	    fseek(file, 0L, SEEK_SET);

	    if (0 == uLen || uLen > nInSize)
	    {
	    	anyka_print("invalide file or buffer size is too small...\n");
	        break;
	    }

	    *pSizeUsed = fread(pBuffer, 1, uLen, file);

	    fclose(file);
	    return true;

	}while(0);

    fclose(file);
	return false;
}

/**
 * NAME         tencent_wait_init_sdk
 * @BRIEF	检查网络是否是正常的，腾讯SDK必须在网络是通的情况下初始化，否则会出问题。
 * @PARAM        
                  
  @RETURN	
 * @RETVAL	
 */
void tencent_wait_init_sdk(void)
{
    int ping_size;
    char *find, *ping_buf = (char *)malloc(512);
    FILE *fp;
    while(1)
    {
        fp = popen("ifconfig wlan0 || ifconfig eth0", "r");
        if(fp == NULL)
        {
            anyka_print("[%s,%d]it fails to popen!\n", __func__, __LINE__);
            continue ;
        }
        ping_size = fread(ping_buf, 1, 511,  fp);
        pclose(fp);
        ping_buf[ping_size] = 0;
        find = strstr(ping_buf, "inet addr");
        if(find)
        {
            break;               
        }
        sleep(2);
    }
    free(ping_buf);
}

int OnNewPkgCome(unsigned long long from, unsigned long long pkg_size, 
		const char * title, const char *desc, unsigned int target_version)
{
	anyka_print("[%s] pkg size: %llu\ttarget_version: %u\n", __func__, pkg_size, target_version);

	return 0; //start download packet
}

void OnDownloadProgress(unsigned long long download_size, unsigned long long total_size)
{
	anyka_print("[%s] download_size: %llu\ttotal_size: %llu\n", __func__, download_size, total_size);
}

void OnDownloadComplete(int ret_code)
{
	if(ret_code == 0) {
		anyka_print("[%s] download success\n", __func__);
	} else {
		anyka_print("[%s] download failed: %d\n",__func__, ret_code);
	}
}

void qq_ack_app()
{
    char ip[32]={0};
    unsigned int port=0;
	FILE * file = fopen("/tmp/smartlink", "r");

	if (file)
	{
        fscanf(file, "%s\n%d\n", ip, &port);
        if (port)
        {
            anyka_print("tx_ack_app():ip=%s, port=%d\n", ip, port);
            tx_ack_app(inet_addr(ip), port);   
        }

        fclose(file);
        remove("/tmp/smartlink");        
	}
}

static int adjustDays= 0;//convenient for test
static int time_zone = 8*3600; //beijing timezone
static void on_fetch_history_video(unsigned int last_time, int max_count, int *count, tx_history_video_range * range_list)
{
	int i,j;
	Precord_replay_info  cur,head;
	int clip_amount=0;
	
	anyka_print("on_fetch_history_video is vorked!last_time=%d,max_count=%d\n",last_time,max_count);

	head = cur = video_fs_list_get_record(0, last_time+time_zone, 0, RECORD_APP_PLAN);
	//先进行链表遍历，确定链表个数
	for(i = 0;; i++)
	{
		if (NULL ==cur)
		{
			break;	
		}

		cur = cur->next;
	}
	clip_amount = i;
	cur = head;
	
	for(j = 0; j < clip_amount; j++)
	{
		if (NULL == cur)
		{
			break;	
		}
		if (j < clip_amount - max_count) // 跳过前面的clip
		{
			cur = cur->next;
			continue;
		}

		if (clip_amount > max_count)
			i = j - (clip_amount - max_count);
		else
			i = j;
		
		range_list[i].start_time = cur->recrod_start_time + cur->play_start - time_zone + adjustDays;
		range_list[i].end_time = range_list[i].start_time + cur->play_time  ;
		
		anyka_print("i=%d, start_time=%d,end_time=%d\n"
				,i, range_list[i].start_time, range_list[i].end_time);
		cur = cur->next;
		
	}
	*count = i + 1;
	anyka_print("clip_total=%d\n", *count);
	
	/** release resource **/
	cur = head;
	while(cur) {
		head = head->next;
		free(cur);
		cur = head;
	}
	
}
unsigned long long g_base_time;
static void on_play_history_video(unsigned int play_time, unsigned long long base_time)
{
	anyka_print("on_play_history_video is vorked!,play_time=%d\n",play_time);
	g_base_time = base_time;
	
	if (play_time !=0)
	  	qq_start_video_replay(play_time  +time_zone -adjustDays);
	else
	  	qq_start_video_replay(0);//for switch to real video
		


}
void* thread_func_initdevice(void * arg)
{
    unsigned int main_ver, sub_ver, build_no, version = (unsigned int)arg;
	int tx_run_mode = 0; //nomal run mode
	tx_device_info info = {0};

	ak_qq_start_voicelink();
    //必须先初始化网络
    tencent_wait_init_sdk();
    ak_qq_stop_voicelink();
    //得到配置文件
    tencent_get_config();
	//get global info
	if(ak_qq_get_global_setting(tx_global_info.dev_name, &tx_global_info.firmware_update) < 0) {
		anyka_print("TencentSDK init failed\n");    
		return NULL;
	}

    //把guid保存在anyka_cfg.ini的osd_name字段，以便工程易用宝可以查看
    Pcamera_disp_setting camera_info;
    camera_info = anyka_get_camera_info();	/* get camera info */
    if (strncmp(camera_info->osd_name, tencent_config.guid, 16) != 0)
    {
        strncpy(camera_info->osd_name, tencent_config.guid, 16);
        anyka_set_camera_info(camera_info);	/* set camera info */
    }
    
    tx_get_sdk_version(&main_ver, &sub_ver, &build_no);
    anyka_print("TencentSDK version : %u.%u.%u\n", main_ver, sub_ver, build_no);    
    
	info.device_name 		   = tx_global_info.dev_name;
    info.product_version       = version; 
    info.os_platform           = "linux3.4.35";
    info.product_id            = tencent_config.product_id;
    info.device_license        = tencent_config.license;
    info.device_serial_number  = tencent_config.guid;
    info.server_pub_key        = tencent_config.product_secret; //change product_secret to server_pub_key
    info.network_type          = network_type_wifi;
    info.run_mode	           = tx_run_mode;
    
    tx_device_notify notify = {0};
    notify.on_login_complete       	= on_login_complete;
    notify.on_online_status        	= on_online_status;
    notify.on_binder_list_change 	= on_binder_list_change;

    tx_ipcamera_notify ipcamera_notify = {0};
    ipcamera_notify.on_control_rotate = ak_qq_control_rotate;
    ipcamera_notify.on_set_definition = on_set_definition;

    tx_init_path init_path = {0};
    init_path.system_path 			= "/etc/jffs2";
    init_path.system_path_capicity  = 1024*50;
    init_path.app_path 				= "/tmp";
    init_path.app_path_capicity  	= 512000;
    init_path.temp_path 			= "/tmp";
    init_path.temp_path_capicity  	= 102400;

    tx_set_log_func(log_func);

	int ret = tx_init_device(&info, &notify, &init_path);
	if (err_null != ret)
	{
		anyka_print("tx_init_device failed [errcode=0x%x]\n", ret);
	    return NULL;
	}

	anyka_print("tx_init_device success\n");
	tx_ipcamera_set_callback(&ipcamera_notify);
    qq_ack_app();

//    tx_msg_notify msgNotify = {0};
//    msgNotify.on_receive_ccmsg = on_receive_ccmsg;
//    tx_init_msg(&msgNotify);

    tx_data_point_notify openPlatformNotify = {0};
    openPlatformNotify.on_receive_data_point = OnReceiveDataPoint;
    tx_init_data_point(&openPlatformNotify);

    tx_file_transfer_notify fileTransferNotify = {0};
    fileTransferNotify.on_transfer_complete = ontransfercomplete;
    fileTransferNotify.on_transfer_progress = ontransferprogress;
    fileTransferNotify.on_file_in_come 		= onrecvfile;

    tx_init_file_transfer(fileTransferNotify, "/tmp/ramdisk/");

	/* init ota */
	tx_ota_notify ota_notify = {0};
	ota_notify.on_new_pkg_come 		= OnNewPkgCome; 
	ota_notify.on_download_progress = OnDownloadProgress;  
	ota_notify.on_download_complete = OnDownloadComplete; 
	ota_notify.on_update_confirm 	= ak_qq_OnUpdateConfirm;  
	
 	tx_init_ota(&ota_notify, UPDATE_TIMEOUT, UPDATE_DOWNLOAD_PATH);

	//init replay function
    tx_history_video_notify  his_notify;
    
	his_notify.on_fetch_history_video = on_fetch_history_video;
	his_notify.on_play_history_video  = on_play_history_video;
	
    tx_init_history_video_notify(&his_notify);
    
	return NULL;
}

bool initdevice(unsigned int version)
{
	int err;
	pthread_t ntid;

	err = anyka_pthread_create(&ntid, thread_func_initdevice, (void *)version, ANYKA_THREAD_MIN_STACK_SIZE, -1);
	if(err == 0 && ntid != 0)
	{
		/** wait until the net has been configured **/
		pthread_join(ntid, NULL);
    }

    return true;
}

/**
 * NAME     ak_qq_send_video_move_alarm
 * @BRIEF	向手Q发送相应的报警信息
 * @PARAM        
  @RETURN	
 * @RETVAL	
 */
void ak_qq_send_video_move_alarm(int alarm_type, int save_flag, char *save_path, int start_time, int time_len)
{    
    int msg_id = 0;
    char *msg_content = NULL;
    
	if(qq_online_status_info == 0)
	{
		anyka_print("[%s:%d] can't send message, device offline and remove file %s!\n", 
				__func__, __LINE__, save_path);
		remove(save_path);
		return ;
	}
	switch(alarm_type)
    {
        case SYS_CHECK_VOICE_ALARM:
            msg_content = "检测到声音，请注意!";
            msg_id = 3;
            break;
            
        case SYS_CHECK_VIDEO_ALARM:
            msg_content = "检测到移动，请注意!";
            msg_id = 1;
            break;
            
        case SYS_CHECK_OTHER_ALARM:
            msg_content = "other check, please notice it!";
            msg_id = 5;
            break;
    }
    ak_qq_send_msg(msg_id, save_path, "收到报警信息", msg_content, start_time);
}


void ak_qq_send_voice_to_phone(char *save_path, int start_time)
{    
   // int msg_id;
   // char *msg_content;
    
	if(qq_online_status_info == 0)
	{
		anyka_print("[%s:%d] can't send message, device offline and remove file %s!\n", 
				__func__, __LINE__, save_path);
		remove(save_path);
		return ;
	}
    ak_qq_send_msg(2, save_path, "收到语音留言", "小Q呼叫主人，请查看！", start_time);
}

void qq_on_erase_all_binders(int error_code)
{
	anyka_print("[%s:%d] retcode=%d\n", __func__, __LINE__, error_code);
}

void ak_qq_unbind()
{
	if(qq_online_status_info == 1)
		tx_erase_all_binders(qq_on_erase_all_binders);
	else
		anyka_print("[%s] unbind failed, device is offline\n", __func__);
}

bool ak_qq_online()
{
    return qq_online_status_info;
}
#if 0
void on_smartlink_notify (tx_wifi_sync_param *pSmartlinkParam, void* pUserData){
}
#endif

//waiting time  syn
static void wait_time_sync()
{
    time_t tmp;
    struct tm *cur_time;

	tmp = time(NULL);
    cur_time = localtime(&tmp);
   	anyka_print("waiting time sync to tencent server...\n");
    while(1)
    {
	    if (cur_time->tm_year + 1900 < 2014)
	    {
			sleep(2);	    	
	    	//anyka_print("waiting time sync to tencent server...\n");
	    }else
	    	break;
    }
   	anyka_print("time sync finish!\n");    
    
}

void tencent_init(unsigned int version)
{		
    ak_qq_init();
 	initdevice(version);
    record_replay_init(); //初始化录像列表
 	
 	wait_time_sync();
}
