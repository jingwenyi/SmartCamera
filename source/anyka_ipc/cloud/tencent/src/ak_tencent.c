#include "common.h"
#include "TXAudioVideo.h"
#include "TXDataPoint.h"
#include "TXOTA.h"
#include "TXVoiceLink.h"
#include "ak_tencent.h"
#include "anyka_config.h"
#include <signal.h>
#include "record_replay.h"
#include "convert.h"

//定义网络模式的视频码率范围 (kbps)
//QQ物联平台会根据网络状态自动调整实际码率，此处只是设置一个大的范围即可
#define NET_720P_MAX_BPS   (1000)   //720P HD
#define NET_720P_MIN_BPS   (500)   //720P HD
#define NET_VGA_MAX_BPS    (500)   //VGA SD
#define NET_VGA_MIN_BPS    (400)   //VGA SD
#define NET_SMOOTH_MAX_BPS (250)   //VGA Smooth
#define NET_SMOOTH_MIN_BPS (200)   //VGA Smooth

#define WIRE_LESS_TMP_DIR      "/tmp/wireless/"


typedef struct _ak_qq_video_info
{
    int run_flag;
    int gop_index;
    int iframe_index;
    int frame_index;
    int reset_flag;
    int min_bps;
    int max_bps;
    int video_type;
}ak_qq_video_info, *Pak_qq_video_info;

static Pak_qq_video_info pvideo_send = NULL;

typedef struct _ak_qq_audio_info
{
    int send_flag;
    int back_flag;
    int accept_flag;
    sem_t talk_sem;
    pthread_mutex_t talk_lock;
    void *talk_queue;
}ak_qq_audio_info, *Pak_qq_audio_info;

enum
{
    AK_QQ_START_VIDEO,
    AK_QQ_RESET_VIDEO,
    AK_QQ_SET_VIDEO,
    AK_QQ_STOP_VIDEO,
    AK_QQ_START_MIC,
    AK_QQ_STOP_MIC,
    AK_QQ_START_VIDEO_REPLAY,
};

typedef struct _ak_qq_command_
{
    int type;
    int para;
}ak_qq_command, *Pak_qq_command;

typedef struct _ak_qq_ctrl_info
{
    sem_t ctrl_sem;
    void *ctrl_queue;
    void *data_point_queue;
}ak_qq_ctrl_info, *Pak_qq_ctrl_info;

static Pak_qq_audio_info paudio_send = NULL;
Pak_qq_ctrl_info pqq_ctrl = NULL;



/**
 * NAME         ak_qq_sendvideo
 * @BRIEF	视频回调函数，将送视频数据给腾讯
 * @PARAM	param   供多用户使用，启动视频时注册时的指针，腾讯暂不支持这方式
                    pstream  视频数据相关信息
 * @RETURN	NONE
 * @RETVAL	
 */

void ak_qq_sendvideo(T_VOID *param, T_STM_STRUCT *pstream)
{
    if(pvideo_send == NULL)
    {
        return;
    }
    if(pvideo_send->run_flag == 0)
    {
        return;
    }
    
    if(pvideo_send->reset_flag == 1)
    {
        if(pstream->iFrame)
        {
            pvideo_send->reset_flag = 0;
        }      
        else
        {
            return;
        }
    }
    if(pstream->iFrame)
    {
        pvideo_send->iframe_index = 0;
        pvideo_send->gop_index ++;
    }
    else
    {
        pvideo_send->iframe_index ++;
    }
    pvideo_send->frame_index ++;
    if(pvideo_send->reset_flag == 0)
    {
        tx_set_video_data(pstream->buf, pstream->size, pstream->iFrame?0:1, pstream->timestamp, pvideo_send->gop_index, pvideo_send->iframe_index, pvideo_send->frame_index, 30);
    }
    
}


/**
 * NAME         ak_qq_start_video
 * @BRIEF	开始启动视频传输
 * @RETURN	NONE
 * @RETVAL	
 */

bool ak_qq_start_video()
{
    if(pvideo_send)
    {
        anyka_print("[%s:%d] it fails to start video because it is running!\n ", __func__, __LINE__);
        return 0;
    }
    
    pvideo_send = (Pak_qq_video_info)malloc(sizeof(ak_qq_video_info));
    memset(pvideo_send, 0, sizeof(ak_qq_video_info));
    pvideo_send->frame_index = -1;
    pvideo_send->gop_index = -1;
    pvideo_send->run_flag = 1;

    //启动视频预览，初始设置为高清模式
    pvideo_send->video_type = FRAMES_ENCODE_720P_NET;
    pvideo_send->min_bps = NET_720P_MIN_BPS;
    pvideo_send->max_bps = NET_720P_MAX_BPS;
    
    anyka_start_video_withbps(pvideo_send->video_type, pvideo_send->min_bps, pvideo_send, ak_qq_sendvideo);
    return 1;
}

/**
 * NAME         ak_qq_set_video_para
 * @BRIEF	开始启动视频传输
 * @PARAM	type    视频的类型
 * @RETURN	NONE
 * @RETVAL	
 */

bool ak_qq_set_video_para(int type)
{
    if(pvideo_send == NULL)
    {
        anyka_print("[%s:%d] it fails to change video beacuse it is down!\n ", __func__, __LINE__);
        return 0;
    }

    if(type == 3) //720P HD
    {
        pvideo_send->video_type = FRAMES_ENCODE_720P_NET;
        pvideo_send->min_bps = NET_720P_MIN_BPS;
        pvideo_send->max_bps = NET_720P_MAX_BPS;
    }
    else if(type == 2) //VGA SD
    {
        pvideo_send->video_type = FRAMES_ENCODE_VGA_NET;
        pvideo_send->min_bps = NET_VGA_MIN_BPS;
        pvideo_send->max_bps = NET_VGA_MAX_BPS;
    }
    else if(type == 1)  //VGA Smooth
    {
        pvideo_send->video_type = FRAMES_ENCODE_VGA_NET;
        pvideo_send->min_bps = NET_SMOOTH_MIN_BPS;
        pvideo_send->max_bps = NET_SMOOTH_MAX_BPS;
    }    
    anyka_set_video_para(pvideo_send->video_type, pvideo_send->min_bps, pvideo_send);
    return 1;
}




/**
 * NAME         ak_qq_stop_video
 * @BRIEF	关闭视频传输
 * @PARAM	bit_rate 码率
 * @RETURN	true
 * @RETVAL	
 */

bool ak_qq_stop_video(void)
{
    if(pvideo_send == NULL)
    {
        return 0;
    }
    anyka_stop_video(pvideo_send);
    free(pvideo_send);
    pvideo_send = NULL;
    return 1;
}


/**
 * NAME         ak_qq_reset_video
 * @BRIEF	已经发生丢帧了，下一帧将传输I帧
 * @PARAM	
 * @RETURN	true
 * @RETVAL	
 */

bool ak_qq_reset_video(void)
{
    if(pvideo_send == NULL)
    {
        return 0;
    }
    anyka_print("[%s,%d]we will reset video to iframe\n",__FUNCTION__, __LINE__);
    pvideo_send->reset_flag = 1;
    anyka_set_video_iframe(pvideo_send);
    return 1;
}


/**
 * NAME         ak_qq_set_video
 * @BRIEF	因网络质量发生变化，将修改码率
 * @PARAM	bit_rate 码率
 * @RETURN	true
 * @RETVAL	
 */
bool ak_qq_set_video(int bit_rate)
{
    if (pvideo_send == NULL)
    {
        return 1;   
    }

	if(pvideo_send->video_type != FRAMES_ENCODE_720P_NET) {
		anyka_debug("[%s:%d] video type is not high digit, skip, bps: %d\n", 
				__func__, __LINE__, bit_rate);
		return 1;
	}

	/*
	 * if last set val equals to current val, return
	 */
	static int last_bps;
	
    if(bit_rate >= pvideo_send->max_bps)
    {
        bit_rate = pvideo_send->max_bps;
    }
    else if(bit_rate < pvideo_send->min_bps)
    {
        bit_rate = pvideo_send->min_bps;
    }

	if (last_bps == bit_rate)
		return 1;
	
    anyka_debug("[%s:%d] we will set video rate: %d\n", __func__, __LINE__, bit_rate);
    anyka_start_video_withbps(pvideo_send->video_type, bit_rate, pvideo_send, NULL);
	last_bps = bit_rate;

    return 1;
}

/**
 * NAME         ak_qq_sendaudio
 * @BRIEF	向腾讯送音频数据
 * @PARAM	param:  供多用户使用，启动音频时注册时的指针，腾讯暂不支持这方式
                    pstream :音频数据方面的信息
 * @RETURN	NONE
 * @RETVAL	
 */
void ak_qq_sendaudio(void *param, T_STM_STRUCT *pstream)
{
    tx_audio_encode_param qq_audio_info;
    int off = 0;
    
    if (paudio_send == NULL)
        return;   
    
    if (paudio_send->send_flag == 0)
        return;

    memset(&qq_audio_info, 0, sizeof(qq_audio_info));
    qq_audio_info.head_length = sizeof(tx_audio_encode_param);
    qq_audio_info.audio_format = 1;  // 音频数据的编码类型，这里是AMR	
    qq_audio_info.encode_param = 7;  // AMR的mode
    qq_audio_info.frame_per_pkg = 8;
    qq_audio_info.sampling_info = GET_SIMPLING_INFO(1, 8000, 16);
    
    while (off < pstream->size) {
        tx_set_audio_data(&qq_audio_info, pstream->buf + off, 32);
        off += 32;
    }
}

/**
 * NAME     ak_qq_start_mic
 * @BRIEF	启动向腾讯发送对讲音频数据
 * @RETURN	true
 * @RETVAL	
 */
bool ak_qq_start_mic(void)
{
    if(paudio_send == NULL || paudio_send->send_flag)
    {
        return 0;
    }
    paudio_send->send_flag = 1;    
    anyka_print("[%s,%d]we will start mic speak!\n",__FUNCTION__, __LINE__);
    audio_speak_start(paudio_send, ak_qq_sendaudio, SYS_AUDIO_ENCODE_AMR);
    return 1;
}


/**
 * NAME     ak_qq_stop_mic
 * @BRIEF	结束向腾讯发送对讲音频数据
 * @RETURN	true
 * @RETVAL	
 */
bool ak_qq_stop_mic(void)
{
    if(paudio_send == NULL || paudio_send->send_flag==0)
    {
        return 0;
    }
    paudio_send->send_flag = 0;
    audio_speak_stop(paudio_send, SYS_AUDIO_ENCODE_AMR);
    anyka_print("[%s,%d]we will stop mic speak!\n",__FUNCTION__, __LINE__);
    return 1;
}

/**
 * NAME         ak_qq_getaudio_toak
 * @BRIEF	从数据缓冲区读数据，再发送给设备DA
 * @RETURN	NONE
 * @RETVAL	
 */

void * ak_qq_getaudio_toak(void * user )
{
    void  *paudio_stream;
    while(paudio_send->talk_queue && anyka_queue_not_empty(paudio_send->talk_queue) == 0)
    {
        sem_wait(&paudio_send->talk_sem);
    }
    paudio_stream = anyka_queue_pop(paudio_send->talk_queue);
    return paudio_stream;
}

/**
 * NAME         ak_qq_get_audio_data
 * @BRIEF	腾讯向我们设备发送音频数据
 * @PARAM	pcEncData  数据缓冲区
                    nEncDataLen 数据长度
 * @RETURN	NONE
 * @RETVAL	
 */
void ak_qq_get_audio_data(tx_audio_encode_param *pAudioEncodeParam, unsigned char *pcEncData, int nEncDataLen)
{
    T_STM_STRUCT  *paudio_stream;

    pthread_mutex_lock(&paudio_send->talk_lock);
    paudio_send->accept_flag = 0; 
    if(paudio_send->talk_queue == NULL)
    {
        anyka_print("[%s:%d]we will start play voice!\n", __func__, __LINE__);
        paudio_send->back_flag = paudio_send->send_flag;
        if(paudio_send->send_flag)
        {
            ak_qq_stop_mic();
        }
        paudio_send->talk_queue = anyka_queue_init(200);
        audio_talk_start(paudio_send, ak_qq_getaudio_toak, SYS_AUDIO_ENCODE_AMR);        
    }
    #if 1
    paudio_stream = anyka_malloc_stream_ram(nEncDataLen);
    if(paudio_stream == NULL)
    {
        anyka_print("[%s:%d] it fails to malloc\n ", __func__, __LINE__);
        pthread_mutex_unlock(&paudio_send->talk_lock);
        return ;
    }
    memcpy(paudio_stream->buf, pcEncData, nEncDataLen);
    paudio_stream->size = nEncDataLen;
    if(anyka_queue_push(paudio_send->talk_queue, paudio_stream) == 0)
    {
        anyka_free_stream_ram(paudio_stream);
        anyka_print("[%s:%d] it fails to add talk audio\n ", __func__, __LINE__);
    }
    else
    {
        sem_post(&paudio_send->talk_sem);
    }
    #endif
    pthread_mutex_unlock(&paudio_send->talk_lock);
}

void ak_qq_sen_msg_over(const unsigned int cookie, int err_code)
{
    char *file_name;
    file_name = (char *)anyka_queue_pop(pqq_ctrl->data_point_queue);

    if(file_name)
    {
        anyka_debug("[%s:%d] qq message send over, del the file(%s)\n",
            __func__, __LINE__, file_name);
        remove(file_name);
        free((void *)file_name);
    }
}

/**
 * NAME         ak_qq_check_video_filter
 * @BRIEF	检测是否在网络视频 
 * @PARAM	NONE
 * @RETURN	0:没有在预览状态；1 :在预览状态
 * @RETVAL	
 */

int ak_qq_check_video_filter(void)
{
    return pvideo_send != NULL;
}    

//get audio message duration
static int get_duration(char *file_name)
{
    int duration = 0;
    char *ext;
    struct stat fi;
    
    ext = strstr(file_name, ".");
	if(!ext)
		return 0;

    if(strcmp(ext, ".amr") == 0)
    {
        if (stat(file_name, &fi) == 0)
            duration = fi.st_size / (12200/8); //amr MR122
    }    
    else
    {
        //printf("%s: unknown file extension %s!!!\n", __FUNCTION__, ext);
    }

    return duration;
}

void ak_qq_send_msg(int msg_id, char * file_name, char *title, char *content, int alarm_time)
{
    char *send_file_name;
    char *old_file;
    structuring_msg 	qq_dp_info = {0};
    tx_send_msg_notify  notify = {0};

	if(strlen(file_name) == 0) {
            anyka_print("[%s:%d] file name err\n", __func__, __LINE__);
			return ;
	}


	
    //在网络不好的情况下，如果队列里面的alarm信息没有上传完毕，会导致队列爆掉，还会引发内存泄漏触发oom-killer。
    //此处检测如果队列里的元素个数超过5个，则删除旧的alarm信息
    if (anyka_queue_items(pqq_ctrl->data_point_queue) > 5)
    {
        old_file = (char *)anyka_queue_pop(pqq_ctrl->data_point_queue);
        if(old_file)
        {
            anyka_print("[%s:%d]pop and delete timeout msg file(%s)\n",
                		__func__, __LINE__, old_file);
            remove(old_file);
            free((void *)old_file);
        }
    }

	qq_dp_info.msg_id = msg_id;
    qq_dp_info.file_path = file_name;
    qq_dp_info.thumb_path = file_name;
    qq_dp_info.title = title;
    qq_dp_info.digest = content;
    qq_dp_info.duration = get_duration(file_name);
    notify.on_send_structuring_msg_ret = ak_qq_sen_msg_over;
    if(file_name)
    {
        send_file_name = (char *)malloc(strlen(file_name) + 2);
        if(send_file_name)
        {
            strcpy(send_file_name, file_name);
            anyka_queue_push(pqq_ctrl->data_point_queue, (void *)send_file_name);
        }
    }
    tx_send_structuring_msg(&qq_dp_info, &notify, NULL);
    anyka_debug("[%s:%d]qq message send with %x)\n",
        __func__, __LINE__, (int)send_file_name);
}


void* ak_qq_check_net_audio_data(void * arg)
{
    while(1)
    {
        pthread_mutex_lock(&paudio_send->talk_lock);
        if((paudio_send->accept_flag >= 2) && paudio_send->talk_queue)
        {
            void *talk_queue = paudio_send->talk_queue;
            paudio_send->talk_queue = NULL;
            sem_post(&paudio_send->talk_sem);
            anyka_queue_destroy(talk_queue, anyka_free_stream_ram);
            audio_talk_stop(paudio_send);  
            if(paudio_send->back_flag)
            {
                ak_qq_start_mic();
                paudio_send->back_flag= 0;
            }
            anyka_print("[%s,%d]we will stop play voice!\n",__FUNCTION__, __LINE__);
        }
        if(paudio_send->talk_queue)
        {
            paudio_send->accept_flag ++;
        }
        pthread_mutex_unlock(&paudio_send->talk_lock);
        usleep(250*1000);
    }
}

/**
 * NAME         ak_qq_init_audio
 * @BRIEF	因腾讯送DA数据没有开始与停止标记，所以先开启音频，等待数据
 * @PARAM	
 * @RETURN	NONE
 * @RETVAL	
 */

void ak_qq_init_audio(void)
{
    paudio_send = (Pak_qq_audio_info)malloc(sizeof(ak_qq_audio_info));
    if(NULL == paudio_send)
    {
        anyka_print("[%s:%d] it fails to malloc\n ", __func__, __LINE__);
        return ;
    }
    
    memset(paudio_send, 0, sizeof(ak_qq_audio_info));
    sem_init(&paudio_send->talk_sem, 0, 0);
    pthread_mutex_init(&paudio_send->talk_lock, NULL);
    
	pthread_t ntid = 0;
	
	anyka_pthread_create(&ntid, ak_qq_check_net_audio_data, NULL, ANYKA_THREAD_MIN_STACK_SIZE, -1);
    return ;
}

/**
 * NAME         qq_start_video
 * @BRIEF	开始启动视频传输
 * @RETURN	true
 * @RETVAL	
 */

bool qq_start_video()
{
    
    Pak_qq_command command;

    command = (Pak_qq_command)malloc(sizeof(ak_qq_command));
    if(command == NULL)
    {
        return 0;
    }
    command->type = AK_QQ_START_VIDEO;
    anyka_queue_push(pqq_ctrl->ctrl_queue, command);
    sem_post(&pqq_ctrl->ctrl_sem);
    return 1;
}


/**
 * NAME         qq_stop_video
 * @BRIEF	关闭视频传输
 * @PARAM	bit_rate 码率
 * @RETURN	true
 * @RETVAL	
 */

bool qq_stop_video(void)
{
    Pak_qq_command command;

    command = (Pak_qq_command)malloc(sizeof(ak_qq_command));
    if(command == NULL)
    {
        return 0;
    }
    if (pvideo_send)
        pvideo_send->run_flag = 0;
    command->type = AK_QQ_STOP_VIDEO;
    anyka_queue_push(pqq_ctrl->ctrl_queue, command);
    sem_post(&pqq_ctrl->ctrl_sem);
    return 1;
}

/**
 * NAME        qq_reset_video
 * @BRIEF	已经发生丢帧了，下一帧将传输I帧
 * @PARAM	
 * @RETURN	true
 * @RETVAL	
 */

bool qq_reset_video(void)
{
    Pak_qq_command command;

    command = (Pak_qq_command)malloc(sizeof(ak_qq_command));
    if(command == NULL)
    {
        return 0;
    }
    command->type = AK_QQ_RESET_VIDEO;
    anyka_queue_push(pqq_ctrl->ctrl_queue, command);
    sem_post(&pqq_ctrl->ctrl_sem);
    return 1;
}

/**
 * NAME         qq_set_video
 * @BRIEF	因网络质量发生变化，将修改码率
 * @PARAM	bit_rate 码率
 * @RETURN	true
 * @RETVAL	
 */

bool qq_set_video(int bit_rate)
{
    Pak_qq_command command;

    command = (Pak_qq_command)malloc(sizeof(ak_qq_command));
    if(command == NULL)
    {
        return 0;
    }
    command->type = AK_QQ_SET_VIDEO;
    command->para = bit_rate;
    anyka_queue_push(pqq_ctrl->ctrl_queue, command);
    sem_post(&pqq_ctrl->ctrl_sem);
    return 1;
}

/**
 * NAME     qq_start_mic
 * @BRIEF	启动向腾讯发送音频数据
 * @RETURN	true
 * @RETVAL	
 */
bool qq_start_mic(void)
{
    Pak_qq_command command;

    command = (Pak_qq_command)malloc(sizeof(ak_qq_command));
    if(command == NULL)
    {
        return 0;
    }
    command->type = AK_QQ_START_MIC;
    anyka_queue_push(pqq_ctrl->ctrl_queue, command);
    sem_post(&pqq_ctrl->ctrl_sem);
    return 1;
}

/**
 * NAME     qq_stop_mic
 * @BRIEF	结束向腾讯发送音频数据
 * @RETURN	true
 * @RETVAL	
 */
bool qq_stop_mic(void)
{
    Pak_qq_command command;

    command = (Pak_qq_command)malloc(sizeof(ak_qq_command));
    if(command == NULL)
    {
        return 0;
    }
    command->type = AK_QQ_STOP_MIC;
    anyka_queue_push(pqq_ctrl->ctrl_queue, command);
    sem_post(&pqq_ctrl->ctrl_sem);
    return 1;
}

/**
 * NAME     qq_start_video_replay 
 * @BRIEF	start qq video replay
 * @PARAM	play_time : begin play time
 * @RETURN	NONE
 * @RETVAL	
 */
void qq_start_video_replay(unsigned int play_time)
{
    Pak_qq_command command;

    command = (Pak_qq_command)malloc(sizeof(ak_qq_command));
    if(command == NULL)
    {
        return ;
    }
    command->type = AK_QQ_START_VIDEO_REPLAY;
    command->para = play_time;
    anyka_queue_push(pqq_ctrl->ctrl_queue, command);
    sem_post(&pqq_ctrl->ctrl_sem);

}

/**
 * NAME     ak_qq_ctrl_main_thread
 * @BRIEF	腾讯消息处理
 * @RETURN	NONE
 * @RETVAL	
 */

void* ak_qq_ctrl_main_thread(void * arg)
{
    Pak_qq_command command;
    while(1)
    {
        sem_wait(&pqq_ctrl->ctrl_sem);
        static int stat_real=1;
        static int stat_mic =0;
        while(NULL != (command = anyka_queue_pop(pqq_ctrl->ctrl_queue)))
        {
            switch(command->type)
            {
                case AK_QQ_START_VIDEO:
                    ak_qq_start_video();
                    stat_real = 1;
                    break;
                    
                case AK_QQ_RESET_VIDEO:
                	if (stat_real)
	                    ak_qq_reset_video();
                    break;
                    
                case AK_QQ_SET_VIDEO:
                	if (stat_real)
	                    ak_qq_set_video(command->para);
                    break;
                    
                case AK_QQ_STOP_VIDEO:
                	if (stat_real)
	                    ak_qq_stop_video();
					else
						ak_qq_stop_video_replay();
                    break;
                    
                case AK_QQ_START_MIC:
                    ak_qq_start_mic();
                    stat_mic = 1;
                    break;
                    
                case AK_QQ_STOP_MIC:
                    ak_qq_stop_mic();
                    stat_mic = 0;
                    break;
                case AK_QQ_START_VIDEO_REPLAY:
					ak_qq_stop_video_replay();
                	if (command->para==0)
                	{
                		stat_real = 1;
                		ak_qq_start_video();
                		if (stat_mic)
		                    ak_qq_start_mic();
                		
                	}
                	else
                	{
                		stat_real = 0;
	                    ak_qq_stop_video();
	                    if (stat_mic)
		                    ak_qq_stop_mic();
	                	ak_qq_start_video_replay(command->para);
	                }
            }
            free(command);
        }
    }
}

/**
 * NAME         ak_qq_init_ctrl
 * @BRIEF	初始化与腾讯相关的对接控制
 * @PARAM	
 * @RETURN	NONE
 * @RETVAL	
 */

void ak_qq_init_ctrl(void)
{
    pqq_ctrl = (Pak_qq_ctrl_info)malloc(sizeof(ak_qq_ctrl_info));
    sem_init(&pqq_ctrl->ctrl_sem, 0, 0);
    pqq_ctrl->ctrl_queue = anyka_queue_init(20);
    pqq_ctrl->data_point_queue = anyka_queue_init(20);
    
	pthread_t ntid = 0;
	
	anyka_pthread_create(&ntid, ak_qq_ctrl_main_thread, NULL, ANYKA_THREAD_MIN_STACK_SIZE, -1);
}

/**
 * *  @brief		ak_qq_reset_timer, reset timer
 * *  @author	chen yanhong   
 * *  @date		2015-5-20
 * *  @param	sec, second you want to set 
 * *  @return	void
 * */
void ak_qq_reset_timer(int sec, int usec)
{
	int i;
	
	struct itimerval s_time;

	for(i = 0; i < 2; i++){

		s_time.it_value.tv_sec  = sec * i;
		s_time.it_value.tv_usec = usec * i;

		s_time.it_interval.tv_sec  = 0; //if reuse, then set this two number
		s_time.it_interval.tv_usec = 0;

		if(setitimer(ITIMER_REAL, &s_time, NULL) != 0) {
			perror("setitimer");
			break;
		}
	}
	
	return ;
}

/**
 * *  @brief		ak_qq_pause_dection, pause system movement dection
 * *  @author	chen yanhong   
 * *  @date		2015-5-20
 * *  @param	void
 * *  @return	void
 * */

void ak_qq_pause_dection(void)
{
    anyka_dection_pause_dection();
	ak_qq_reset_timer(5, 0); //reset timer
}

/**
 * *  @brief		time up callback
 * *  @author	chen yanhong   
 * *  @date		2015-5-20
 * *  @param	signal number
 * *  @return	void
 * */
void ak_qq_rotate_dealwith(int sig)
{
	//printf("######### resum dection #########\n");
    anyka_dection_resume_dection();
}

/**
 * NAME         ak_qq_init
 * @BRIEF	启动与腾讯对接的相关资源初始化
 * @PARAM	
 * @RETURN	NONE
 * @RETVAL	
 */

void ak_qq_init(void )
{
	signal(SIGALRM, ak_qq_rotate_dealwith);	//for timer
    ak_qq_init_audio();
    ak_qq_init_ctrl();
}


/**
 * NAME         ak_qq_control_rotate
 * @BRIEF	马达控制
 * @PARAM	rotate_direction: 马达转动的方向
                    rotate_degree   : 马达转动的角度，目前我们不支持。转动默认角度
 * @RETURN	NONE
 * @RETVAL	
 */
int ak_qq_control_rotate(int rotate_direction, int rotate_degree)
{
	//pause movement detection during PTZ control
	ak_qq_pause_dection();
	
	switch( rotate_direction )
	{
		case 1:
			PTZControlLeft(0);
			break;
		case 2:
			PTZControlRight(0);
			break;
		case 3:
			PTZControlUp(0);
			break;
		case 4:
			PTZControlDown(0);
			break;
		default :
			return 0;		
	}	
	anyka_print("on_control_rotate:::::%d::::%d----------------------------------------------------\n", 
				rotate_direction,rotate_degree);
	return 1;
}

/**
 * NAME     ak_qq_get_global_setting
 * @BRIEF	use to get the device name and check whether this machine is first start after update
 * @PARAM	dev_name: store the device name buf
 * 			first_start: store the value; 
 * 			1=> the device is first start; 0=> the device is not first start after update
 * @RETURN	int
 * @RETVAL	on success, return 0; else return -1;
 */
int ak_qq_get_global_setting(char *dev_name, unsigned short *firmware_update)
{
	/** get sys config **/
	system_user_info * set_info = anyka_get_system_user_info();
	if(set_info == NULL) {
		anyka_print("[%s:%d] get system user info error!!!\n", __func__, __LINE__);
		return -1;
	}

	if(set_info->firmware_update)
		*firmware_update = set_info->firmware_update; 

	if(set_info->dev_name)
		strcpy(dev_name, set_info->dev_name);

	anyka_print("[%s:%d] get system user info success!!!\n", __func__, __LINE__);
	return 0;
}

/**
 * NAME   	ak_qq_change_firmware_update_status 
 * @BRIEF	change the firmware update status
 * @PARAM	flags: flags 0 or 1
 * @RETURN	NONE
 * @RETVAL	
 */
void ak_qq_change_firmware_update_status(int flags)
{
	int ret;
	system_user_info* Puser_info = anyka_get_system_user_info();

	if(Puser_info == NULL) {
		anyka_print("[%s:%d] get system user info error!!!\n", __func__, __LINE__);
		return ;
	} 			 
	
	Puser_info->firmware_update = flags;

	ret = anyka_set_system_user_info(Puser_info);
	if(ret)
		anyka_print("[%s:%d] set system user info error!!!\n", __func__, __LINE__);
	else
		anyka_print("[%s:%d] set system user info success!!!\n", __func__, __LINE__);
}

static void* sys_update(void *user)
{
	//wait update confirm return to tencent
	sleep(2);
	system("/usr/sbin/update.sh");

	return NULL;
}

/**
 * NAME      ak_qq_OnUpdateConfirm
 * @BRIEF	on update confirm, when user tick the confirm button then we start update	
 * @PARAM	NONE
 * @RETURN	NONE
 * @RETVAL	
 */
void ak_qq_OnUpdateConfirm()
{
	pthread_t thread_id;
	anyka_print("[%s] [update confirm !!!]\n", __func__);
	/*	
	 *	change the status, 
	 *	when finished update use this flag to report to app
	 */
	ak_qq_change_firmware_update_status(1);		// open 1, close 0
	system("tar xf /tmp/update_pkg.tar -C /tmp/");
    if (anyka_pthread_create(&thread_id, sys_update, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, -1) != 0 )
    {
		anyka_print("create thread fail\n");
    }
	anyka_print("return from %s\n",__func__);
    
	
}

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
			anyka_print("[%s:%d] the %s exist\n",
				   	__func__, __LINE__, WIRE_LESS_TMP_DIR);
			/** check file exist **/
			if(access(gbk_path_name, F_OK) == 0)
				gbk_exist = 1;
			
			if(access(utf8_path_name, F_OK) == 0)
				utf8_exist = 1;	

			if(gbk_exist && utf8_exist) {
				anyka_print("[%s:%d] the wireless config has been saved," 
					"now do nothing\n", __func__, __LINE__);
				return 0;
			}
				
		} else {
			anyka_print("[%s:%d] make directory %s, %s\n", 
				__func__, __LINE__, WIRE_LESS_TMP_DIR, strerror(errno));
			return -1;
		}
	}


	if(utf8_exist == 0) {
		
		FILE *filp_utf8 = fopen(utf8_path_name, "w+");	
		if(!filp_utf8) {
			anyka_print("[%s:%d] open: %s, %s\n", 
					__func__, __LINE__, utf8_path_name, strerror(errno));
			return -1;
		}

		ret = fwrite(ssid, 1, ssid_len + 1, filp_utf8);
		if(ret != ssid_len + 1) {
			anyka_print("[%s:%d] fails write data\n", __func__, __LINE__);
			fclose(filp_utf8);
			return -1;
		}
		fclose(filp_utf8);
	}

	if(gbk_exist == 0) {

		/** utf-8 to gbk code change **/
		ret = u2g(ssid, ssid_len, gbk_ssid, 32);
		if(ret < 0) {
			anyka_print("[%s:%d] faile to change code from utf8 to gbk\n", 
				__func__, __LINE__);
			return -1;
		}
		anyka_print("*** u2g changed[%d], %s\n", ret, gbk_ssid);

		FILE *filp_gbk = fopen(gbk_path_name, "w+");
		if(!filp_gbk) {
			anyka_print("[%s:%d] open: %s, %s\n", 
					__func__, __LINE__, gbk_path_name, strerror(errno));
			return -1;
		}
		
		ret = fwrite(gbk_ssid, 1, strlen(gbk_ssid) + 1, filp_gbk);
		if(ret != strlen(gbk_ssid) + 1) {
			anyka_print("[%s:%d] fails write data\n", __func__, __LINE__);
			fclose(filp_gbk);
			return -1;
		}
		fclose(filp_gbk);
	}

	return 0;
}

static int voicelink_sys_wifi_info(char *ssid, char *passwd, char *ip, int port)
{
    Psystem_wifi_set_info p_wifi_info;

	/* save ssid to tmp */
	save_ssid_to_tmp(ssid, strlen(ssid));

    p_wifi_info = anyka_get_sys_wifi_setting();
    //strcpy(p_wifi_info->ssid, ssid);
    strcpy(p_wifi_info->passwd, passwd);
    anyka_set_sys_wifi_setting(p_wifi_info);

	FILE * file = fopen("/tmp/smartlink", "w");
	if (file)
	{
        fprintf(file, "%s\n%d\n", ip, port);
        fclose(file);
	}

	return 0;
}

static void voicelink_callback(tx_voicelink_param* pparam)
{
  
    anyka_print("###################################\n");
    anyka_print("[voicelink]ssid:%s\n", pparam->sz_ssid);
    anyka_print("[voicelink]password:%s\n", pparam->sz_password);
    anyka_print("[voicelink]ip:%s\n", pparam->sz_ip);
    anyka_print("[voicelink]port:%d\n", pparam->sh_port);
    anyka_print("###################################\n");

    voicelink_sys_wifi_info(pparam->sz_ssid, pparam->sz_password, pparam->sz_ip, pparam->sh_port);
}

void voicelink_get_audio_data(T_VOID *param, T_STM_STRUCT *pstream)
{
    tx_fill_audio((short int*)pstream->buf, pstream->size/2);
}

void ak_qq_start_voicelink()
{
    anyka_print("start voice link....\n");   
    tx_init_decoder(voicelink_callback, 8000);
    audio_add(SYS_AUDIO_RAW_PCM, voicelink_get_audio_data, NULL);
}

void ak_qq_stop_voicelink()
{
    anyka_print("stop voice link....\n");   
    audio_del(SYS_AUDIO_RAW_PCM, NULL);
    tx_uninit_decoder();
}

static ak_qq_video_info g_video_replay_send ;

extern unsigned long long g_base_time;

void ak_qq_send_video_replay_data(T_VOID *param, int type, T_STM_STRUCT *pstream)
{
	//毫秒为单位的时间戳
	pstream->timestamp =(unsigned int)param*1000 + pstream->timestamp - 8*3600*1000 - g_base_time;
    if(type == 1) //video data
    {
	    if(pstream->iFrame)
	    {
	        g_video_replay_send.iframe_index = 0;
	        g_video_replay_send.gop_index ++;
	    }
	    else
	    {
	        g_video_replay_send.iframe_index ++;
	    }
	    g_video_replay_send.frame_index ++;
	/*
		{
			FILE * fp = fopen("/mnt/video.data", "a");

			fwrite(pstream->buf+8,pstream->size-8,1, fp);

			fclose(fp);
		}
*/		
		tx_set_video_data(pstream->buf+8, pstream->size, pstream->iFrame?0:1, pstream->timestamp, g_video_replay_send.gop_index, g_video_replay_send.iframe_index, g_video_replay_send.frame_index, 30);
	}else  //audio data
	{
	    tx_audio_encode_param qq_audio_info;
	    int off = 0;

/*
		{
			FILE * fp = fopen("/mnt/audio.data", "a");

			fwrite(pstream->buf,pstream->size,1, fp);

			fclose(fp);
		}
		return ;
*/		
	    memset(&qq_audio_info, 0, sizeof(qq_audio_info));
	    qq_audio_info.head_length = sizeof(tx_audio_encode_param);
	    qq_audio_info.audio_format = 1;  // 音频数据的编码类型，这里是AMR	
	    qq_audio_info.encode_param = 7;  // AMR的mode
	    qq_audio_info.frame_per_pkg = 8;
	    qq_audio_info.sampling_info = GET_SIMPLING_INFO(1, 8000, 16);
	    
	    while(off < pstream->size)
	    {
	        tx_set_audio_data(&qq_audio_info, pstream->buf + off, 32);
	        off += 32;
	    }
	}
}

static unsigned int g_replay_time;
/**
* NAME     ak_qq_start_video_replay 
* @BRIEF   start video replay
* @PARAM   play_time : begin time 
* @RETURN  NONE
* @RETVAL  
*/
void ak_qq_start_video_replay(unsigned int play_time)
{
    unsigned long end_time;
    time_t tmp;
    struct tm *dest_time;
	
	tmp = play_time;
	
    dest_time = localtime(&tmp);
    dest_time->tm_hour = 0;
    dest_time->tm_sec = 0;
    dest_time->tm_min = 0;
    end_time = mktime(dest_time)+ 24*60*60; //最长24 小时
	
    memset(&g_video_replay_send, 0, sizeof(g_video_replay_send));
    g_video_replay_send.frame_index = -1;
    g_video_replay_send.gop_index = -1;
    g_video_replay_send.run_flag = 1;

	g_replay_time = play_time;
    record_replay_start(play_time, end_time, 1, RECORD_APP_PLAN, (void*)play_time, ak_qq_send_video_replay_data);

	return;
}

/**
* NAME     ak_qq_stop_video_replay 
* @BRIEF   stop video replay
* @PARAM   NONE
* @RETURN  NONE
* @RETVAL  
*/
void ak_qq_stop_video_replay(void)
{
	record_replay_stop((void*)g_replay_time);
}

