/*******************************************************************
此文件完成对讲功能，因目前大拿平台只是单工功能，所以这里
只完成对网络的音源处理。
*******************************************************************/


#include "common.h"
#include "audio_talk.h"
#include "sdcodec.h"

typedef struct _audio_talk_handle
{
    uint8 run_flag;
    PA_HTHREAD talk_accept;
    PA_HTHREAD talk_send;
    Pnet_get_talk_data *net_get_data_func;
    int   decode_buf_size;
    void  *decode_handle;
    void *da_handle;
    void *user_data;
    uint8 *decode_buf;
}audio_talk_handle, *Paudio_talk_handle;


Paudio_talk_handle paudio_talk = NULL;
/**
 * NAME         audio_talk_send_run
 * @BRIEF	接受网络数据,目前是对网络数据的查询，因没有触发
 * @PARAM	user 用户数据 
 * @RETURN	void *
 * @RETVAL	
 */

static void * audio_talk_accept_run(void * user )
{
    T_STM_STRUCT * pstream;
    int off, size;
    
    anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());
    
    while(paudio_talk->run_flag)
    {
        if((pstream = (T_STM_STRUCT * )paudio_talk->net_get_data_func(user))!= NULL)
        {
            off = 0;
            while(off < pstream->size)
            {
                size = audio_decode_add_data(paudio_talk->decode_handle, (uint8 *)pstream->buf + off, pstream->size - off); 
                off += size;
                if(off < pstream->size)
                {
                    usleep(100 *1000);
                }
            }
            free(pstream->buf);
            free(pstream);
        } 
        else
        {
            usleep(10 *1000);
        }
        
    }
    anyka_print( "[%s:%d]: talk is exit\n", __func__, __LINE__);

	return NULL;
}

/**
 * NAME         audio_talk_send_run
 * @BRIEF	对网络数据进行解码，并送到DA进行播放
 * @PARAM	user 用户数据 
 * @RETURN	void *
 * @RETVAL	
 */

static void * audio_talk_send_run( void *user )
{
    int out_sz, cur_size = 0;
    
	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());
    
    while(paudio_talk->run_flag)
    {
        out_sz = audio_decode_decode(paudio_talk->decode_handle, paudio_talk->decode_buf + cur_size, 2);
        cur_size += out_sz;
        if(cur_size >= (AUDIO_DECODE_MAX_SZ <<1))
        {
            audio_write_da(paudio_talk->da_handle, paudio_talk->decode_buf, cur_size);
            cur_size = 0;
        }
        if((0 == out_sz))
        {
            usleep(10 *1000);
        }
        
    }
    anyka_print("[%s:%d] talk send is exit\n", __func__, __LINE__);

	return NULL;
}

/**
 * NAME         audio_speak_start
 * @BRIEF	开启对讲功能,这个函数完成将AD数据采集，并编码送给网络
 * @PARAM	mydata    用户数据
                    pget_callback       得到网络数据的回调
                    encode_type     编码类型
 * @RETURN	0--fail; 1-->ok
 * @RETVAL	
 */

void audio_speak_start(void *mydata, AUDIO_SEND_CALLBACK *pcallback, int encode_type)
{
    audio_add(encode_type, pcallback, mydata);
}

/**
 * NAME         audio_speak_stop
 * @BRIEF	停止对讲功能,关闭AD数据编码
 * @PARAM	encode_type     编码类型
 * @RETURN	void
 * @RETVAL	
 */

void audio_speak_stop(void *mydata, int encode_type)
{
    audio_del(encode_type, mydata);
}

/**
 * NAME         audio_talk_start
 * @BRIEF	开启对讲功能，这个函数完成接受网络数据，并解码，送给DA
 * @PARAM	mydata    用户数据
                    pget_callback       得到网络数据的回调
                    encode_type     编码类型
 * @RETURN	0--fail; 1-->ok
 * @RETVAL	
 */

uint8 audio_talk_start(void *mydata, Pnet_get_talk_data pget_callback, int encode_type)
{
	T_S32	ret = 0;
    T_AUDIO_INPUT audio_info;
    media_info talk_media_info;

    if(paudio_talk)
    {
        anyka_print("[%s:%d] fails to start talk beacuse it runs!\n", __func__, __LINE__);
        return 0;
    }
    paudio_talk = (Paudio_talk_handle)malloc(sizeof(audio_talk_handle));
    if(paudio_talk == NULL)
    {
        return 0;
    }
    paudio_talk->user_data = mydata;
    paudio_talk->net_get_data_func = pget_callback;
    paudio_talk->decode_buf = malloc(8192);
    audio_info.nBitsPerSample = 16;
    audio_info.nChannels = 1;
    audio_info.nSampleRate = 8000;
    paudio_talk->da_handle = audio_da_open(&audio_info);
    audio_da_spk_enable(1);
    audio_da_filter_enable(paudio_talk->da_handle, 1);
	audio_set_da_vol(paudio_talk->da_handle, 5);
	switch(encode_type)
	{
        case SYS_AUDIO_ENCODE_AAC:
	        talk_media_info.m_AudioType = _SD_MEDIA_TYPE_AAC;
            break;
            
        case SYS_AUDIO_ENCODE_G711:
	        talk_media_info.m_AudioType = _SD_MEDIA_TYPE_PCM_ALAW;
            break;
            
        case SYS_AUDIO_ENCODE_AMR:
	        talk_media_info.m_AudioType = _SD_MEDIA_TYPE_AMR;
            break;
	}
	talk_media_info.m_nChannels = 1;
	talk_media_info.m_nSamplesPerSec = 8000;
	talk_media_info.m_wBitsPerSample = 16;
    paudio_talk->decode_handle = audio_decode_open(&talk_media_info);
	switch(encode_type)
	{            
        case SYS_AUDIO_ENCODE_G711:
	        audio_decode_change_mode(paudio_talk->decode_handle);
            break;
            
        case SYS_AUDIO_ENCODE_AMR:
	       // audio_decode_change_mode(paudio_talk->decode_handle);
            audio_decode_set_buf(paudio_talk->decode_handle, 32);
            break;
	}
    
    paudio_talk->decode_buf_size = audio_decode_get_free(paudio_talk->decode_handle);
    paudio_talk->run_flag = 1;    
	ret = anyka_pthread_create(&(paudio_talk->talk_accept), audio_talk_accept_run, 
			mydata, ANYKA_THREAD_MIN_STACK_SIZE, -1);
    if((ret) != 0) {
    	anyka_print("[%s:%d] fails to create thread, ret = %ld\n", __func__, __LINE__, ret);
    	return 0;
    }
	ret = anyka_pthread_create(&(paudio_talk->talk_send), audio_talk_send_run, 
			mydata, ANYKA_THREAD_MIN_STACK_SIZE, -1);
    if((ret) != 0) {
    	anyka_print("[%s:%d]fails to create thread, ret = %ld\n",__func__, __LINE__, ret);
    	return 0;	
    }
    return 1;
}
/**
 * NAME         audio_talk_status
 * @BRIEF	检查编码缓冲区是不是为空的状态
 * @PARAM	
 * @RETURN	0-->为空，1-->不为空 
 * @RETVAL	
 */

int audio_talk_status(void)
{
    if(paudio_talk == NULL)
    {
        return 0;
    }

    if(paudio_talk->decode_buf_size < audio_decode_get_free(paudio_talk->decode_handle) + 32)
    {
        return 0;
    }
    else
    {
        return 1;
    }
    
}
/**
 * NAME         audio_talk_stop
 * @BRIEF	停止对讲功能,将不再接受网络数据
 * @PARAM	
 * @RETURN	void
 * @RETVAL	
 */

void audio_talk_stop(void *mydata)
{
    if(paudio_talk && paudio_talk->user_data == mydata)
    {
        paudio_talk->run_flag = 0;
        pthread_join(paudio_talk->talk_accept, NULL);
        pthread_join(paudio_talk->talk_send, NULL);        
        audio_da_spk_enable(0);
        audio_da_close(paudio_talk->da_handle);
        audio_decode_close(paudio_talk->decode_handle);
        free(paudio_talk->decode_buf);
        free(paudio_talk);
        paudio_talk = NULL;
    }
}




