#include "common.h"

/**************************************************************
此模块只播放AMR格式的文件，目前设计不考虑与其它模块对DA的冲突使用

***************************************************************/
    
typedef struct _audio_play_ctrl_
{
    PA_HTHREAD thread_id;
    sem_t tell_end;
    char *file_name;
    int end;    
    int fd;
    int del_flag;
}audio_play_ctrl_info, *Paudio_play_ctrl_info;


static Paudio_play_ctrl_info paudio_play_ctrl = NULL;

/**
 * NAME         audio_play_get__data
 * @BRIEF	读音频数据
 * @PARAM	user   无意义
 * @RETURN	NONE
 * @RETVAL	
 */

void * audio_play_get__data(void * user )
{
    T_STM_STRUCT * pstream;

    if(paudio_play_ctrl==NULL || paudio_play_ctrl->end)
    {
        return NULL;
    }
    
    pstream = anyka_malloc_stream_ram(1024);
    if(pstream == NULL)
    {
        return NULL;
    }

    pstream->size = read(paudio_play_ctrl->fd, pstream->buf, 1024);

    if(pstream->size < 1024)
    {
        if(pstream->size == 0)
        {
            anyka_free_stream_ram(pstream);
            pstream = NULL;
        }
        paudio_play_ctrl->end = 1;
        sem_post(&paudio_play_ctrl->tell_end);
    }

    return pstream;    
}


/**
 * NAME         audio_play_wait_end
 * @BRIEF	等待音乐播放结束，并释放相关资源
 * @PARAM	param   无意义
 * @RETURN	NONE
 * @RETVAL	
 */


void* audio_play_wait_end(void* param)
{
    sem_wait(&paudio_play_ctrl->tell_end);
    while(audio_talk_status() || (paudio_play_ctrl->end == 0))
    {
        usleep(100 *1000);
    }

	while(audio_get_da_status()){
        usleep(100 *1000);		
	}
	
    audio_talk_stop(paudio_play_ctrl);
    close(paudio_play_ctrl->fd);
    sem_destroy(&paudio_play_ctrl->tell_end);
    if(paudio_play_ctrl->del_flag){
		remove(paudio_play_ctrl->file_name);
    }
    free(paudio_play_ctrl->file_name);
    free(paudio_play_ctrl);
    paudio_play_ctrl = NULL;   
    anyka_print("[%s:%d] the audio play is over!\n", __func__, __LINE__);
    pthread_exit(NULL);
}

/**
 * NAME         audio_play_start
 * @BRIEF	开始AMR的音乐播放，目前只做了AMR文件的播放
 * @PARAM	audio_path   AMR音乐路径
                    del_flag 是否删除文件
 * @RETURN	NONE
 * @RETVAL	
 */


void audio_play_start(char *audio_path, int del_flag)
{
    char file_head[6];
    const T_U8 amrHeader[]= "#!AMR\n";

    if(paudio_play_ctrl)
    {
        anyka_print("[%s:%d] fails to start beacuse started!\n", __func__, __LINE__);
        return ;
    }

    paudio_play_ctrl = (Paudio_play_ctrl_info)malloc(sizeof(audio_play_ctrl_info));
    paudio_play_ctrl->fd = open(audio_path, O_RDONLY);
    if(paudio_play_ctrl->fd < 0)
    {
        anyka_print("[%s:%d] fails to open src file!\n", __func__, __LINE__);
        free(paudio_play_ctrl);
        paudio_play_ctrl = NULL;
        return;
    }
    paudio_play_ctrl->del_flag = del_flag;
    read(paudio_play_ctrl->fd, file_head, 6);
    if(memcmp(file_head, amrHeader, 6))
    {
        anyka_print("[%s:%d] it isn't the AMR file!\n", __func__, __LINE__);
        close(paudio_play_ctrl->fd);
        free(paudio_play_ctrl);
        paudio_play_ctrl = NULL;
        return ;
    }
    paudio_play_ctrl->file_name = malloc(strlen(audio_path) + 2);
    strcpy(paudio_play_ctrl->file_name, audio_path);
    paudio_play_ctrl->end = 0;
    sem_init(&paudio_play_ctrl->tell_end, 0, 0);
    audio_talk_start(paudio_play_ctrl, audio_play_get__data, SYS_AUDIO_ENCODE_AMR);    
    anyka_pthread_create(&paudio_play_ctrl->thread_id, audio_play_wait_end, paudio_play_ctrl, ANYKA_THREAD_MIN_STACK_SIZE, -1);
}

/**
 * NAME         audio_play_stop
 * @BRIEF	停止当前音乐的播放
 * @PARAM	void
 * @RETURN	NONE
 * @RETVAL	
 */

void audio_play_stop(void)
{    
    if(paudio_play_ctrl == NULL)
    {
        return ;
    } 
    anyka_print("test stop start signal!\n");
    paudio_play_ctrl->end = 2;
    sem_post(&paudio_play_ctrl->tell_end);
}

