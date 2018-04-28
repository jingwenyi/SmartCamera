

#include <linux/input.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "common.h"
#include "anyka_dana.h"
#include "dana_interface.h"
#include "danavideo_cmd.h"
#include "danavideo.h"
#include "danavideo.h"
#include "mediatyp.h"
#include "debug.h"
#include "errdefs.h"
#include "voice_tips.h"
#include "convert.h"

#define SDC_PATH 			"/mnt/"
#define CONFIG_PATH			"/etc/jffs2/anyka_cfg.ini"
#define SYS_CONNECTED 		"/usr/share/anyka_net_connected.mp3"	//网络连接成功，应用上线提示
#define SYS_ONLINE			"/usr/share/anyka_camera_online.mp3"	//摄像机上线提示音
#define SYS_OFFLINE			"/usr/share/anyka_camera_off_line.mp3"	//摄像机下线提示音
#define SYS_CONNECTING_NET	"/usr/share/anyka_connecting_net.mp3"	//正在连接
#define SYS_GET_CONFIG		"/usr/share/anyka_camera_get_config.mp3"	//获取智能声控传递的信息
#define WIRE_LESS_TMP_DIR      "/tmp/wireless/"

int dana_time_zone = 0;
void * smart_handle = NULL;
int smart_run_flag = 0;
static pthread_mutex_t smart_voice_mutex;
unsigned long dana_hb_time = 0;
unsigned short online_status = 0;
static bool dana_airlink_called = false;

void anyka_set_osd_info(DANAVIDEOCMD_SETOSD_ARG *osd_info);
void anyka_get_osd_info(libdanavideo_osdinfo_t *osd_info);
void dana_stop_voice_control_wifi(void);
void init_dana_pzt_ctrl(void);


libdanavideo_reclist_recordinfo_t * anyka_get_reclist(DANAVIDEOCMD_RECLIST_ARG *rec_info, int *rec_num);


/**
* @brief 	anyka_set_cloud_alarm
* 			设置移动侦测相关报警信息
* @date 	2015/3
* @param:	DANAVIDEOCMD_SETALARM_ARG *alarm_info，要设置的数据
* @return 	void 
* @retval 	
*/

void anyka_set_cloud_alarm(DANAVIDEOCMD_SETALARM_ARG *alarm_info)
{
    Psystem_alarm_set_info sys_alarm = anyka_sys_alarm_info();

    sys_alarm->motion_detection = alarm_info->motion_detection;
    sys_alarm->opensound_detection = alarm_info->opensound_detection;
    sys_alarm->openi2o_detection = alarm_info->openi2o_detection;
    sys_alarm->smoke_detection = alarm_info->smoke_detection;
    sys_alarm->shadow_detection = alarm_info->shadow_detection;
    sys_alarm->other_detection = alarm_info->other_detection;

    anyka_set_sys_alarm_status();
	
}


/**
* @brief 	anyka_get_sdcard_total_size
* 			获取sd 卡总空间
* @date 	2015/3
* @param:	char *path， sd 卡挂载在系统的路径
* @return 	uint64_t 
* @retval 	返回sd 卡总空间
*/

uint64_t anyka_get_sdcard_total_size(char *path)
{
	struct statfs disk_statfs;
	uint64_t total_size;

	bzero(&disk_statfs, sizeof( struct statfs ) );

	while ( statfs( path, &disk_statfs ) == -1 ) {
		if ( errno != EINTR ) {
			anyka_print( "statfs: %s Last error == %s\n", path, strerror(errno) );
			return -1;
		}
	}
	total_size = disk_statfs.f_bsize;
	total_size = total_size * disk_statfs.f_blocks;
	return total_size;
}


/**
* @brief 	anyka_get_sdcard_free_size
* 			获取sd 卡可用空间
* @date 	2015/3
* @param:	char *path， sd 卡挂载在系统的路径
* @return 	uint64_t 
* @retval 	返回sd 卡可用空间
*/

uint64_t anyka_get_sdcard_free_size(char *path)
{
	struct statfs disk_statfs;
	uint64_t free_size;

	bzero(&disk_statfs, sizeof( struct statfs ) );

	while ( statfs( path, &disk_statfs ) == -1 ) {
		if ( errno != EINTR ) {
			anyka_print( "statfs: %s Last error == %s\n", path, strerror(errno) );
			return -1;
		}
	}
	free_size = disk_statfs.f_bsize;
	free_size = free_size * disk_statfs.f_bavail;
	return free_size;
}


/**
* @brief 	danavideo_check_current_net_mode
* 			检测当前网络模式
* @date 	2015/3
* @param:	void
* @return 	int 
* @retval 	1 --> 有线模式，0 --> 无线模式
*/

int danavideo_check_current_net_mode()
{
	char res[128] = {0};
	anyka_print("[%s:%d] checking current net working mode......\n", __func__, __LINE__);

	do_syscmd("ifconfig eth0 | grep RUNNING", res);
	
	if(strstr(res, "RUNNING")){
		anyka_print("[%s:%d] The system now working on wired mode\n", __func__, __LINE__);
		return 1;
	}else{
		anyka_print("[%s:%d] The system now working on wireless mode\n", __func__, __LINE__);
	}
	
	return 0;
}


/**
* @brief 	anyka_get_cloud_alarm
* 			获取云报警信息
* @date 	2015/3
* @param:	DANAVIDEOCMD_SETALARM_ARG *alarm_info，信息存放到该结构体
* @return 	void 
* @retval 	
*/

void anyka_get_cloud_alarm(DANAVIDEOCMD_SETALARM_ARG *alarm_info)
{
    Psystem_alarm_set_info sys_alarm = anyka_sys_alarm_info();
    
    alarm_info->motion_detection = sys_alarm->motion_detection;
    alarm_info->opensound_detection = sys_alarm->opensound_detection;
    alarm_info->openi2o_detection = sys_alarm->openi2o_detection ;
    alarm_info->smoke_detection = sys_alarm->smoke_detection ;
    alarm_info->shadow_detection = sys_alarm->shadow_detection;
    alarm_info->other_detection = sys_alarm->other_detection ;
}


/**
* @brief 	dana_get_record_plan
* 			获取录像计划
* @date 	2015/3
* @param:	uint32_t *plan_num，获取的最大数目
* @return 	libdanavideo_recplanget_recplan_t *
* @retval 	指向获取结构的指针
*/

libdanavideo_recplanget_recplan_t *dana_get_record_plan(uint32_t *plan_num)
{
    int i, index, hour, min, sec, j;
    video_record_setting * psys_setting;
    libdanavideo_recplanget_recplan_t *local_plan;

    
    psys_setting = anyka_get_record_plan();		//get sys record plan
    *plan_num = 0;
    for(i = 0, index = 0; i < MAX_RECORD_PLAN_NUM; i++)
    {
        anyka_print("haveplan :%d, %d\n", i, psys_setting->record_plan[i].active);
        if(psys_setting->record_plan[i].active)
        {
            (*plan_num) ++;
        }
    }

    if(*plan_num == 0)
    {
        anyka_print("have no plan\n");
        return NULL;
    }
    local_plan = malloc(sizeof(libdanavideo_recplanget_recplan_t)*(*plan_num));

    for(i = 0, index = 0; i < MAX_RECORD_PLAN_NUM; i++)
    {
        if(psys_setting->record_plan[i].active)
        {
            local_plan[index].status = psys_setting->record_plan[i].active;
            hour = psys_setting->record_plan[i].end_time / 3600 ;
            min = (psys_setting->record_plan[i].end_time % 3600) / 60 ;
            sec = psys_setting->record_plan[i].end_time % 60;
            sprintf(local_plan[index].end_time, "%02d:%02d:%02d", hour, min, sec);
            hour = psys_setting->record_plan[i].start_time / 3600 ;
            min = (psys_setting->record_plan[i].start_time % 3600) / 60 ;
            sec = psys_setting->record_plan[i].start_time % 60;
            sprintf(local_plan[index].start_time, "%02d:%02d:%02d", hour, min, sec);
            local_plan[index].record_no = index;
            local_plan[index].week_count = 0;

            
            for(j = 0; j < 7; j ++)
            {
                if(psys_setting->record_plan[i].day_flag & ( 1 << j))
                {
                    if(j == 0)
                    {
                        local_plan[index].week[local_plan[index].week_count ++] = DANAVIDEO_REC_WEEK_SUN;
                    }
                    else
                    {
                        local_plan[index].week[local_plan[index].week_count ++] = j;
                    }
                }
            }
            index ++ ;
        }
    }


    return local_plan;
    
}


/**
* @brief 	dana_get_record_plan
* 			获取录像计划
* @date 	2015/3
* @param:	uint32_t *plan_num，获取的最大数目
* @return 	libdanavideo_recplanget_recplan_t *
* @retval 	指向获取结构的指针
*/

void dana_set_record_plan(DANAVIDEOCMD_RECPLANSET_ARG * pnet_record_plan)
{
    int i;
    int hour, min, sec;
    record_plan_info local_plan;
    video_record_setting * psys_setting;
    psys_setting = anyka_get_record_plan();

    local_plan.active = pnet_record_plan->status;
    sscanf(pnet_record_plan->start_time, "%d:%d:%d", &hour, &min, &sec);
    local_plan.start_time = hour * 3600 + min * 60 + sec;
    sscanf(pnet_record_plan->end_time, "%d:%d:%d", &hour, &min, &sec);
    local_plan.end_time =  hour * 3600 + min * 60 + sec;
    local_plan.day_flag = 0;

    for(i = 0; i < pnet_record_plan->week_count; i++)
    {
        
        if(pnet_record_plan->week[i] == DANAVIDEO_REC_WEEK_SUN)
        {
            local_plan.day_flag |= 1;
        }
        else
        {
            local_plan.day_flag |= (1 << pnet_record_plan->week[i]);
        }
    }
    anyka_net_set_record_plan(&local_plan);

}


/**
* @brief 	danavideoconn_created
* 			大拿启动用的callback
* @date 	2015/3
* @param:	uint32_t *plan_num，获取的最大数目
* @return 	libdanavideo_recplanget_recplan_t *
* @retval 	指向获取结构的指针
*/

uint32_t danavideoconn_created(void *arg) // pdana_video_conn_t
{
    dbg("TEST danavideoconn_created\n");
    pdana_video_conn_t *danavideoconn = (pdana_video_conn_t *)arg; 
    // set user data  
    MYDATA *mydata = (MYDATA*) calloc(1, sizeof(MYDATA));
    if (NULL == mydata) {
        dbg("TEST danavideoconn_created failed calloc\n");
        return 0;
    }
    dana_stop_voice_control_wifi();
   // mydata->thread_media = -1;
   // mydata->thread_talkback = -1;
    mydata->danavideoconn = danavideoconn;
    mydata->chan_no = 1;
    strncpy(mydata->appname, "anyka_video", strlen("anyka_video"));

    
    if (0 != lib_danavideo_set_userdata(danavideoconn, mydata)) {
        dbg("TEST danavideoconn_created lib_danavideo_set_userdata failed\n");
        free(mydata);
        return 0;
    }

    dbg("TEST danavideoconn_created succeed\n");

    return 0; 
}


/**
* @brief 	danavideoconn_created
* 			大拿停止用 的callback
* @date 	2015/3
* @param:	void *arg
* @return 	void 
* @retval 	
*/

void danavideoconn_aborted(void *arg) // pdana_video_conn_t
{
    dbg("\n\n\n\n\n\x1b[34mTEST danavideoconn_aborted\x1b[0m\n\n\n\n\n\n");
    pdana_video_conn_t *danavideoconn = (pdana_video_conn_t *)arg;
    MYDATA *mydata;
    if (0 != lib_danavideo_get_userdata(danavideoconn, (void **)&mydata)) {
        dbg("TEST danavideoconn_aborted lib_danavideo_get_userdata failed, mem-leak\n");
        return;
    }
	//free resource
    anyka_clear_all_resource(mydata);
    if (NULL != mydata) {
        // stop video thread
   //     mydata->run_media = false;
  //      mydata->run_talkback = false;

        usleep(2000*1000); // 浣滃悓姝ュ垽鏂槸鍚﹂��鍏?       

        dbg("TEST danavideoconn_aborted APPNAME: %s\n", mydata->appname);

        free(mydata);
    }
    
    dbg("TEST danavideoconn_aborted succeed\n");
    // get user data
    return;
}



/**
* @brief 	danavideoconn_command_handler
* 			大拿命令解析处理
* @date 	2015/3
* @param:	void *arg，参数
			dana_video_cmd_t cmd， 命令
			uint32_t trans_id，发送id
			void *cmd_arg，命令参数
			uint32_t cmd_arg_len，命令参数长度
* @return 	void 
* @retval 	
*/

void danavideoconn_command_handler(void *arg, dana_video_cmd_t cmd, uint32_t trans_id, void *cmd_arg, uint32_t cmd_arg_len) // pdana_video_conn_t
{
    dbg("TEST danavideoconn_command_handler: %d\n", cmd);
    pdana_video_conn_t *danavideoconn = (pdana_video_conn_t *)arg;
    // 鍝嶅簲涓嶅悓鐨勫懡涔?   
    MYDATA *mydata;

	/** get userdata **/
    if (0 != lib_danavideo_get_userdata(danavideoconn, (void**)&mydata)) {
        dbg("TEST danavideoconn_command_handler lib_danavideo_get_userdata failed\n");
        return;
    }
    dbg("TEST danavideoconn_command_handler APPNAME: %s\n", mydata->appname);

    uint32_t i;
    uint32_t error_code;
    char *code_msg;
    
    // 鍙戦�佸懡浠ゅ洖搴旂殑鎺ュ彛杩涜涠?   
    switch (cmd) {
        case DANAVIDEOCMD_DEVDEF:					/** Recover Device **/
            {
				
				anyka_print("*********Recover Device Setting**********\n");
				
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_DEVDEF\n");
                DANAVIDEOCMD_DEVDEF_ARG *devdef_arg = (DANAVIDEOCMD_DEVDEF_ARG *)cmd_arg;
                dbg("devdef_arg\n");
                dbg("ch_no: %ld\n", devdef_arg->ch_no);
                dbg("\n");
                anyka_clear_all_resource(mydata);
                video_record_err_stop();
				
				anyka_print("***Ready to recover system configure!!!***\n");
			
                system("/usr/sbin/recover_cfg.sh");
                sync();
                system("reboot");
				
                error_code = 0;
                code_msg = (char *)"";
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)"", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_DEVDEF send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_DEVDEF send response failed\n");
                }
            }
            break; 
        case DANAVIDEOCMD_DEVREBOOT:		/** reboot Device **/
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_DEVREBOOT\n");
                DANAVIDEOCMD_DEVREBOOT_ARG *devreboot_arg = (DANAVIDEOCMD_DEVREBOOT_ARG *)cmd_arg;
                dbg("devreboot_arg\n");
                dbg("ch_no: %ld\n", devreboot_arg->ch_no);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                anyka_clear_all_resource(mydata);
                video_record_err_stop();
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)"", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_DEVREBOOT send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_DEVREBOOT send response failed\n");
                } 
                sleep(1);
                system("reboot");
            }
            break; 
        case DANAVIDEOCMD_GETSCREEN:	/** get screen, take photo **/
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETSCREEN\n");
                DANAVIDEOCMD_GETSCREEN_ARG *getscreen_arg = (DANAVIDEOCMD_GETSCREEN_ARG *)cmd_arg;
                dbg("getcreen_arg\n");
                dbg("ch_no: %ld\n", getscreen_arg->ch_no);
                dbg("\n");
                error_code = 0;
                code_msg = "";
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, "", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETSCREEN send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETSCREEN send response failed\n");
                }
                // 鑾峰彇涓�鍓浘鐗囪皟鐢╨ib_danavideoconn_sendpicmsg鎺ュ彛鍙戣究//                photograph_setcallack(anyka_send_dana_pictrue, (void *)danavideoconn);
                anyka_start_picture(mydata, anyka_send_dana_pictrue);

            }
            break;
        case DANAVIDEOCMD_GETALARM:		/** get cloud alarm info **/
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETALARM\n");
                DANAVIDEOCMD_GETALARM_ARG *getalarm_arg = (DANAVIDEOCMD_GETALARM_ARG *)cmd_arg;
                dbg("getalarm_arg\n");
                dbg("ch_no: %ld\n", getalarm_arg->ch_no);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                DANAVIDEOCMD_SETALARM_ARG sys_alarm;
                anyka_get_cloud_alarm(&sys_alarm);		/** get cloud alarm info **/
                if (lib_danavideo_cmd_getalarm_response(danavideoconn, trans_id, error_code, code_msg, 
                    sys_alarm.motion_detection, sys_alarm.opensound_detection, sys_alarm.openi2o_detection, 
                    sys_alarm.smoke_detection, sys_alarm.shadow_detection, sys_alarm.other_detection)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETALARM send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETALARM send response failed\n");
                }
            }
            break; 
        case DANAVIDEOCMD_GETBASEINFO:	/** get device base info **/
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETBASEINFO\n");
                DANAVIDEOCMD_GETBASEINFO_ARG *getbaseinfo_arg = (DANAVIDEOCMD_GETBASEINFO_ARG *)cmd_arg;
                dbg("getbaseinfo_arg\n");
                dbg("ch_no: %ld\n", getbaseinfo_arg->ch_no);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                char *dana_id = (char *)"di3_1";
                char *api_ver = (char *)"di3_2";
                char *sn      = (char *)"di3_3";
                char *device_name = (char *)"di3_4";
                char *rom_ver = (char *)"di3_5";
                uint32_t device_type = 1;
                uint32_t ch_num = 25;
                //uint64_t sdc_size = 1024;
                uint64_t sdc_size = anyka_get_sdcard_total_size(SDC_PATH);	/** get sd card total size **/
                //uint64_t sdc_free = 512;
				uint64_t sdc_free = anyka_get_sdcard_free_size(SDC_PATH);	/** get sd card free size **/
                if (lib_danavideo_cmd_getbaseinfo_response(danavideoconn, trans_id, error_code, code_msg, dana_id, api_ver, sn, device_name, rom_ver, device_type, ch_num, sdc_size, sdc_free)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETBASEINFO send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETBASEINFO send response failed\n");
                }
            }
            break;
        case DANAVIDEOCMD_GETCOLOR:		/** get system color **/
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETCOLOR\n");
                DANAVIDEOCMD_GETCOLOR_ARG *getcolor_arg = (DANAVIDEOCMD_GETCOLOR_ARG *)cmd_arg;
                dbg("getcolor_arg\n");
                dbg("ch_no: %ld\n", getcolor_arg->ch_no);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
    			int brightness, contrast, saturation, hue ;
    			/************************************************************************/
    			/*    
        			 you will get lum/contrast/saturation/hue  from ipc,and send to client
        			 the value is between 0 to 100*/
    			/************************************************************************/
    			anyka_print("GET_COLOR\n");
				/** get system color **/
    			Get_Color( &brightness, &contrast, &saturation, &hue );
                if (lib_danavideo_cmd_getcolor_response(danavideoconn, trans_id, error_code, code_msg, brightness, contrast, saturation, hue)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETCOLOR send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETCOLOR send response failed\n");
                }
            }
            break; 
        case DANAVIDEOCMD_GETFLIP:	/** get camera flip **/
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETFLIP\n");
                DANAVIDEOCMD_GETFLIP_ARG *getflip_arg = (DANAVIDEOCMD_GETFLIP_ARG *)cmd_arg;
                dbg("getflip_arg_arg\n");
                dbg("ch_no: %ld\n", getflip_arg->ch_no);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                uint32_t flip_type = GetCamerFilp();	/** get camera flip **/
                if (lib_danavideo_cmd_getflip_response(danavideoconn, trans_id, error_code, code_msg, flip_type)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETFLIP send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETFLIP send response failed\n");
                }
            }
            break; 
        case DANAVIDEOCMD_GETFUNLIST:	/** get function list **/
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETFUNLIST\n");
                DANAVIDEOCMD_GETFUNLIST_ARG *getfunlist_arg = (DANAVIDEOCMD_GETFUNLIST_ARG *)cmd_arg;
                dbg("getfunlist_arg\n");
                dbg("ch_no: %ld\n", getfunlist_arg->ch_no);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                uint32_t methodes_count = 43;
            char *methods[] = { (char *)"DANAVIDEOCMD_DEVDEF", 
                                (char *)"DANAVIDEOCMD_DEVREBOOT", 
                                (char *)"DANAVIDEOCMD_GETSCREEN",
                                (char *)"DANAVIDEOCMD_GETALARM",
                                (char *)"DANAVIDEOCMD_GETBASEINFO",
                                (char *)"DANAVIDEOCMD_GETCOLOR",
                                (char *)"DANAVIDEOCMD_GETFLIP",
                                (char *)"DANAVIDEOCMD_GETFUNLIST",
                                (char *)"DANAVIDEOCMD_GETNETINFO",
                                (char *)"DANAVIDEOCMD_GETPOWERFREQ",
                                (char *)"DANAVIDEOCMD_GETTIME",
                                (char *)"DANAVIDEOCMD_GETWIFIAP",
                                (char *)"DANAVIDEOCMD_GETWIFI",
                                (char *)"DANAVIDEOCMD_PTZCTRL",
                                (char *)"DANAVIDEOCMD_SDCFORMAT",
                                (char *)"DANAVIDEOCMD_SETALARM",
                                (char *)"DANAVIDEOCMD_SETCHAN",
                                (char *)"DANAVIDEOCMD_SETCOLOR",
                                (char *)"DANAVIDEOCMD_SETFLIP",
                                (char *)"DANAVIDEOCMD_SETNETINFO",
                                (char *)"DANAVIDEOCMD_SETPOWERFREQ",
                                (char *)"DANAVIDEOCMD_SETTIME",
                                (char *)"DANAVIDEOCMD_SETVIDEO",
                                (char *)"DANAVIDEOCMD_SETWIFIAP",
                                (char *)"DANAVIDEOCMD_SETWIFI",
                                (char *)"DANAVIDEOCMD_STARTAUDIO",
                                (char *)"DANAVIDEOCMD_STARTTALKBACK",
                                (char *)"DANAVIDEOCMD_STARTVIDEO",
                                (char *)"DANAVIDEOCMD_STOPAUDIO",
                                (char *)"DANAVIDEOCMD_STOPTALKBACK",
                                (char *)"DANAVIDEOCMD_STOPVIDEO",
                                (char *)"DANAVIDEOCMD_RECLIST",
                                (char *)"DANAVIDEOCMD_RECPLAY",
                                (char *)"DANAVIDEOCMD_RECSTOP",
                                (char *)"DANAVIDEOCMD_RECACTION",
                                (char *)"DANAVIDEOCMD_RECSETRATE",
                                (char *)"DANAVIDEOCMD_RECPLANGET",
                                (char *)"DANAVIDEOCMD_RECPLANSET",
                                (char *)"DANAVIDEOCMD_EXTENDMETHOD" ,
                                (char *)"DANAVIDEOCMD_SETOSD",
                                (char *)"DANAVIDEOCMD_GETOSD",    
                                (char *)"DANAVIDEOCMD_SETCHANNAME",
                                (char *)"DANAVIDEOCMD_GETCHANNAME" 
                                
                                };
                if (lib_danavideo_cmd_getfunlist_response(danavideoconn, trans_id, error_code, code_msg, methodes_count, (const char**)methods)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETFUNLIST send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETFUNLIST send response failed\n");
                }
            }
            break; 
        case DANAVIDEOCMD_GETNETINFO:	/** get net info **/
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETNETINFO\n");
                DANAVIDEOCMD_GETNETINFO_ARG *getnetinfo_arg = (DANAVIDEOCMD_GETNETINFO_ARG *)cmd_arg;
                dbg("getnetinfo_arg\n");
                dbg("ch_no: %ld\n", getnetinfo_arg->ch_no);
                dbg("\n");
                Psystem_net_set_info sys_net = anyka_get_sys_net_setting();	/** get net info **/
                error_code = 0;
                code_msg = (char *)"";
                if (lib_danavideo_cmd_getnetinfo_response(danavideoconn, trans_id, error_code, code_msg, 
                    sys_net->dhcp, sys_net->ipaddr, sys_net->netmask, sys_net->gateway, sys_net->dhcp, sys_net->firstdns, sys_net->backdns, 80)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETNETINFO send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETNETINFO send response failed\n");
                }
            }
            break;
        case DANAVIDEOCMD_GETPOWERFREQ:		/** get power frequece **/
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETPOWERFREQ\n");
                DANAVIDEOCMD_GETPOWERFREQ_ARG *getpowerfreq_arg = (DANAVIDEOCMD_GETPOWERFREQ_ARG *)cmd_arg;
                dbg("getpowerfreq_arg\n");
                dbg("ch_no: %ld\n", getpowerfreq_arg->ch_no);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                int freq = 0;
                /************************************************************************/
                /*
                            you can get the power freq from ipc,and send to client
                            0: 50HZ
                            1: 60HZ
                            */
                /************************************************************************/
                freq = Get_Freq();	/** get power frequece **/
                if (lib_danavideo_cmd_getpowerfreq_response(danavideoconn, trans_id, error_code, code_msg, freq)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETPOWERFREQ send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETPOWERFREQ send response failed\n");
                }
            }
            break; 
	
		/******* ------ modefied in 2014-08-20,gettime function ------ ********/
        case DANAVIDEOCMD_GETTIME:	/** get system times **/
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETTIME\n");
                DANAVIDEOCMD_GETTIME_ARG *gettime_arg = (DANAVIDEOCMD_GETTIME_ARG *)cmd_arg;
                dbg("gettime_arg_arg\n");
                dbg("ch_no: %ld\n", gettime_arg->ch_no);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                
                int64_t now_time = (int64_t)gettime();	/** get system times **/

                char *time_zone = (char *)"GMT+08:00";	/** set timezone **/
                char *ntp_server_1 = "NTP_SRV_1";		/** ntp server 1 **/
                char *ntp_server_2 = "NTP_SRV_2";		/** ntp server 2 **/
                if (lib_danavideo_cmd_gettime_response(danavideoconn, trans_id, error_code, code_msg, 
													now_time, time_zone, ntp_server_1, ntp_server_2)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETTIME send response succeed\n");
                    anyka_print("TEST danavideoconn_command_handler DANAVIDEOCMD_GETTIME send response succeed\n");
                } 
				else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETTIME send response failed\n");
                    anyka_print("TEST danavideoconn_command_handler DANAVIDEOCMD_GETTIME send response failed\n");
                }
            }
            break; 

        case DANAVIDEOCMD_GETWIFIAP:	/** get ap , use system wlan1 to scan  **/
            {
				anyka_print("\nGetWifiAP, searching wifi......\n");
				
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETWIFIAP\n");
                DANAVIDEOCMD_GETWIFIAP_ARG *getwifiap_arg = (DANAVIDEOCMD_GETWIFIAP_ARG *)cmd_arg;
                dbg("getwifiap_arg\n");
                dbg("ch_no: %ld\n", getwifiap_arg->ch_no);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                uint32_t wifi_device = 1;         
				int j;

                Panyka_ap_list ap_list = anyka_get_ap_list(); /** get ap list , use system wlan1 to scan  **/
                
                libdanavideo_wifiinfo_t *wifi_list = NULL;				
				
				wifi_list = (libdanavideo_wifiinfo_t *)malloc(sizeof(libdanavideo_wifiinfo_t) * ap_list->ap_num);
				if(wifi_list == NULL)
				{
					anyka_print("fail to malloc.\n");
					ap_list->ap_num = 0;
				}
				else
				{
					/** strore wifi list message by loop **/
					for(j = 0; j < ap_list->ap_num; j++)
					{
						wifi_list[j].enc_type = ap_list->ap_info[j].enc_type;
						wifi_list[j].quality  = ap_list->ap_info[j].quality;
						strcpy(wifi_list[j].essid, ap_list->ap_info[j].essid);
						anyka_print("wifi:%d, enc:%d, quality:%d, ssid:%s\n", 
							j+1, wifi_list[j].enc_type, wifi_list[j].quality, wifi_list[j].essid);
					}
					//wifi_list = (libdanavideo_wifiinfo_t *)(&ap_list->ap_info[0]);
				}
                
    			anyka_print("---------------->>>LISTWIFIAP_REQ: %d\n", ap_list->ap_num);
    			
                error_code = 0;
                code_msg = (char *)"";
                if (lib_danavideo_cmd_getwifiap_response(danavideoconn, trans_id, error_code, code_msg, wifi_device, ap_list->ap_num, (const libdanavideo_wifiinfo_t *)wifi_list)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETWIFIAP send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETWIFIAP send response failed\n");
                }
            
				if(ap_list)
				{
					free(ap_list);
				}
				
				if(wifi_list)
				{
					free(wifi_list);
				}
            }
            break; 
        case DANAVIDEOCMD_GETWIFI:	/** get wifi message from system **/
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETWIFI\n");
                DANAVIDEOCMD_GETWIFI_ARG *getwifi_arg = (DANAVIDEOCMD_GETWIFI_ARG *)cmd_arg;
                dbg("getwifi_arg\n");
                dbg("ch_no: %ld\n", getwifi_arg->ch_no);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                Psystem_wifi_set_info resp = anyka_get_sys_wifi_setting();
                if (lib_danavideo_cmd_getwifi_response(danavideoconn, trans_id, error_code, code_msg, resp->ssid, resp->passwd, resp->enctype)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETWIFI send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETWIFI send response failed\n");
                }
            }
            break;
        case DANAVIDEOCMD_PTZCTRL:	/** ptz control  **/
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_PTZCTRL\n");
                DANAVIDEOCMD_PTZCTRL_ARG *ptzctrl_arg = (DANAVIDEOCMD_PTZCTRL_ARG *)cmd_arg;
                dbg("ptzctrl_arg\n");
                dbg("ch_no: %ld\n", ptzctrl_arg->ch_no);
                dbg("code: %ld\n", ptzctrl_arg->code);
                dbg("para1: %ld\n", ptzctrl_arg->para1);
                dbg("para2: %ld\n", ptzctrl_arg->para2);
                dbg("\n");
                anyka_set_pzt_control(ptzctrl_arg->code, ptzctrl_arg->para1);
                switch (ptzctrl_arg->code) {
                    case DANAVIDEO_PTZ_CTRL_MOVE_UP:	/**  ptz control  , move up**/ 
                        dbg("DANAVIDEO_PTZ_CTRL_MOVE_UP\n");
                        break;
                    case DANAVIDEO_PTZ_CTRL_MOVE_DOWN:	/**  ptz control  , move down**/ 
                        dbg("DANAVIDEO_PTZ_CTRL_MOVE_UP\n");
                        break;
                    case DANAVIDEO_PTZ_CTRL_MOVE_LEFT:	/**  ptz control  , move left**/ 
                        dbg("DANAVIDEO_PTZ_CTRL_MOVE_UP\n");
                        break;
                    case DANAVIDEO_PTZ_CTRL_MOVE_RIGHT:	/**  ptz control  , move right**/ 
                        dbg("DANAVIDEO_PTZ_CTRL_MOVE_UP\n");
                        break;
                        // ...
                }
                error_code = 0;
                code_msg = (char *)"";
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)"", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_PTZCTRL send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_PTZCTRL send response failed\n");
                }
            }
            break; 
        case DANAVIDEOCMD_SDCFORMAT:	/** format sd card  **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SDCFORMAT\n");
                DANAVIDEOCMD_SDCFORMAT_ARG *sdcformat_arg = (DANAVIDEOCMD_SDCFORMAT_ARG *)cmd_arg; 
                dbg("sdcformat_arg\n");
                dbg("ch_no: %ld\n", sdcformat_arg->ch_no);
                dbg("\n");

                if(video_record_check_run())	//If system is being record, do not format the sd card
                {
                    error_code = E_BUSY;
                    code_msg = (char *)"system is recording\n";
                }
				else if(check_sdcard() < 0)	/** sd card is out of system  **/ 
				{
					error_code = E_INVALID_OPERATION;
					code_msg = (char *)"System has no SD card!\n";
				}
                else
                {
                    system("umount /mnt/");		/** do format **/ 
                    system("mkfs.vfat /dev/mmcblk0p1");
                    error_code = 0;
                    code_msg = (char *)"";
    				//mount 
    				system("mount /dev/mmcblk0p1 /mnt/");
                }
			
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)"", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SDCFORMAT send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SDCFORMAT send response failed\n");
                }
            }
            break; 
        case DANAVIDEOCMD_SETALARM:	/** set sys alarm **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETALARM\n");
                DANAVIDEOCMD_SETALARM_ARG *setalarm_arg = (DANAVIDEOCMD_SETALARM_ARG *)cmd_arg;
                dbg("setalarm_arg\n");
                dbg("ch_no: %ld\n", setalarm_arg->ch_no);
                dbg("motion_detection: %ld\n", setalarm_arg->motion_detection);
                dbg("opensound_detection: %ld\n", setalarm_arg->opensound_detection);
                dbg("openi2o_detection: %ld\n", setalarm_arg->openi2o_detection);
                dbg("smoke_detection: %ld\n", setalarm_arg->smoke_detection);
                dbg("shadow_detection: %ld\n", setalarm_arg->shadow_detection);
                dbg("other_detection: %ld\n", setalarm_arg->other_detection);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                
                dbg("motion_detection: %d\n", setalarm_arg->motion_detection);
                dbg("opensound_detection: %d\n", setalarm_arg->opensound_detection);
                if(setalarm_arg->motion_detection)
				{	
					/** start sys motion_detection **/ 
                    anyka_dection_start(setalarm_arg->motion_detection, SYS_CHECK_VIDEO_ALARM, anyka_send_video_move_alarm, NULL);
                }
				else
				{
                	/** stop sys motion_detection **/ 
                    anyka_dection_stop(SYS_CHECK_VIDEO_ALARM);
                }
                if(setalarm_arg->opensound_detection)
				{
					/** start sys opensound_detection **/ 
                    anyka_dection_start(setalarm_arg->opensound_detection, SYS_CHECK_VOICE_ALARM, anyka_send_video_move_alarm, NULL);
                }
				else
				{
	                /** stop sys opensound_detection **/ 
                    anyka_dection_stop(SYS_CHECK_VOICE_ALARM);
                }
				if(setalarm_arg->other_detection){
					/** start sys other_detection **/ 
					anyka_dection_start(setalarm_arg->other_detection, SYS_CHECK_OTHER_ALARM, anyka_send_video_move_alarm, NULL);	
				}else{
					/** stop sys other_detection **/ 
					anyka_dection_stop(SYS_CHECK_OTHER_ALARM);
				}
				
                anyka_set_cloud_alarm(setalarm_arg);
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)"", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETALARM send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETALARM send response failed\n");
                }
            }
            break; 
        case DANAVIDEOCMD_SETCHAN:	/** set channal **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETCHAN\n");
                DANAVIDEOCMD_SETCHAN_ARG *setchan_arg = (DANAVIDEOCMD_SETCHAN_ARG *)cmd_arg; 
                dbg("setchan_arg\n");
                dbg("ch_no: %ld\n", setchan_arg->ch_no);
                dbg("chans_count: %zd\n", setchan_arg->chans_count);
                for (i=0; i<setchan_arg->chans_count; i++) {
                    dbg("chans[%ld]: %ld\n", i, setchan_arg->chans[i]);
                }
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)"", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETCHAN send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETCHAN send response failed\n");
                }
            }
            break; 
        case DANAVIDEOCMD_SETCOLOR:	/** set color **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETCOLOR\n"); 
                DANAVIDEOCMD_SETCOLOR_ARG *setcolor_arg = (DANAVIDEOCMD_SETCOLOR_ARG *)cmd_arg;
                dbg("setcolor_arg\n");
                dbg("ch_no: %ld\n", setcolor_arg->ch_no);
                dbg("video_rate: %ld\n", setcolor_arg->video_rate);
                dbg("brightness: %ld\n", setcolor_arg->brightness);
                dbg("contrast: %ld\n", setcolor_arg->contrast);
                dbg("saturation: %ld\n", setcolor_arg->saturation);
                dbg("hue: %ld\n", setcolor_arg->hue);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)"", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETCOLOR send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETCOLOR send response failed\n");
                }
            }
            break;
        case DANAVIDEOCMD_SETFLIP:		/** set flip **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETFLIP\n"); 
                DANAVIDEOCMD_SETFLIP_ARG *setflip_arg = (DANAVIDEOCMD_SETFLIP_ARG *)cmd_arg;
                dbg("setflip_arg\n");
                dbg("ch_no: %ld\n", setflip_arg->ch_no);
                dbg("flip_type: %ld\n", setflip_arg->flip_type);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                SetCamerFlip(setflip_arg->flip_type);
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)"", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETFLIP send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETFLIP send response failed\n");
                }
            }
            break; 
        case DANAVIDEOCMD_SETNETINFO:		/** set net info **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETNETINFO\n"); 
                DANAVIDEOCMD_SETNETINFO_ARG *setnetinfo_arg = (DANAVIDEOCMD_SETNETINFO_ARG *)cmd_arg;
                dbg("setnetinfo_arg\n");
                dbg("ch_no: %ld\n", setnetinfo_arg->ch_no);
                dbg("ip_type: %ld\n", setnetinfo_arg->ip_type);
                dbg("ipaddr: %s\n", setnetinfo_arg->ipaddr);
                dbg("netmask: %s\n", setnetinfo_arg->netmask);
                dbg("gateway: %s\n", setnetinfo_arg->gateway);
                dbg("dns_type: %ld\n", setnetinfo_arg->dns_type);
                dbg("dns_name1: %s\n", setnetinfo_arg->dns_name1);
                dbg("dns_name2: %s\n", setnetinfo_arg->dns_name2);
                dbg("http_port: %ld\n", setnetinfo_arg->http_port);
                dbg("\n");
                
                Psystem_net_set_info net_info = anyka_get_sys_net_setting(); /** get net info **/ 
                net_info->dhcp = setnetinfo_arg->ip_type;
                strcpy(net_info->ipaddr, setnetinfo_arg->ipaddr);
                strcpy(net_info->netmask, setnetinfo_arg->netmask);
                strcpy(net_info->gateway, setnetinfo_arg->gateway);
				if(!setnetinfo_arg->dns_type)
				{
	                strcpy(net_info->firstdns, setnetinfo_arg->dns_name1);
	                strcpy(net_info->backdns, setnetinfo_arg->dns_name2);
				}
                anyka_set_sys_net_setting(net_info);	/** set net info **/ 
                error_code = 0;
                code_msg = (char *)"";
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)"", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETNETINFO send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETNETINFO send response failed\n");
                }
            }
            break; 
        case DANAVIDEOCMD_SETPOWERFREQ:	/** set power frequece **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETPOWERFREQ\n"); 
                DANAVIDEOCMD_SETPOWERFREQ_ARG *setpowerfreq_arg = (DANAVIDEOCMD_SETPOWERFREQ_ARG *)cmd_arg;
                dbg("setpowerfreq_arg\n");
                dbg("ch_no: %ld\n", setpowerfreq_arg->ch_no);
                if (DANAVIDEO_POWERFREQ_50HZ == setpowerfreq_arg->freq) {
                    dbg("freq: DANAVIDEO_POWERFREQ_50HZ\n");
                } else if (DANAVIDEO_POWERFREQ_60HZ == setpowerfreq_arg->freq) {
                    dbg("freq: DANAVIDEO_POWERFREQ_60HZ\n");
                } else {
                    dbg("UnKnown freq: %ld\n", setpowerfreq_arg->freq);
                }
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                Set_Freq(setpowerfreq_arg->freq);	/** set power frequece **/ 
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)"", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETPOWERFREQ send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETPOWERFREQ send response failed\n");
                }
            }
            break; 

        case DANAVIDEOCMD_SETTIME:	/** set sys time **/ 
            {
				anyka_print("\nSetting times...\n");	
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETTIME\n"); 
                DANAVIDEOCMD_SETTIME_ARG *settime_arg = (DANAVIDEOCMD_SETTIME_ARG *)cmd_arg;
                dbg("settime_arg\n");
                dbg("ch_no: %ld\n", settime_arg->ch_no);

				anyka_print("\nSet time, now time is :\n");
				/** set sys time **/ 
				if(settime(settime_arg->now_time) < 0)	
				{
					anyka_print("Settime failed\n");
					break;
				}
				else
				{
					anyka_print("settime success\n");
				}

                dbg("now_time: %d\n", (int)settime_arg->now_time);

                dbg("time_zone: %s\n", settime_arg->time_zone);
                if (settime_arg->has_ntp_server1) {
                    dbg("ntp_server_1: %s\n", settime_arg->ntp_server1);
                }
                if (settime_arg->has_ntp_server2) {
                    dbg("ntp_server_2: %s\n", settime_arg->ntp_server2);
                }
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)"", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETTIME send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETTIME send response failed\n");
                }
			}
            break;

        case DANAVIDEOCMD_SETVIDEO:		/** set sys video **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETVIDEO\n"); 
                DANAVIDEOCMD_SETVIDEO_ARG *setvideo_arg = (DANAVIDEOCMD_SETVIDEO_ARG *)cmd_arg;
                dbg("setvideo_arg\n");
                dbg("ch_no: %ld\n", setvideo_arg->ch_no);
                dbg("video_quality: %ld\n", setvideo_arg->video_quality);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";

                /** set video quality **/ 
                int video_quality = setvideo_arg->video_quality;

                if(mydata->device_type == 1 && video_quality >= 60)
                {
                    video_quality = 59;
                }
                
                if(mydata->device_type == 3)
                {
                    if(video_quality <= 50)
                    {
                        video_quality = video_quality / 2;
                        video_quality --;
                    }
                    else 
                    {
                        video_quality -= 10;
                    }
                }
				/** start video **/ 
                uint32_t set_video_fps = anyka_start_video(video_quality, mydata, NULL);
                if (lib_danavideo_cmd_setvideo_response(danavideoconn, trans_id, error_code, code_msg, set_video_fps)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETVIDEO send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETVIDEO send response failed\n");
                }
            }
            break; 
        case DANAVIDEOCMD_SETWIFIAP:	/** set ap **/ 
            {
				anyka_print("\n\t--------enter SETWIFIAP-case-------\n");

                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETWIFIAP\n"); 
                DANAVIDEOCMD_SETWIFIAP_ARG *setwifiap_arg = (DANAVIDEOCMD_SETWIFIAP_ARG *)cmd_arg;
                dbg("setwifiap_arg\n");
                dbg("ch_no: %ld\n", setwifiap_arg->ch_no);
                dbg("ip_type: %ld\n", setwifiap_arg->ip_type);
                dbg("ipaddr: %s\n", setwifiap_arg->ipaddr);
                dbg("netmask: %s\n", setwifiap_arg->netmask);
                dbg("gateway: %s\n", setwifiap_arg->gateway);
                dbg("dns_name1: %s\n", setwifiap_arg->dns_name1);
                dbg("dns_name2: %s\n", setwifiap_arg->dns_name2);
                dbg("essid: %s\n", setwifiap_arg->essid);
                dbg("auth_key: %s\n", setwifiap_arg->auth_key);
                dbg("enc_type: %ld\n", setwifiap_arg->enc_type);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";


                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)"", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETWIFIAP send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETWIFIAP send response failed\n");
                }
            }
            break; 
        case DANAVIDEOCMD_SETWIFI:	/** set wifi  **/ 
            {
				anyka_print("\nSetting Wifi ......\n");

                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETWIFI\n");
                DANAVIDEOCMD_SETWIFI_ARG *setwifi_arg = (DANAVIDEOCMD_SETWIFI_ARG *)cmd_arg; 
                dbg("setwifi_arg\n");
                dbg("ch_no: %ld\n", setwifi_arg->ch_no);
                dbg("essid: %s\n", setwifi_arg->essid);
                dbg("auth_key: %s\n", setwifi_arg->auth_key);
                dbg("enc_type: %ld\n", setwifi_arg->enc_type);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
				//char comd[32] = {0};

				
				Psystem_wifi_set_info wifi_ifo = (Psystem_wifi_set_info)malloc(sizeof(system_wifi_set_info));
				memset(wifi_ifo, 0, sizeof(system_wifi_set_info));	

				strcpy(wifi_ifo->ssid, setwifi_arg->essid);
				strcpy(wifi_ifo->passwd, setwifi_arg->auth_key);
				bzero(wifi_ifo->mode, sizeof(wifi_ifo->mode));
				wifi_ifo->enctype = setwifi_arg->enc_type;
				
				anyka_set_sys_wifi_setting(wifi_ifo);		/** set wifi  **/ 

                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)"", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETWIFI send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETWIFI send response failed\n");
                }

				/** check current net mode, if current mode is wire, do not play the tips  **/ 
				if(danavideo_check_current_net_mode() == 0){
					voice_tips_add_music(SYS_CONNECTING_NET);
					/*now use airlink to ser wifi configure . donot have reset,disable it*/
					/*
					sprintf(comd , "%s %s", "/usr/sbin/wifi_manage.sh", "reset");
					anyka_print("Call %s to reset wifi.\n", comd);
	                		system(comd);
	                		*/
					}

				if(wifi_ifo)
					free(wifi_ifo);
			
            }
            break; 
        case DANAVIDEOCMD_STARTAUDIO:		/** start audio **/ 
            {
				anyka_print("[%s:%d] start audio\n", __func__, __LINE__);
		
				dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_STARTAUDIO\n"); 
                DANAVIDEOCMD_STARTAUDIO_ARG *startaudio_arg = (DANAVIDEOCMD_STARTAUDIO_ARG *)cmd_arg;
                dbg("startaudio_arg\n");
                dbg("ch_no: %ld\n", startaudio_arg->ch_no);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
				/** start audio **/ 
                audio_speak_start(mydata, anyka_send_net_audio, SYS_AUDIO_ENCODE_G711);
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)"", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_STARTAUDIO send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_STARTAUDIO send response failed\n");
                }
            }
            break;
        case DANAVIDEOCMD_STARTTALKBACK:	/** start talk **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_STARTTALKBACK\n"); 
                DANAVIDEOCMD_STARTTALKBACK_ARG *starttalkback_arg = (DANAVIDEOCMD_STARTTALKBACK_ARG *)cmd_arg;
                dbg("starttalkback_arg\n");
                dbg("ch_no: %ld\n", starttalkback_arg->ch_no);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                uint32_t audio_codec = G711A;
                if (lib_danavideo_cmd_starttalkback_response(danavideoconn, trans_id, error_code, code_msg, audio_codec)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_STARTTALKBACK send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_STARTTALKBACK send response failed\n");
                }
				/** start talk **/ 
                audio_talk_start((void *)mydata, anyka_get_talk_data, SYS_AUDIO_ENCODE_G711);
                #if 0
                // 寮�鍚鍙栭煶棰戞暟鎹殑绾跨▼
                mydata->run_talkback = true;
                mydata->exit_talback = false;
                if (0 != pthread_create(&(mydata->thread_talkback), NULL, &th_talkback, mydata)) {
                    dbg("TEST danavideoconn_command_handler pthread_create th_talkback failed\n"); 
                } else {
                    pthread_detach(mydata->thread_talkback);
                    dbg("TEST danavideoconn_command_handler th_talkback is started, enjoy!\n");
                }
                #endif
            }
            break; 
        case DANAVIDEOCMD_STARTVIDEO:	/** start video **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMDSTARTVIDEO\n");
                DANAVIDEOCMD_STARTVIDEO_ARG *startvideo_arg = (DANAVIDEOCMD_STARTVIDEO_ARG *)cmd_arg;
                dbg("startvideo_arg\n");
                dbg("ch_no: %ld\n", startvideo_arg->ch_no);
                dbg("client_type: %ld\n", startvideo_arg->client_type);
                dbg("video_quality: %ld\n", startvideo_arg->video_quality);
                dbg("vstrm: %ld\n", startvideo_arg->vstrm);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                mydata->device_type = startvideo_arg->client_type;
                int video_quality = startvideo_arg->video_quality;

				/** start video should stop record at first **/ 
                anyka_dana_record_stop(mydata);
				
				/** set quality **/ 
                if(startvideo_arg->client_type == 1 && video_quality >= 60)
                {
                    video_quality = 59;
                }

                if(startvideo_arg->client_type == 3)
                {
                    if(video_quality <= 50)
                    {
                        video_quality = video_quality / 2;
                        video_quality --;
                    }
                    else 
                    {
                        video_quality -= 10;
                    }
                }
				/** start video  **/ 
                uint32_t start_video_fps = anyka_start_video(video_quality, mydata, anyka_send_net_video);
                if (lib_danavideo_cmd_startvideo_response(danavideoconn, trans_id, error_code, code_msg, start_video_fps)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMDSTARTVIDEO send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMDSTARTVIDEO send response failed\n");
                } 
                usleep(10*1000);
                #if 0
                // 寮�鍚棰戠敓浜ц�呯嚎绂?               mydata->run_media = true;
                mydata->exit_media = false;
                mydata->chan_no = startvideo_arg->ch_no;
                if (0 != pthread_create(&(mydata->thread_media), NULL, &th_media, mydata)) {
                    dbg("TEST danavideoconn_command_handler pthread_create th_media failed\n");
                    // 鍙戦�佹棤娉曞紑鍚棰戠嚎绋嬪埌瀵圭
                } else {
                    pthread_detach(mydata->thread_media);
                    dbg("TEST danavideoconn_command_handler th_media is started, enjoy!\n");
                }
                #endif
                 
            }
            break; 
        case DANAVIDEOCMD_STOPAUDIO:	/**stop audio **/ 
            {
				anyka_print("[%s:%d] stop audio\n", __func__, __LINE__);
				
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_STOPAUDIO\n");
                DANAVIDEOCMD_STOPAUDIO_ARG *stopaudio_arg = (DANAVIDEOCMD_STOPAUDIO_ARG *)cmd_arg;
                dbg("stopaudio_arg\n");
                dbg("ch_no: %ld\n", stopaudio_arg->ch_no);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
				/**stop speek **/ 
                audio_speak_stop(mydata, SYS_AUDIO_ENCODE_G711);
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)"", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_STOPAUDIO send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_STOPAUDIO send response failed\n");
                }
            }
            break; 
        case DANAVIDEOCMD_STOPTALKBACK:	/**stop talk **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_STOPTALKBACK\n");
                DANAVIDEOCMD_STOPTALKBACK_ARG *stoptalkback_arg = (DANAVIDEOCMD_STOPTALKBACK_ARG *)cmd_arg; 
                dbg("stoptalkback_arg\n");
                dbg("ch_no: %ld\n", stoptalkback_arg->ch_no);
                dbg("\n");
                // 鍏抽棴闊抽璇诲彇绾跨▼
                dbg("TEST danavideoconn_command_handler stop th_talkback\n");
                //mydata->run_talkback = false;
                error_code = 0;
                code_msg = (char *)"";
				/**stop talk **/ 
                audio_talk_stop(mydata);
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)"", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_STOPTALKBACK send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_STOPTALKBACK send response failed\n");
                }
            }
            break;
        case DANAVIDEOCMD_STOPVIDEO:	/**stop video **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMDSTOPVIDEO\n");
                DANAVIDEOCMD_STOPVIDEO_ARG *stopvideo_arg = (DANAVIDEOCMD_STOPVIDEO_ARG *)cmd_arg;
                dbg("stopvideo_arg\n");
                dbg("ch_no: %ld\n", stopvideo_arg->ch_no);
                dbg("\n"); 
                // 鍏抽棴瑙嗛鐢熶骇鑰呯嚎绂?               dbg("TEST danavideoconn_command_handler stop th_media\n");
                //mydata->run_media = false;
                error_code = 0;
                code_msg = (char *)"";
				/**stop video **/ 
                anyka_stop_video(mydata);
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)"", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMDSTOPVIDEO send response succeed\n");
                } else {

                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMDSTOPVIDEO send response failed\n");
                } 
            }
            break;
        case DANAVIDEOCMD_RECLIST:	/**get record list **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECLIST\n");
                #if 0
                DANAVIDEOCMD_RECLIST_ARG *reclist_arg = (DANAVIDEOCMD_RECLIST_ARG *)cmd_arg;
                dbg("reclist_arg\n");
                dbg("ch_no: %ld\n", reclist_arg->ch_no);
                if (DANAVIDEO_REC_GET_TYPE_PREV == reclist_arg->get_type) {
                    dbg("get_type: DANAVIDEO_REC_GET_TYPE_PREV\n");
                } else if (DANAVIDEO_REC_GET_TYPE_NEXT == reclist_arg->get_type) {
                    dbg("get_type: DANAVIDEO_REC_GET_TYPE_NEXT\n");
                } else {
                    dbg("Unknown get_type: %ld\n", reclist_arg->get_type);
                }
                dbg("get_num: %ld\n", reclist_arg->get_num);
                dbg("last_time: %ld\n", (unsigned long)reclist_arg->last_time );
                dbg("\n"); 
                error_code = 0;
                code_msg = "";
                int rec_lists_count = 2;
				/**get record list **/ 
                libdanavideo_reclist_recordinfo_t *rec_lists = anyka_get_reclist(reclist_arg, &rec_lists_count);
                //libdanavideo_reclist_recordinfo_t rec_lists[] = {{123, 123, DANAVIDEO_REC_TYPE_NORMAL}, {456, 456, DANAVIDEO_REC_TYPE_ALARM}};
                if (lib_danavideo_cmd_reclist_response(danavideoconn, trans_id, error_code, code_msg, rec_lists_count, rec_lists)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECLIST send response succeed\n");
                } else {

                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECLIST send response failed\n");
                } 
                if(rec_lists)
                {
                    free(rec_lists);
                }
                #endif
            }
            break;
        case DANAVIDEOCMD_RECPLAY:	/**record replay start **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECPLAY\n");
                #if 0
                DANAVIDEOCMD_RECPLAY_ARG *recplay_arg = (DANAVIDEOCMD_RECPLAY_ARG *)cmd_arg;
                dbg("recplay_arg\n");
                dbg("ch_no: %ld\n", recplay_arg->ch_no);
                dbg("time_stamp: %ld\n", recplay_arg->time_stamp);
                dbg("\n"); 
                error_code = 0;
                code_msg = "";
				/**record replay **/ 
                anyka_dana_record_play(recplay_arg->time_stamp, mydata);
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, "", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECPLAY send response succeed\n");
                } else {

                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECPLAY send response failed\n");
                } 
                #endif
            }
            break;
         case DANAVIDEOCMD_RECSTOP:	/**record replay stop**/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECSTOP\n");
                DANAVIDEOCMD_RECSTOP_ARG *recstop_arg = (DANAVIDEOCMD_RECSTOP_ARG *)cmd_arg;
                dbg("recstop_arg\n");
                dbg("ch_no: %ld\n", recstop_arg->ch_no);
                dbg("\n"); 
                error_code = 0;
                code_msg = "";
				/**record replay stop**/ 
                anyka_dana_record_stop(mydata);
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, "", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECSTOP send response succeed\n");
                } else {

                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECSTOP send response failed\n");
                }
            } 
            break;
        case DANAVIDEOCMD_RECACTION:	/**record replay pause or go on **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECACTION\n");
                DANAVIDEOCMD_RECACTION_ARG *recaction_arg = (DANAVIDEOCMD_RECACTION_ARG *)cmd_arg;
                dbg("recaction_arg\n");
                dbg("ch_no: %ld\n", recaction_arg->ch_no);
                if (DANAVIDEO_REC_ACTION_PAUSE == recaction_arg->action) {
                    dbg("action: DANAVIDEO_REC_ACTION_PAUSE\n");
                    anyka_dana_record_play_pause(mydata, 0);	/**record replay pause **/ 
                } else if (DANAVIDEO_REC_ACTION_PLAY == recaction_arg->action) {
                    dbg("action: DANAVIDEO_REC_ACTION_PLAY\n");
                    anyka_dana_record_play_pause(mydata, 1);	/**record replay go on **/ 
                } else {
                    dbg("Unknown action: %ld\n", recaction_arg->action);
                }
                dbg("\n"); 
                error_code = 0;
                code_msg = "";
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, "", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECACTION send response succeed\n");
                } else {

                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECACTION send response failed\n");
                } 
            }
            break;
        case DANAVIDEOCMD_RECSETRATE:	/**record replay set rates **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECSETRATE\n");
                DANAVIDEOCMD_RECSETRATE_ARG *recsetrate_arg = (DANAVIDEOCMD_RECSETRATE_ARG *)cmd_arg;
                dbg("recsetrate_arg\n");
                dbg("ch_no: %ld\n", recsetrate_arg->ch_no);
                if (DANAVIDEO_REC_RATE_HALF == recsetrate_arg->rec_rate) {
                    dbg("rec_rate: DANAVIDEO_REC_RATE_HALF\n");
                } else if (DANAVIDEO_REC_RATE_NORMAL == recsetrate_arg->rec_rate) {
                    dbg("rec_rate: DANAVIDEO_REC_RATE_NORMAL\n");
                } else if (DANAVIDEO_REC_RATE_DOUBLE == recsetrate_arg->rec_rate) {
                    dbg("rec_rate: DANAVIDEO_REC_RATE_DOUBLE\n");
                } else if (DANAVIDEO_REC_RATE_FOUR == recsetrate_arg->rec_rate) {
                    dbg("rec_rate: DANAVIDEO_REC_RATE_FOUR\n");
                } else {
                    dbg("Unknown rec_rate: %ld\n", recsetrate_arg->rec_rate);
                }
                dbg("\n"); 
                error_code = 0;
                code_msg = "";
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, "", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECSETRATE send response succeed\n");
                } else {

                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECSETRATE send response failed\n");
                } 
            }
            break;
         case DANAVIDEOCMD_RECPLANGET:	/**record replay get plan **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECPLANGET\n");
                DANAVIDEOCMD_RECPLANGET_ARG *recplanget_arg = (DANAVIDEOCMD_RECPLANGET_ARG *)cmd_arg;
                dbg("recplanget_arg\n");
                dbg("ch_no: %ld\n", recplanget_arg->ch_no);
                dbg("\n"); 
                error_code = 0;
                code_msg = "";
                uint32_t rec_plans_count;
				/**record replay get plan **/ 
                libdanavideo_recplanget_recplan_t *rec_plans = dana_get_record_plan(&rec_plans_count);
                if (lib_danavideo_cmd_recplanget_response(danavideoconn, trans_id, error_code, code_msg, rec_plans_count, rec_plans)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECPLANGET send response succeed\n");
                } else {

                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECPLANGET send response failed\n");
                }
                free(rec_plans);
            } 
            break;
        case DANAVIDEOCMD_RECPLANSET:	/**record replay set plan **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECPLANSET\n");
                DANAVIDEOCMD_RECPLANSET_ARG *recplanset_arg = (DANAVIDEOCMD_RECPLANSET_ARG *)cmd_arg;
                dbg("recplanset_arg\n");
                dbg("ch_no: %ld\n", recplanset_arg->ch_no);
                dbg("record_no: %ld\n", recplanset_arg->record_no);
                size_t i; 
                for (i=0; i<recplanset_arg->week_count; i++) {
                    if (DANAVIDEO_REC_WEEK_MON == recplanset_arg->week[i]) {
                        dbg("week: DANAVIDEO_REC_WEEK_MON\n");
                    } else if (DANAVIDEO_REC_WEEK_TUE == recplanset_arg->week[i]) {
                        dbg("week: DANAVIDEO_REC_WEEK_TUE\n");
                    } else if (DANAVIDEO_REC_WEEK_WED == recplanset_arg->week[i]) {
                        dbg("week: DANAVIDEO_REC_WEEK_WED\n");
                    } else if (DANAVIDEO_REC_WEEK_THU == recplanset_arg->week[i]) {
                        dbg("week: DANAVIDEO_REC_WEEK_THU\n");
                    } else if (DANAVIDEO_REC_WEEK_FRI == recplanset_arg->week[i]) {
                        dbg("week: DANAVIDEO_REC_WEEK_FRI\n");
                    } else if (DANAVIDEO_REC_WEEK_SAT == recplanset_arg->week[i]) {
                        dbg("week: DANAVIDEO_REC_WEEK_SAT\n");
                    } else if (DANAVIDEO_REC_WEEK_SUN == recplanset_arg->week[i]) {
                        dbg("week: DANAVIDEO_REC_WEEK_SUN\n");
                    } else {
                        dbg("Unknown week: %ld\n", recplanset_arg->week[i]);
                    } 
                } 
                dbg("start_time: %s\n", recplanset_arg->start_time);
                dbg("end_time: %s\n", recplanset_arg->end_time);
                dbg("status: %ld\n", recplanset_arg->status);
                dbg("\n"); 
                dana_set_record_plan(recplanset_arg);	/**record replay set plan **/ 
                error_code = 0;
                code_msg = "";
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, "", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECPLANSET send response succeed\n");
                } else {

                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECPLANSET send response failed\n");
                } 
            }
            break;
        case DANAVIDEOCMD_EXTENDMETHOD:		/**extend method **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_EXTENDMETHOD\n");
                DANAVIDEOCMD_EXTENDMETHOD_ARG *extendmethod_arg = (DANAVIDEOCMD_EXTENDMETHOD_ARG *)cmd_arg;
                dbg("extendmethod_arg\n");
//                dbg("extend_no: %ld\n", extendmethod_arg->extend_no);
                dbg("extend_data_size: %zd\n", extendmethod_arg->extend_data.size);
                // extend_data_bytes access via extendmethod_arg->extend_data.bytes 
                dbg("\n"); 
                error_code = 0;
                code_msg = "";
                // 鍙戦�佹暟鎷縏ODO
            } 
            break;
        case DANAVIDEOCMD_SETOSD:	/**set osd **/ 
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETOSD\n");
                DANAVIDEOCMD_SETOSD_ARG *setosd_arg = (DANAVIDEOCMD_SETOSD_ARG *)cmd_arg;
                dbg("ch_no: %ld\n", setosd_arg->ch_no);
                dbg("osd_info:\n");
                if (DANAVIDEO_OSD_SHOW_OPEN == setosd_arg->osd.chan_name_show) {
                    dbg("chan_name_show OPEN\n");
                    if (setosd_arg->osd.has_show_name_x) {
                        dbg("show_name_x: %ld\n", setosd_arg->osd.show_name_x);
                    }
                    if (setosd_arg->osd.has_show_name_y) {
                        dbg("show_name_y: %ld\n", setosd_arg->osd.show_name_y);
                    } 
                } else if (DANAVIDEO_OSD_SHOW_CLOSE == setosd_arg->osd.chan_name_show) {
                    dbg("chan_name_show CLOSE\n");
                } else {
                    dbg("chan_name_show unknown type[%ld]\n", setosd_arg->osd.chan_name_show);
                }
                if (DANAVIDEO_OSD_SHOW_OPEN == setosd_arg->osd.datetime_show) {
                    dbg("datetime_show OPEN\n");
                    if (setosd_arg->osd.has_show_datetime_x) {
                        dbg("show_datetime_x: %ld\n", setosd_arg->osd.show_datetime_x);
                    }
                    if (setosd_arg->osd.has_show_datetime_y) {
                        dbg("show_datetime_y: %ld\n", setosd_arg->osd.show_datetime_y);
                    }
                    if (setosd_arg->osd.has_show_format) {
                        dbg("show_format:\n");

						/**set osd display format **/ 
                        switch (setosd_arg->osd.show_format) {
                            case DANAVIDEO_OSD_DATE_FORMAT_YYYY_MM_DD:	//year-mon-day
                                dbg("DANAVIDEO_OSD_DATE_FORMAT_YYYY_MM_DD\n");
                                break;
                            case DANAVIDEO_OSD_DATE_FORMAT_MM_DD_YYYY:	//mon-day-year
                                dbg("DANAVIDEO_OSD_DATE_FORMAT_MM_DD_YYYY\n");
                                break;
                            case DANAVIDEO_OSD_DATE_FORMAT_YYYY_MM_DD_CH:	//chinese year-mon-day
                                dbg("DANAVIDEO_OSD_DATE_FORMAT_YYYY_MM_DD_CH\n");
                                break;
                            case DANAVIDEO_OSD_DATE_FORMAT_MM_DD_YYYY_CH:	//chinese mon-day-year
                                dbg("DANAVIDEO_OSD_DATE_FORMAT_MM_DD_YYYY_CH\n");
                                break;
                            case DANAVIDEO_OSD_DATE_FORMAT_DD_MM_YYYY:	//day-mon-year
                                dbg("DANAVIDEO_OSD_DATE_FORMAT_DD_MM_YYYY\n");
                                break;
                            case DANAVIDEO_OSD_DATE_FORMAT_DD_MM_YYYY_CH:	//chinese day-mon-year
                                dbg("DANAVIDEO_OSD_DATE_FORMAT_DD_MM_YYYY_CH\n");
                                break;
                            default:
                                dbg("DANAVIDEO_OSD_DATE_FORMAT_XXXX\n");
                                break;
                        }
                    }
                    if (setosd_arg->osd.has_hour_format) {
                        dbg("hour_format:\n");
                        switch (setosd_arg->osd.hour_format) {
                            case DANAVIDEO_OSD_TIME_24_HOUR:	//24 hour format 
                                dbg("DANAVIDEO_OSD_TIME_24_HOUR\n");
                                break;
                            case DANAVIDEO_OSD_TIME_12_HOUR:	//12 hour format
                                dbg("DANAVIDEO_OSD_TIME_12_HOUR\n");
                                break;
                            default:
                                dbg("DANAVIDEO_OSD_TIME_XXXX\n");
                                break;
                        }
                    }
                    if (setosd_arg->osd.has_show_week) {
                        dbg("show_week:\n");
                        switch (setosd_arg->osd.show_week) {
                            case DANAVIDEO_OSD_SHOW_CLOSE:	//do not show weekens 
                                dbg("DANAVIDEO_OSD_SHOW_CLOSE\n");
                                break;
                            case DANAVIDEO_OSD_SHOW_OPEN:	//show weekens 
                                dbg("DANAVIDEO_OSD_SHOW_OPEN\n");
                                break;
                            default:
                                dbg("DANAVIDEO_OSD_SHOW_XXXX\n");
                                break;
                        }
                    }
                    if (setosd_arg->osd.has_datetime_attr) {
                        dbg("datetime_attr:\n");
                        switch (setosd_arg->osd.datetime_attr) {
                            case DANAVIDEO_OSD_DATETIME_TRANSPARENT:
                                dbg("DANAVIDEO_OSD_DATETIME_TRANSPARENT\n");
                                break;
                            case DANAVIDEO_OSD_DATETIME_DISPLAY:
                                dbg("DANAVIDEO_OSD_DATETIME_DISPLAY\n");
                                break;
                            default:
                                dbg("DANAVIDEO_OSD_DATETIME_XXXX\n");
                                break;
                        }
                    }
                } 
				/* does not show osd */
				else if (DANAVIDEO_OSD_SHOW_CLOSE == setosd_arg->osd.datetime_show) {
                    dbg("datetime_show CLOSE\n");
                } else {
                    dbg("datetime_show unknown type[%ld]\n", setosd_arg->osd.datetime_show);
                }
				/* show osd */
                if (DANAVIDEO_OSD_SHOW_OPEN == setosd_arg->osd.custom1_show) {
                    dbg("custom1_show OPEN\n");
                    if (setosd_arg->osd.has_show_custom1_str) {
                        dbg("show_custom1_str: %s\n", setosd_arg->osd.show_custom1_str);
                    }
                    if (setosd_arg->osd.has_show_custom1_x) {
                        dbg("show_custom1_x: %ld\n", setosd_arg->osd.show_custom1_x);
                    }
                    if (setosd_arg->osd.has_show_custom1_y) {
                        dbg("show_custom1_y: %ld\n", setosd_arg->osd.show_custom1_y);
                    }
                } else if (DANAVIDEO_OSD_SHOW_CLOSE == setosd_arg->osd.custom1_show) {
                    dbg("custom1_show CLOSE\n");
                } else {
                    dbg("custom1_show unknown type[%ld]\n", setosd_arg->osd.custom1_show);
                }
                if (DANAVIDEO_OSD_SHOW_OPEN == setosd_arg->osd.custom2_show) {
                    dbg("custom2_show OPEN\n");
                    if (setosd_arg->osd.has_show_custom2_str) {
                        dbg("show_custom2_str: %s\n", setosd_arg->osd.show_custom2_str);
                    }
                    if (setosd_arg->osd.has_show_custom2_x) {
                        dbg("show_custom2_x: %ld\n", setosd_arg->osd.show_custom2_x);
                    }
                    if (setosd_arg->osd.has_show_custom2_y) {
                        dbg("show_custom2_y: %ld\n", setosd_arg->osd.show_custom2_y);
                    }
                } else if (DANAVIDEO_OSD_SHOW_CLOSE == setosd_arg->osd.custom2_show) {
                    dbg("custom2_show CLOSE\n");
                } else {
                    dbg("custom2_show unknown type[%ld]\n", setosd_arg->osd.custom2_show);
                }
                //
                anyka_set_osd_info(setosd_arg);
                error_code = 0;
                code_msg = "";
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, "", trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETOSD send response succeeded\n");
                } else {
        
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETOSD send response failed\n");
                }
            }
            break;
        case DANAVIDEOCMD_GETOSD:	/* get osd message */
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETOSD\n");
                DANAVIDEOCMD_GETOSD_ARG *getosd_arg = (DANAVIDEOCMD_GETOSD_ARG *)cmd_arg;
                dbg("ch_no: %ld\n", getosd_arg->ch_no);
                dbg("\n");
                error_code = 0;
                code_msg = (char *)"";
                libdanavideo_osdinfo_t osdinfo;
				/* get osd message from config file */
                anyka_get_osd_info(&osdinfo);
                #if 0
                osdinfo.chan_name_show = DANAVIDEO_OSD_SHOW_OPEN;
                osdinfo.show_name_x = 1;
                osdinfo.show_name_y = 2;
        
                osdinfo.datetime_show = DANAVIDEO_OSD_SHOW_OPEN;
                osdinfo.show_datetime_x = 3;
                osdinfo.show_datetime_y = 4;
                osdinfo.show_format = DANAVIDEO_OSD_DATE_FORMAT_YYYY_MM_DD_CH;
                osdinfo.hour_format = DANAVIDEO_OSD_TIME_24_HOUR;
                osdinfo.show_week = DANAVIDEO_OSD_SHOW_OPEN;
                osdinfo.datetime_attr = DANAVIDEO_OSD_DATETIME_DISPLAY;
        
                osdinfo.custom1_show = DANAVIDEO_OSD_SHOW_OPEN;
                strncpy(osdinfo.show_custom1_str, "show_custom1_str", sizeof(osdinfo.show_custom1_str) -1);
                osdinfo.show_custom1_x = 5;
                osdinfo.show_custom1_y = 6;
        
                osdinfo.custom2_show = DANAVIDEO_OSD_SHOW_OPEN;
                strncpy(osdinfo.show_custom2_str, "show_custom2_str", sizeof(osdinfo.show_custom2_str) -1);
                osdinfo.show_custom2_x = 7;
                osdinfo.show_custom2_y = 8;
                #endif
                if (lib_danavideo_cmd_getosd_response(danavideoconn, trans_id, error_code, code_msg, &osdinfo)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETOSD send response succeeded\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETOSD send response failed\n");
                }
            }
            break;
        case DANAVIDEOCMD_SETCHANNAME:	/* get channal name */
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECPLANGET\n");
                DANAVIDEOCMD_SETCHANNAME_ARG *setchanname_arg = (DANAVIDEOCMD_SETCHANNAME_ARG *)cmd_arg;
                dbg("setchanname_arg\n");
                dbg("ch_no: %d\n", setchanname_arg->ch_no);
                anyka_print("chan_name: %s\n", setchanname_arg->chan_name);
                dbg("\n"); 
                error_code = 0;
                code_msg = "";

                Pcamera_disp_setting camera_info;
                camera_info = anyka_get_camera_info();	/* get camera info */
                strncpy(camera_info->osd_name, setchanname_arg->chan_name, 49);
                anyka_set_camera_info(camera_info);	/* set camera info */
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)cmd_arg, trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETCHANNAME send response succeeded\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_SETCHANNAME send response failed\n");
                }
            } 
            break;
        case DANAVIDEOCMD_GETCHANNAME:	/* set channal name */
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RECPLANGET\n");
                DANAVIDEOCMD_GETCHANNAME_ARG *setchanname_arg = (DANAVIDEOCMD_GETCHANNAME_ARG *)cmd_arg;
                dbg("setchanname_arg\n");
                dbg("ch_no: %d\n", setchanname_arg->ch_no);
                dbg("\n"); 
                error_code = 0;
                code_msg = "";
                Pcamera_disp_setting camera_info;
                camera_info = anyka_get_camera_info();	/* get camera info */
        
                char *chan_name = camera_info->osd_name;
                if (lib_danavideo_cmd_getchanname_response(danavideoconn, trans_id, error_code, code_msg, chan_name)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETCHANNAME send response succeeded\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_GETCHANNAME send response failed\n");
                }
            } 
            break;
        case DANAVIDEOCMD_RESOLVECMDFAILED:	/* cmd err */
            {
                dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RESOLVECMDFAILED\n");
                // 鏍规嵁涓嶅悓鐨刴ethod,璋冪敤lib_danavideo_cmd_response
                error_code = 20145;
                code_msg = (char *)"danavideocmd_resolvecmdfailed";
                if (lib_danavideo_cmd_exec_response(danavideoconn, cmd, (char *)cmd_arg, trans_id, error_code, code_msg)) {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RESOLVECMDFAILED send response succeed\n");
                } else {
                    dbg("TEST danavideoconn_command_handler DANAVIDEOCMD_RESOLVECMDFAILED send response failed\n");
                }
                break;
                default:
                dbg("TEST danavideoconn_command_handler UnKnown CMD: %ld\n", cmd);
            }
            break;
    }

    return;
}

dana_video_callback_funs_t danavideocallbackfuns = {
    .danavideoconn_created = danavideoconn_created,
    .danavideoconn_aborted = danavideoconn_aborted,
    .danavideoconn_command_handler = danavideoconn_command_handler,
};

/**
* @brief 	danavideo_hb_is_ok
* 			大拿心跳包检测OK
* @date 	2015/3
* @param:	void 
* @return 	void 
* @retval 	
*/

void danavideo_hb_is_ok(void)
{
    dana_hb_time = time(0);
    dbg("THE CONN TO P2P SERVER IS OK:(%ld)\n",dana_hb_time);
	if(online_status != 1){
		voice_tips_add_music(SYS_ONLINE);
		online_status = 1;
	}
}

/**
* @brief 	danavideo_hb_is_not_ok
* 			大拿心跳包检测失败
* @date 	2015/3
* @param:	void 
* @return 	void 
* @retval 	
*/

void danavideo_hb_is_not_ok(void) 
{
    dbg("THE CONN TO P2P SERVER IS NOT OK\n");
}

/**
* @brief 	danavideo_hb_is_not_ok
* 			固件升级
* @date 	2015/3
* @param:	 const char* rom_path，ftp服务器地址
			const char *rom_md5，映像文件MD5
			const uint32_t rom_size， 映像文件大小
* @return 	 void
* @retval 	
*/
void danavideo_upgrade_rom(const char* rom_path,  const char *rom_md5, const uint32_t rom_size)
{
    dbg("NEED UPGRADE ROM rom_path: %s\trom_md5: %s\trom_size: %ld\n", rom_path, rom_md5, rom_size);

	/***** get rom by ftp *****/
	char cmd[100] = {0};		//commond buf

	//change directory to /tmp
	chdir("/tmp");
	
	/***** ftpget ip filename *****/
	sprintf(cmd, "%s %s %s", "ftpget", rom_path, "rom_name.tar.bz2");
	system(cmd);
	bzero(cmd, sizeof(cmd));
	
	/***** tar rom *****/
	//解压文件
	sprintf(cmd, "%s %s %s", "tar", "xvf", "rom_name.tar.bz2");
	system(cmd);
	bzero(cmd, sizeof(cmd));
	
	/******* 调用升级脚本 *******/
	system("/usr/sbin/update.sh");

	return ;
}


/**
* @brief 	danavideo_autoset
* 			自动设置信息
* @date 	2015/3
* @param:	 const uint32_t power_freq，电源频率
			const int64_t now_time， 当前时间
			const char *time_zone，根据ip获取的时区
			const char *ntp_server1, ntp 服务器1 地址
			const char *ntp_server2, ntp 服务器2 地址
* @return 	 void
* @retval 	
*/

void danavideo_autoset(const uint32_t power_freq, const int64_t now_time, const char *time_zone, const char *ntp_server1, const char *ntp_server2)
{
    dbg("AUTOSET\n\tpower_freq: %ld\n\tnow_time: %ld\n\ttime_zone: %s\n\tntp_server1: %s\n\tntp_server2: %s\n", power_freq, now_time, time_zone, ntp_server1, ntp_server2);
	
	char cmd[256] = {0};	//cmd buffer
	int hour = 0, min = 0;
	time_t now;
	now = now_time;
	struct tm *now_tm;
	now_tm = gmtime(&now);
	if(now_tm == NULL)
	{
		anyka_print("[%s:%d] get local time error\n", __func__, __LINE__);
		return;	
	}
    if(time_zone)
    {
        //sscanf(ntp_server1+4, "%d:%d", &hour, &min);
        hour = atoi(time_zone+4);
        min = atoi(time_zone+7);
        dana_time_zone = hour * 3600 + min * 60;
        if(time_zone[3]!='+')
        {
            dana_time_zone = 0 - dana_time_zone;
            hour = 0 - hour;
            min = 0 - min;
        }
    }
	/** construc cmd: date year-mon-mday hour:min:sec **/
	sprintf(cmd, "date \"%d-%d-%d %d:%d:%d\"", now_tm->tm_year+1900, now_tm->tm_mon+1, now_tm->tm_mday,
						now_tm->tm_hour+hour, now_tm->tm_min + min, now_tm->tm_sec);
	anyka_print("[%s:%d]set system time :%s\n", __func__, __LINE__, cmd);
	system(cmd);

    //save to rtc
   	bzero(cmd, sizeof(cmd));  //clean buffer
	sprintf(cmd, "hwclock -w");
	if(system(cmd) < 0)
	{	
		anyka_print("[%s:%d] save system time to RTC failed.\n", __func__, __LINE__);			
	}
	
	return;
}

/**
* @brief 	danavideo_match_ssid_pswd
* 			匹配当前wifi 账户和密码
* @date 	2015/3
* @param:	 const char * essid，账户-->ssid
			const char * auth_key， 密码
* @return 	 返回比较的值
* @retval 	两者都相同返回0，否则非0
*/

int danavideo_match_ssid_pswd(const char * essid, const char * auth_key)
{
	Psystem_wifi_set_info wifi_info = anyka_get_sys_wifi_setting();
	
	return (strcmp(wifi_info->ssid, essid) || strcmp(wifi_info->passwd, auth_key));
}


/**
* @brief save_ssid_to_tmp
* 		 save the ssid to tmp file
* @author cyh
* @date 2016-03-09
* @param[in]  ssid
* @return int
* @retval if return 0 success, otherwise failed
*/

static int save_ssid_to_tmp(char *ssid, unsigned int ssid_len)
{
	int ret;
	char gbk_exist = 0, utf8_exist = 0;
	char gbk_path_name[32] = {0}; //path name
	char utf8_path_name[32] = {0};
	char gbk_ssid[32] = {0}; //store ssid after change encode func

	sprintf(gbk_path_name, "%s/gbk_ssid", WIRE_LESS_TMP_DIR);
	sprintf(utf8_path_name, "%s/utf8_ssid", WIRE_LESS_TMP_DIR);

	ret = mkdir(WIRE_LESS_TMP_DIR, O_CREAT | 0666);
	if(ret < 0) {
		if(errno == EEXIST)	{
			printf("[%s:%d] the %s exist\n",
				   	__func__, __LINE__, WIRE_LESS_TMP_DIR);

			/** check file exist **/
			if(access(gbk_path_name, F_OK) == 0)
				gbk_exist = 1;
			
			if(access(utf8_path_name, F_OK) == 0)
				utf8_exist = 1;	

			if(gbk_exist && utf8_exist) {
				printf("[%s:%d] the wireless config has been saved," 
					"now do nothing\n", __func__, __LINE__);
				return 0;
			}
			
		} else {
			printf("[%s:%d] make directory %s, %s\n", __func__,
				   	__LINE__, WIRE_LESS_TMP_DIR, strerror(errno));
			return -1;
		}
	}

	if(utf8_exist == 0) {

		FILE *filp_utf8 = fopen(utf8_path_name, "w+");	
		if(!filp_utf8) {
			printf("[%s:%d] open: %s, %s\n", 
					__func__, __LINE__, utf8_path_name, strerror(errno));
			return -1;
		}

		ret = fwrite(ssid, 1, ssid_len + 1, filp_utf8);
		if(ret != ssid_len + 1) {
			printf("[%s:%d] fails write data\n", __func__, __LINE__);
			fclose(filp_utf8);
			return -1;
		}
		fclose(filp_utf8);
	}

	if(gbk_exist == 0) {
		
		/** utf-8 to gbk code change **/
		ret = u2g(ssid, ssid_len, gbk_ssid, 32);
		if(ret < 0) {
			printf("[%s:%d] faile to change code from utf8 to gbk\n", 
				__func__, __LINE__);
			return -1;
		}
		printf("*** u2g changed[%d], %s\n", ret, gbk_ssid);

		FILE *filp_gbk = fopen(gbk_path_name, "w+");
		if(!filp_gbk) {
			printf("[%s:%d] open: %s, %s\n", 
					__func__, __LINE__, gbk_path_name, strerror(errno));
			return -1;
		}
		
		ret = fwrite(gbk_ssid, 1, strlen(gbk_ssid) + 1, filp_gbk);
		if(ret != strlen(gbk_ssid) + 1) {
			printf("[%s:%d] fails write data\n", __func__, __LINE__);
			fclose(filp_gbk);
			return -1;
		}
		fclose(filp_gbk);

	}
	
	return 0;
}

/**
* @brief 	danavideo_setwifiap
* 			设置wifi 
* @date 	2015/3
* @param:	 const uint32_t ch_no，通道
			const uint32_t ip_type，ip 类型
			const char *ipaddr，ip 地址
			const char *netmask，子网掩码
			const char *gateway,  	网关
			const char *dns_name1, 	dns 1
			const char *dns_name2, 	dns 2
			const char *essid,		ssid
			const char *auth_key, 	passwd
			const uint32_t enc_type	加密类型
* @return 	void
* @retval 	
*/
#if 0
void danavideo_setwifiap(const uint32_t ch_no, const uint32_t ip_type, const char *ipaddr, const char *netmask, const char *gateway, const char *dns_name1, const char *dns_name2, const char *essid, const char *auth_key, const uint32_t enc_type)
{
	dbg("[%s:%d] enter \n", __func__, __LINE__);
    dbg("SETWIFIAP\n\tch_no: %ld\n\tip_type: %ld\n\tipaddr: %s\n\tnetmask: %s\n"
			"gateway: %s\n\tdns_name1: %s\n\tdns_name2: %s\n\tessid: %s\n\tauth_key: %s\n\tenc_type: %ld\n", 
			ch_no, ip_type, ipaddr, netmask, gateway, dns_name1, dns_name2, essid, auth_key, enc_type);

	/**** 调用设置wifi函数保存设置到本地.ini文件上***/
	anyka_print("[%s:%d] ssid:%s,authkey:%s, enctype : %d\n", __func__, __LINE__, essid, auth_key, enc_type);
	char comd[32] = {0};

	/** 先判断要设置的和当前是是否一致，一致则不修改 **/
	if(danavideo_match_ssid_pswd(essid, auth_key) == 0){
		anyka_print("[%s:%d] the ssid and password is same, do nothing with it.\n", __func__, __LINE__);
		return;
	}
	
	smart_run_flag = 0;
	voice_tips_add_music(SYS_GET_CONFIG); 	//get config 

	Psystem_wifi_set_info user =(Psystem_wifi_set_info)malloc(sizeof(system_wifi_set_info));
	user->enctype = enc_type;
	sprintf(user->mode, "%s", "Infra");
	strcpy(user->ssid, essid);
	strcpy(user->passwd, auth_key);
	
	anyka_set_sys_wifi_setting(user);
	
	if(	danavideo_check_current_net_mode() > 0)		//有线状态不允许修改无线
		return;
	anyka_print("set wifi ok, reset wifi device!\n");
	sprintf(comd , "%s %s", "/usr/sbin/wifi_manage.sh", "reset");
	anyka_print("Call %s to reset wifi.\n", comd);
	voice_tips_add_music(SYS_CONNECTING_NET);		
    system(comd);

	online_status = 0;		//play the online tone
 
}
#else
/* 20160309 set wifi configure by airlink */
void danavideo_setwifiap(const uint32_t ch_no, const uint32_t ip_type, const char *ipaddr, const char *netmask, const char *gateway, const char *dns_name1, const char *dns_name2, const char *essid, const char *auth_key, const uint32_t enc_type)
{
    
  	
    dbg("SETWIFIAP\n\tch_no: %ld\n\tip_type: %ld\n\tipaddr: %s\n\tnetmask: %s\n"
			"gateway: %s\n\tdns_name1: %s\n\tdns_name2: %s\n\tessid: %s\n\tauth_key: %s\n\tenc_type: %ld\n", 
			ch_no, ip_type, ipaddr, netmask, gateway, dns_name1, dns_name2, essid, auth_key, enc_type);

	  /**** 调用设置wifi函数保存设置到本地.ini文件上***/
	  anyka_print("[%s:%d] ssid:%s,authkey:%s, enctype : %d\n", __func__, __LINE__, essid, auth_key, enc_type);
	 

	  /** 先判断要设置的和当前是是否一致，一致则不修改 **/
	  if(danavideo_match_ssid_pswd(essid, auth_key) == 0){
	      dana_airlink_called = true; // 通知主线程配置完成
		  anyka_print("[%s:%d] the ssid and password is same, do nothing with it.\n", __func__, __LINE__);
	  	return;
	  }
		  	  	
	  if(	danavideo_check_current_net_mode() > 0)		//有线状态不允许修改无线
		  return;
	  
	  Psystem_wifi_set_info wifi_info = anyka_get_sys_wifi_setting();
	  strncpy(wifi_info->passwd, auth_key, 31);
	  anyka_set_sys_wifi_setting(wifi_info);
	  
	  
	  if(save_ssid_to_tmp((char *)essid, strlen(essid))) {
		    anyka_print("[airlink] fails to save ssid to tmp\n");
		}
      dbg("配置WiFi...\n");
      //sleep(3);
      dana_airlink_called = true; // 通知主线程配置完成
}

#endif



/**
* @brief 	dana_airlink_thread
			dana set wifi by airlink
* @date 	2016/3
* @param:	void* param
* @return 	void*
* @retval 	null 
*/
static void* dana_airlink_thread(void* param)
{
	  // 测试DanaAirLink
    // DanaAirLink
    // 目前支持MT7601, RTL8188, RT3070等芯片,如果采用的是其他芯片可以联系Danale
    // 某些芯片需要采用大拿提供的驱动
    // 需要lib_danavideo_set_local_setwifiap注册配置Wi-Fi回调函数
    const char *danale_path = (const char *)param;    
    char *if_name = "wlan0";
	struct ifreq ifr;
  	struct sockaddr_in *sai = NULL;

	struct stat st_buf;
	char *chk_dir = "/tmp/wireless";
  	
  	int sockfd;	
  	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
 	bzero(&ifr,sizeof(struct ifreq));
  	strcpy(ifr.ifr_name,if_name);
        
    anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());
    if (!danaairlink_set_danalepath(danale_path)) {
        dbg("testdanavideo danaairlink_set_danalepath failed\n");        
        return NULL;
    }
    if (!danaairlink_init(DANAAIRLINK_CHIP_TYPE_RLT8188, if_name)) {
        dbg("testdanavideo danaairlink_set_danalepath failed\n");        
        return NULL;
    }
    
    dbg("testdanavideo danaairlink_init succeeded\n");    
    if (!danaairlink_start_once()) {
        dbg("testdanavideo danaairlink_start_once failed\n");
        danaairlink_deinit();
        return NULL;
    }
    
    dbg("testdanavideo danaairlink_start_once succeeded\n");
	  
    while (1) {
        if (dana_airlink_called) {
			/*wait udhcpc to configure wifi-net */
			sleep(10);
						
			/*if ip is not configure , ioctl(sockfd, SIOCGIFADDR, &ifr) return fail*/
            if(ioctl(sockfd, SIOCGIFADDR, &ifr) >=0){
				sai = (struct sockaddr_in *) &ifr.ifr_addr;
				anyka_print("[%s] %s ip:%s \n ", __func__, if_name, inet_ntoa(sai->sin_addr));
	   			if(strlen(inet_ntoa(sai->sin_addr)) > 6){
					
	   			 	break;
            	} 
            }
            	// at first to check wifi ssid file exist? and then redo start airlink
            if(stat(chk_dir, &st_buf) < 0){ 
				
				/*wait script  reload airlink wifi mod */
				sleep(3);
				if ((ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) || 
					(!(ifr.ifr_flags & IFF_RUNNING))) {
					anyka_print(" [%s] get ifflags fail or  not RUNNING.\n ", __func__);
				    continue;
				}
		
				if(!danaairlink_start_once()) {
                	anyka_err("danaairlink_start_once failed\n");
                	danaairlink_deinit();
					close(sockfd);
                	return NULL;
            	}
				
            	dana_airlink_called = false;
            	
            }
			 
        } else {
        	//anyka_print(" %s %d \n ",__FUNCTION__,__LINE__);
            sleep(2);
        }
    }
	close(sockfd);
    dana_airlink_called = false;
    // danaairlink_stop停止配置
    //danaairlink_deinit清理资源
    danaairlink_stop();       
    danaairlink_deinit();        
    
    return NULL;
}


/**
* @brief 	set_wifi_by_airlink
* 		use dana airlink to set wifi configure
* @date 	2016/3
* @param:	void *param
			
* @return 	void
* @retval 	
*/

static void set_wifi_by_airlink(void* param)
{
	Psystem_wifi_set_info p_wifi_info;    
    pthread_t airlink_thread_id;
    
    p_wifi_info = anyka_get_sys_wifi_setting();
    
    if (0 != strlen(p_wifi_info->ssid)){
    	  anyka_print("[%s:%d] wifi ssid:%s \n", 
		  	__func__, __LINE__, p_wifi_info->ssid);
    	  return;
    }
	anyka_pthread_create(&airlink_thread_id, dana_airlink_thread, param, ANYKA_THREAD_MIN_STACK_SIZE, -1);
}
/**
* @brief 	danavideo_local_auth_callback
* 			本地登录校验账户密码
* @date 	2015/3
* @param:	const char *user_name，本地登录使用的账户
			const char *user_pass，本地登录使用的密码
* @return 	uint32_t
* @retval 	0 -- > success,  1 --> failed 
*/

uint32_t danavideo_local_auth_callback(const char *user_name, const char *user_pass)
{
    
    system_user_info * sys_user = anyka_get_system_user_info();
    if((strcmp(user_name, sys_user->user) == 0) && (strcmp(user_pass, sys_user->secret) == 0))
    {
        return 0;
    }
    else
    {
        anyka_print("###### user and secret is error:name=%s, pwd=%s ####\n", user_name, user_pass);
        return 1;
    }
    
}



/**
* @brief 	danavideo_audio_add_callback
* 			添加智能声控回调
* @date 	2015/3
* @param:	void *param, 参数
			T_STM_STRUCT * pstream_data，音频数据
* @return 	void
* @retval 	
*/

void danavideo_audio_add_callback(void *param, T_STM_STRUCT * pstream_data)
{
    lib_danavideo_smart_conf_parse((const int16_t *)pstream_data->buf, pstream_data->size);
}


/**
* @brief 	danavideo_smart_conf_response_callback
			智能声控解析成功后播放
* @date 	2015/3
* @param:	const char *audio,  要播的音频
			size_t size， 音频的size
* @return 	void
* @retval 	
*/

void danavideo_smart_conf_response_callback(const char *audio, size_t size)
{
	pthread_mutex_lock(&smart_voice_mutex); 
    if(smart_run_flag)
    {
        audio_write_da(smart_handle, (uint8 *)audio, size);
    }
	pthread_mutex_unlock(&smart_voice_mutex);
}


/**
* @brief 	dana_start_voice_control_wifi
			开启智能声控
* @date 	2015/3
* @param:	const char *path
* @return 	void
* @retval 	
*/

void dana_start_voice_control_wifi(const char *path)
{    
	T_AUDIO_INPUT audioInput;

    //return ;
    if(smart_handle)
    {
        return;
    }
    anyka_print("[%s,%d] we will start dana smart voice!\n", __FUNCTION__, __LINE__);
	audioInput.nBitsPerSample = 16;
	audioInput.nChannels = 1;
	audioInput.nSampleRate = 44100;
    audio_modify_sample(&audioInput);	//set sample rate
    audio_modify_filter(0);	//close filter
	audio_modify_vol(7);	//volum to high

    smart_handle = audio_da_open(&audioInput);		//open da
    audio_da_spk_enable(1);	//enable pa
    audio_set_ad_agc_enable(0);	//agc disable
    audio_da_filter_enable(smart_handle, 0);	//filter disable

	/* dana replace lib for airlink, compile fail disable it */
    //lib_danavideo_smart_conf_set_danalepath(path);
    //lib_danavideo_smart_conf_set_volume(0x2400);	//set volume
    lib_danavideo_smart_conf_init();	//init smart 
    lib_danavideo_set_smart_conf_response(danavideo_smart_conf_response_callback);	//register callback
	
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&smart_voice_mutex, &attr);
	
	/**add audio **/
    if(audio_add(SYS_AUDIO_RAW_PCM, danavideo_audio_add_callback, (void *)&danavideocallbackfuns) < 0)
    {       
        anyka_print("audio_add failed\n");       
        return ;  
    }
    smart_run_flag = 1;
}


/**
* @brief 	dana_stop_voice_control_wifi
			停止智能声控
* @date 	2015/3
* @param:	void
* @return 	void
* @retval 	
*/

void dana_stop_voice_control_wifi(void)
{
	T_AUDIO_INPUT audioInput;
    if(smart_handle)
    {
    	pthread_mutex_lock(&smart_voice_mutex); 
        smart_run_flag = 0;
        audio_del(SYS_AUDIO_RAW_PCM, (void *)&danavideocallbackfuns);
    	audioInput.nBitsPerSample = 16;
    	audioInput.nChannels = 1;
    	audioInput.nSampleRate = 8000;
        audio_modify_sample(&audioInput);      //recover simple rate to 8K
        audio_da_spk_enable(0);	//pa disable
        audio_modify_filter(1);	//filter enable
		audio_set_ad_agc_enable(1);		//agc enable
        audio_da_close(smart_handle);	//close da , no efficences
        smart_handle = NULL;
		anyka_pthread_mutex_destroy(&smart_voice_mutex); 	//destroy lock resource 
        anyka_print("[%s,%d] we will stop dana smart voice!\n", __FUNCTION__, __LINE__);
    }
}


/**
* @brief 	dana_init_thread
			大拿云平台初始化线程，主要负责智能声控开关
* @date 	2015/3
* @param:	void* param
* @return 	void*
* @retval 	null 
*/
void* dana_init_thread(void* param)
{
	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());
    unsigned long time_jiff = 0, delay_cnt = 0, run_flag = 0;
    
    while(!lib_danavideo_init((const char *)param, NULL, NULL, NULL, NULL, NULL, &danavideocallbackfuns)) 
    {
		//record run or detection run , stop voice control wifi 
        if(video_record_check_run() || anyka_dection_save_record())
        {
            dana_stop_voice_control_wifi();
        }
        sleep(1);
    }

    while(!lib_danavideo_start()) //while dana lib start , do next things
    {
        delay_cnt ++;
        if(delay_cnt > 2)
        {	
			//record not run and  detection not  run
            if((run_flag == 0) && (!(video_record_check_run() || anyka_dection_save_record())))
            {
                run_flag = 1;
				// start voice control wifi 
                dana_start_voice_control_wifi((const char *)param);
            }
        }
		/** wait for service start **/
        for(time_jiff = 0; time_jiff < 100; time_jiff ++)
        {
            if(video_record_check_run() || anyka_dection_save_record())
            {
                dana_stop_voice_control_wifi();
            }
            usleep(10*1000);
        }
    }
	//stop voice control wifi 
    dana_stop_voice_control_wifi();
    anyka_print("************dana app runs****************\n");
    #if 0
    unsigned long pre_hb_time = 0, ping_size;
    char *ping_buf = (char *)malloc(512), *find;
    while(1)
    {
        pre_hb_time = dana_hb_time;
        sleep(30);
        if(dana_hb_time == 0)
        {
            continue;
        }
        if(pre_hb_time == dana_hb_time)
        {
           //system("rm -rf /tmp/ping.txt && ping -c 4 8.8.8.8 > /tmp/ping.txt");            
            FILE *fp = popen ("ping -c 4 8.8.8.8", "r");
            if(fp == NULL)
            {
                anyka_print("[%s,%d]it fails to popen!\n", __FUNCTION__, __LINE__);
                continue ;
            }
            ping_size = fread(ping_buf, 1, 511,  fp);
            ping_buf[ping_size] = 0;
            find = strstr(ping_buf, "packets transmitted,");
            if(find)
            {
                find += strlen("packets transmitted,");
                ping_size = atoi(find);
                if(ping_size != 0)
                {
                    anyka_print("\n[%s,%d] the dana heart lost, we will reset anyka_ipc\n\n", __FUNCTION__, __LINE__);
                    video_record_stop();
                    anyka_dection_stop(0xFF);
                    exit(-1);
                }
                else
                {
                    dana_hb_time = 0;
                }
            }
            pclose(fp);
        }
    }
    #endif
    pthread_exit(NULL);
    return NULL;
}

/**
* @brief 	danavideo_conf_create_or_updated
* @date 	2015/3
* @param:	const char *conf_absolute_pathname
* @return 	void
* @retval 
*/

void danavideo_conf_create_or_updated(const char *conf_absolute_pathname)
{
    dbg("CONF_create_or_updated  conf_absolute_pathname: %s\n", conf_absolute_pathname);
}


/**
* @brief 	danavideo_get_connected_wifi_quality
			获取连接的wifi的信号强度，目前定死为45
* @date 	2015/3
* @param:	const char *conf_absolute_pathname
* @return 	uint32_t
* @retval 	wifi的信号强度
*/

uint32_t danavideo_get_connected_wifi_quality()
{
    uint32_t wifi_quality = 45;
    return wifi_quality;
}



/**
* @brief 	danavideo_productsetdeviceinfo
			设置设备信息
* @date 	2015/3
* @param:	const char *model, 
			const char *sn, 
			const char *hw_mac
* @return 	void
* @retval 	
*/
  
// 鐢熶骇宸ュ叿鍙互璁剧疆杩欎簺淇℃伅
void danavideo_productsetdeviceinfo(const char *model, const char *sn, const char *hw_mac)
{
    dbg("danavideo_productsetdeviceinfo\n");
    dbg("model: %s\tsn: %s\thw_mac: %s\n", model, sn, hw_mac);
}


/**
* @brief 	dana_init
			大拿云平台初始化入口函数
* @date 	2015/3
* @param:	char *danale_path， 大拿序列号文件路径
* @return 	void
* @retval 	
*/

void dana_init(char *danale_path)
{
    pthread_t dana_thread_id;
    dbg_off();	//关闭大拿库调试打印

    anyka_print("USING LIBDANAVIDEO_VERSION %s\n", lib_danavideo_linked_version_str(lib_danavideo_linked_version()));

	//注册回调
    lib_danavideo_set_hb_is_ok(danavideo_hb_is_ok);
    lib_danavideo_set_hb_is_not_ok(danavideo_hb_is_not_ok);

    lib_danavideo_set_upgrade_rom(danavideo_upgrade_rom);
     
    lib_danavideo_set_autoset(danavideo_autoset);
    lib_danavideo_set_local_setwifiap(danavideo_setwifiap);

    lib_danavideo_set_local_auth(danavideo_local_auth_callback);
    
    lib_danavideo_set_conf_created_or_updated(danavideo_conf_create_or_updated);
    
    lib_danavideo_set_get_connected_wifi_quality(danavideo_get_connected_wifi_quality);
    
    lib_danavideo_set_productsetdeviceinfo(danavideo_productsetdeviceinfo);
    int32_t maximum_size = 3*1024*1024;
    lib_danavideo_set_maximum_buffering_data_size(maximum_size);
	    
	set_wifi_by_airlink((void *)danale_path);
	//启动智能声控线程
    anyka_pthread_create(&dana_thread_id, dana_init_thread, (void *)danale_path, ANYKA_THREAD_MIN_STACK_SIZE, -1 );
	init_dana_pzt_ctrl();

}

