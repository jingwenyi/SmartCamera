#include "common.h"
#include "anyka_dana.h"
#include "danavideo_cmd.h"
#include "danavideo.h"
#include "danavideo.h"
#include "mediatyp.h"
#include "debug.h"
#include "dana_interface.h"

extern int dana_time_zone;	//dana time zone 
#define STEP_MAX 100
typedef struct _ak_pzt_ctrl_info
{
    sem_t ctrl_sem;
	danavideo_ptz_ctrl_t code;
	int step_count;
}ak_pzt_ctrl_info, *Pak_pzt_ctrl_info;

static Pak_pzt_ctrl_info ppzt_ctrl = NULL;



/**
* @brief 	dana_pzt_ctrl_thread
			dana control pzt
* @date 	2016/4
* @param:	void* param
* @return 	void*
* @retval 	null 
*/
static void* dana_pzt_ctrl_thread(void* param)
{
	      	  
    while (1) {
         sem_wait(&ppzt_ctrl->ctrl_sem);
		 while((ppzt_ctrl->code > DANAVIDEO_PTZ_CTRL_STOP) && (ppzt_ctrl->code <= DANAVIDEO_PTZ_CTRL_MOVE_RIGHT)&& (ppzt_ctrl->step_count++ < STEP_MAX)){
		 	switch( ppzt_ctrl->code )
			{
				case DANAVIDEO_PTZ_CTRL_MOVE_UP:
					PTZControlUp(0);		//run up step
					break;
				case DANAVIDEO_PTZ_CTRL_MOVE_DOWN:
					PTZControlDown(0);		//run down step
					break;
				case DANAVIDEO_PTZ_CTRL_MOVE_LEFT:
					PTZControlLeft(0);	//run left step
					break;
				case DANAVIDEO_PTZ_CTRL_MOVE_RIGHT:
					PTZControlRight(0);		//run right step
					break;
				default:
					break;
    		}
			usleep(10000);
		 }
		 ppzt_ctrl->step_count = 0;
    }
	free(ppzt_ctrl);
    return NULL;
}


/**
* @brief 	init_dana_pzt_ctrl
* 		init one thread to control dana pzt control
* @date 	2016/4
* @param:	void 
			
* @return 	void
* @retval 	int 
*/

int  init_dana_pzt_ctrl(void)
{    
    pthread_t pzt_thread_id;
	ppzt_ctrl = (Pak_pzt_ctrl_info)malloc(sizeof(ak_pzt_ctrl_info));
    if(NULL == ppzt_ctrl )
	{
        anyka_print("[%s:%d] it fail to malloc !\n", __func__, __LINE__);		
        return -1;		
    }    
    if(sem_init(&ppzt_ctrl->ctrl_sem, 0, 0))
    {
        anyka_print("[%s:%d] it fail to create sem!\n", __func__, __LINE__);		
        return -1;		
    }    
	ppzt_ctrl->step_count = 0;
	anyka_pthread_create(&pzt_thread_id, dana_pzt_ctrl_thread, NULL, ANYKA_THREAD_MIN_STACK_SIZE, -1);
	return 0;
}


/**
* @brief 	anyka_set_pzt_control
* 			云台控制接口
* @date 	2015/3
* @param:	int code ，控制类型
			int para1，参数
* @return 	int
* @retval 	成功0
*/

int anyka_set_pzt_control( int code , int para1)
{

	if( code == DANAVIDEO_PTZ_CTRL_ZOOM_IN )
	{
		//Set_Zoom(1); undefined
		return 0;
	}
	else if( code == DANAVIDEO_PTZ_CTRL_ZOOM_OUT)
	{
		//Set_Zoom(0); undefined
		return 0;
	}
	
	switch( code )
	{
	/*after up down left right cmd, pzt move until stop cmd*/
		case DANAVIDEO_PTZ_CTRL_STOP:	
		case DANAVIDEO_PTZ_CTRL_MOVE_UP:			
		case DANAVIDEO_PTZ_CTRL_MOVE_DOWN:			
		case DANAVIDEO_PTZ_CTRL_MOVE_LEFT:			
		case DANAVIDEO_PTZ_CTRL_MOVE_RIGHT:
			ppzt_ctrl->code = code;
			ppzt_ctrl->step_count = 0;
			sem_post(&ppzt_ctrl->ctrl_sem);
			break;

		case DANAVIDEO_PTZ_CTRL_SET_PSP:
			PTZControlSetPosition(para1);	//set positon
			break;
		case DANAVIDEO_PTZ_CTRL_CALL_PSP:
			PTZControlRunPosition(para1);	//run to the position
			break;
		case DANAVIDEO_PTZ_CTRL_DELETE_PSP:
			break;
		default :
			return 0;		
	}	
	
	return 0;
}


/**
* @brief 	anyka_set_pzt_control
* 			云台控制接口
* @date 	2015/3
* @param:	int code ，控制类型
			int para1，参数
* @return 	void
* @retval 	
*/

void anyka_send_video_move_alarm(int alarm_type, int save_flag, char *save_path, int start_time, int time_len)
{    
    uint32_t msg_type = 1;
    char     *msg_title = "anyka video warning";
    char     *msg_body  = "lib_danavideo_util_pushmsg";
    int64_t  cur_time = time(0);

	Psystem_alarm_set_info alarm_info = anyka_sys_alarm_info();

    if(alarm_type == SYS_CHECK_VIDEO_ALARM)
        msg_type = 1;
    else if(alarm_type == SYS_CHECK_VOICE_ALARM)
        msg_type = 2;
    else
        msg_type = 4;
    anyka_print("[%s:%d]we will send the warning to the phone!\n", __func__, __LINE__);
    if(save_flag == 0)
    {     
        uint32_t att_flag = 0;
        uint32_t record_flag = 0;
        if (lib_danavideo_util_pushmsg(1, alarm_info->motion_detection, msg_type, msg_title, msg_body, cur_time, att_flag, NULL, NULL, record_flag, 0, 0, 0, NULL))
        {
            anyka_print("\x1b[32mtestdanavideo TEST lib_danavideo_util_pushmsg success\x1b[0m\n");
        } 
        else 
        {        
            anyka_print("\x1b[34mtestdanavideo TEST lib_danavideo_util_pushmsg failed\x1b[0m\n");    
        }
    }
    else
    {
        uint32_t att_flag = 0;
        uint32_t record_flag = 1;
        uint32_t save_site = 1; //1,SD;2, 为? 3, S3?4, Google?   
        if (lib_danavideo_util_pushmsg(1, alarm_info->motion_detection, msg_type, msg_title, msg_body, cur_time, att_flag, NULL, NULL, record_flag, start_time, time_len, save_site, save_path)) 
        {
            anyka_print("\x1b[32mtestdanavideo TEST lib_danavideo_util_pushmsg success\x1b[0m\n");
        } 
        else 
        { 
            anyka_print("\x1b[34mtestdanavideo TEST lib_danavideo_util_pushmsg failed\x1b[0m\n");
        }
    }
}



/**
* @brief 	anyka_send_net_video
* 			发送网络视频回调
* @date 	2015/3
* @param:	T_VOID *param，数据
			T_STM_STRUCT *pstream，视频数据参数
* @return 	void
* @retval 	
*/

void anyka_send_net_video(T_VOID *param, T_STM_STRUCT *pstream)
{
    MYDATA *mydata = (MYDATA *)param;
    

    if(pstream->iFrame)
    {
        lib_danavideoconn_send((pdana_video_conn_t)mydata->danavideoconn, video_stream, H264, mydata->chan_no, 
            pstream->iFrame, pstream->timestamp, (const char*)pstream->buf, 37, 500*1000); 
        lib_danavideoconn_send((pdana_video_conn_t)mydata->danavideoconn, video_stream, H264, mydata->chan_no, 
            pstream->iFrame, pstream->timestamp, (const char*)(pstream->buf + 37), pstream->size - 37, 500*1000); 
    }
    else
    {
        lib_danavideoconn_send((pdana_video_conn_t)mydata->danavideoconn, video_stream, H264, mydata->chan_no, 
            pstream->iFrame, pstream->timestamp, (const char*)pstream->buf, pstream->size, 500*1000); 
    }

}

/**
* @brief 	anyka_send_dana_pictrue
* 			发送网络照片回调
* @date 	2015/3
* @param:	T_VOID *param，数据
			T_STM_STRUCT *pstream，视频数据参数
* @return 	void 
* @retval 
*/

void anyka_send_dana_pictrue(void *parm, T_STM_STRUCT *pstream)
{
    MYDATA *mydata = (MYDATA *)parm;


    lib_danavideoconn_send((pdana_video_conn_t)mydata->danavideoconn, pic_stream, JPG, mydata->chan_no, 
        pstream->iFrame, pstream->timestamp, (const char*)pstream->buf, pstream->size, 500*1000); 
}


/**
* @brief 	anyka_send_net_audio
* 			发送网络音频回调
* @date 	2015/3
* @param:	T_VOID *param，数据
			T_STM_STRUCT *pstream，视频数据参数
* @return 	void
* @retval 	
*/

void anyka_send_net_audio(T_VOID *param, T_STM_STRUCT *pstream)
{
    MYDATA *mydata = (MYDATA *)param;


    lib_danavideoconn_send((pdana_video_conn_t)mydata->danavideoconn, audio_stream, G711A, mydata->chan_no, 
        pstream->iFrame, pstream->timestamp, (const char*)pstream->buf, pstream->size, 500*1000); 
}


/**
* @brief 	anyka_get_talk_data
* 			获取语音数据回调
* @date 	2015/3
* @param:	T_VOID *param，数据
* @return 	void *
* @retval 	
*/

void *anyka_get_talk_data(void *param )
{
    MYDATA *mydata = (MYDATA *)param;
    T_STM_STRUCT *ptalk_data = NULL;
   
    dana_audio_packet_t *pmsg = lib_danavideoconn_readaudio(mydata->danavideoconn, 1000);
    if (pmsg) 
    {
        ptalk_data = malloc(sizeof(T_STM_STRUCT));
        ptalk_data->buf = (T_pDATA)malloc(pmsg->len);
        ptalk_data->size = pmsg->len;
        memcpy((void *)ptalk_data->buf, pmsg->data, ptalk_data->size);
        lib_danavideo_audio_packet_destroy(pmsg);
    }     
    return (void *)ptalk_data;
}

/**
* @brief 	anyka_get_osd_info
* 			获取osd信息
* @date 	2015/3
* @param:	libdanavideo_osdinfo_t *osd_info，将信息填入该结构
* @return 	void
* @retval 	
*/

void anyka_get_osd_info(libdanavideo_osdinfo_t *osd_info)
{
#if 0
    Pcamera_disp_setting pcamera_info = anyka_get_camera_info();

    osd_info->chan_name_show = pcamera_info->osd_switch;
    osd_info->show_name_x = pcamera_info->osd_pos_x;
    osd_info->show_name_y = pcamera_info->osd_pos_y;
    osd_info->datetime_show = pcamera_info->time_switch;
    osd_info->show_datetime_x = pcamera_info->time_pos_x;
    osd_info->show_datetime_y = pcamera_info->time_pos_y;
    osd_info->custom1_show = 0;
    osd_info->custom2_show = 0;

    if(pcamera_info->date_format == 0)
    {
        osd_info->show_format = 0;
    }
    else
    {
        osd_info->show_format = pcamera_info->date_format - 1;
    }
    osd_info->hour_format = pcamera_info->hour_format;
    osd_info->show_week = pcamera_info->week_format;
    osd_info->datetime_attr = 0;
#endif    
}


/**
* @brief 	anyka_set_osd_info
* 			设置osd信息
* @date 	2015/3
* @param:	DANAVIDEOCMD_SETOSD_ARG *osd_info，指向要设置的信息的结构体
* @return 	void
* @retval 	
*/

void anyka_set_osd_info(DANAVIDEOCMD_SETOSD_ARG *osd_info)
{
#if 0
    Pcamera_disp_setting pcamera_info = anyka_get_camera_info();


    pcamera_info->osd_switch = osd_info->osd.chan_name_show;
    pcamera_info->osd_pos_x = osd_info->osd.has_show_name_x;
    pcamera_info->osd_pos_y = osd_info->osd.has_show_name_y;
    pcamera_info->time_switch = osd_info->osd.datetime_show;
    pcamera_info->time_pos_x = osd_info->osd.show_datetime_x;
    pcamera_info->time_pos_y = osd_info->osd.show_datetime_y;
    if(osd_info->osd.has_show_format == 0)
    {        
        pcamera_info->date_format = 0;
    }
    else
    {
        pcamera_info->date_format = osd_info->osd.show_format + 1;
    }
    pcamera_info->hour_format = osd_info->osd.hour_format;
    pcamera_info->week_format = osd_info->osd.show_week;
    anyka_set_camera_info(pcamera_info);	//set info to camera
#endif
}



/**
* @brief 	anyka_get_reclist
* 			获取录像回放列表
* @date 	2015/3
* @param:	DANAVIDEOCMD_RECLIST_ARG *rec_info，要获取录像的信息，如起始时间
			int *rec_num， 获取录像信息数目
* @return 	libdanavideo_reclist_recordinfo_t *指向录像回放列表指针
* @retval 	
*/
#if 0
libdanavideo_reclist_recordinfo_t * anyka_get_reclist(DANAVIDEOCMD_RECLIST_ARG *rec_info, int *rec_num)
{
    int i = 0;
    libdanavideo_reclist_recordinfo_t *record_info; 
    record_replay_info find;
    unsigned long find_end;
    void *record_fd;
    unsigned long check_time = rec_info->last_time;
   
	check_time += dana_time_zone;
    find_end = check_time + 24*60*60;

    record_info = (libdanavideo_reclist_recordinfo_t *)malloc(sizeof(libdanavideo_reclist_recordinfo_t) * rec_info->get_num);
    memset(record_info, 0, sizeof(libdanavideo_reclist_recordinfo_t) * rec_info->get_num);
    record_fd = record_replay_get_open_file(RECORD_REPLAY_TYPE, check_time, find_end, &find);
    if(record_fd != NULL)
    {
        for(i = 0; i < rec_info->get_num; i ++)
        {
            record_info[i].length = find.record_time;
            record_info[i].start_time = find.recrod_start_time - dana_time_zone;
            record_info[i].record_type = DANAVIDEO_REC_TYPE_NORMAL;
            anyka_print("[%s:%d] we find(%d)the record with name:%s, start:(%ld),time:(%ld)\n",
						__func__, __LINE__, i + 1, find.file_name, find.recrod_start_time, find.record_time);            
            if(0 == record_replay_get_read(record_fd, find_end, &find))
            {
                i ++;
                break;
            }
        }
        record_replay_get_close_file(record_fd);
    }
    
    *rec_num = i;
    anyka_print("we find fit record:%d; start:%ld, end:%ld\n", i,check_time, find_end);
    if(i == 0)
    {
        free(record_info);
        return NULL;
    }
    return record_info;
}

/**
* @brief 	anyka_dana_send_record_data
* 			录像回放发送录像数据
* @date 	2015/3
* @param:	T_VOID *param
			int typ， 是视频还是音频
			T_STM_STRUCT *pstream， 音/ 视频数据信息
* @return 	void
* @retval 	
*/

void anyka_dana_send_record_data(T_VOID *param, int type, T_STM_STRUCT *pstream)
{
    if(type == 1)
    {
        anyka_send_net_video(param, pstream);
    }
    else
    {
        //anyka_send_net_audio(param, pstream);
    }
}


/**
* @brief 	anyka_dana_record_play
* 			启动录像回放
* @date 	2015/3
* @param:	unsigned long start_time， 起始时间
			void *mydata， 数据
* @return 	int
* @retval 	
*/

int anyka_dana_record_play(unsigned long start_time, void *mydata)
{
    unsigned long end_time;
    time_t tmp;
    struct tm *dest_time;

	start_time += dana_time_zone;
	tmp = start_time;
	
    dest_time = localtime(&tmp);
    dest_time->tm_hour = 0;
    dest_time->tm_sec = 0;
    dest_time->tm_min = 0;
    end_time = mktime(dest_time)+ 24*60*60;
	
    anyka_dana_record_stop(mydata);
    return record_replay_start(start_time, end_time, RECORD_REPLAY_TYPE, mydata, anyka_dana_send_record_data);
}
#endif

/**
* @brief 	anyka_dana_record_play
* 			停止录像回放
* @date 	2015/3
* @param:	
			void *mydata， 数据
* @return 	int
* @retval 	
*/

void anyka_dana_record_stop(void *mydata)
{
    record_replay_stop(mydata);
}

/**
* @brief 	anyka_dana_record_play
* 			暂停录像回放
* @date 	2015/3
* @param:	void *mydata， 数据
			int play，播放标志， 0  暂停
* @return 	int
* @retval 	
*/

void anyka_dana_record_play_pause(void *mydata, int play)
{
    record_replay_play_status(mydata, play);
}



