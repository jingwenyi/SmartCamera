#include "common.h"
#include "pcm_lib.h"
#include "audio_hd.h"

/**
 * NAME         audio_da_open
 * @BRIEF	打开 DA
                  
 * @PARAM	T_AUDIO_INPUT *input，打开时设置的参数
                    
 * @RETURN	void *
 * @RETVAL	指向打开后的句柄
 */

void * audio_da_open(T_AUDIO_INPUT *input)
{
    void *handle;
    pcm_pars_t da_info;
    handle = pcm_open(PCM_DEV_DA);

    da_info.samplebits = input->nBitsPerSample;
    da_info.samplerate = input->nSampleRate;
    pcm_ioctl(handle, DA_SET_PARS, (void *)&da_info);
    return handle;
}


/**
 * NAME         audio_da_filter_enable
 * @BRIEF	开关DA 重采样
                  
 * @PARAM	void *handle,打开时返回的句柄
 			int enable,	1，开；0，关
                    
 * @RETURN	void 
 * @RETVAL	
 */

void audio_da_filter_enable(void *handle, int enable)
{
    pcm_ioctl(handle, DA_SET_SDF, (void *)&enable);
}

/**
 * NAME         audio_da_spk_enable
 * @BRIEF	开关DA pa
                  
 * @PARAM	
 			int enable,	1，开；0，关
 * @RETURN	void 
 * @RETVAL	
 */

void audio_da_spk_enable(int enable)
{
    pcm_set_speaker_enable(enable);	
}

/**
 * NAME         audio_write_da
 * @BRIEF	写DA
                  
 * @PARAM	void *handle,  打开时返回的句柄 
 			uint8 *pdata,  要写的数据
 			int buf_size,    数据的大小
 			
 * @RETURN	int 
 * @RETVAL	实际写入DA 的数据大小
 */

int audio_write_da(void *handle, uint8 *pdata, int buf_size)
{
    return pcm_write(handle, pdata, buf_size);
}


/**
 * NAME         audio_da_close
 * @BRIEF	关闭DA
                  
 * @PARAM	void *handle,  打开时返回的句柄 

 * @RETURN	int 
 * @RETVAL	 if return 0 success, otherwise failed
 */

int audio_da_close(void *handle)
{
	return pcm_close(handle);
}

/**
 * NAME         audio_da_open
 * @BRIEF	打开 AD
                  
 * @PARAM	T_AUDIO_INPUT *input，打开时设置的参数
                    
 * @RETURN	void *
 * @RETVAL	指向打开后的句柄
 */
void *audio_ad_open(T_AUDIO_INPUT *input)
{
    void *handle;
    pcm_pars_t da_info;
    handle = pcm_open(PCM_DEV_AD);

    da_info.samplebits = input->nBitsPerSample;
    da_info.samplerate = input->nSampleRate;
    pcm_ioctl(handle, AD_SET_PARS, (void *)&da_info);    
    return handle;
}


/**
 * NAME         audio_ad_filter_enable
 * @BRIEF	开关AD 重采样
                  
 * @PARAM	void *handle,打开时返回的句柄
 			int enable,	1，开；0，关
                    
 * @RETURN	void 
 * @RETVAL	
 */

void audio_ad_filter_enable(void *handle, int enable)
{
    pcm_ioctl(handle, AD_SET_SDF, (void *)&enable);
}

/**
 * NAME        audio_ad_clr_buffer
 * @BRIEF	清空AD 缓冲区BUF
                  
 * @PARAM	void *handle,打开时返回的句柄
 * @RETURN	void 
 * @RETVAL	
 */
void audio_ad_clr_buffer(void *handle)
{
    pcm_ioctl(handle, SET_RSTBUF, (void *)~0);
}


/**
 * NAME        audio_da_clr_buffer
 * @BRIEF	清空DA 缓冲区BUF
                  
 * @PARAM	void *handle,打开时返回的句柄
 * @RETURN	void 
 * @RETVAL	
 */

void audio_da_clr_buffer(void *handle)
{
    pcm_ioctl(handle, SET_RSTBUF, (void *)~0);
}

/**
 * NAME        audio_set_smaple
 * @BRIEF	设置ad 采样率
                  
 * @PARAM	void *handle,打开时返回的句柄
			T_AUDIO_INPUT *input, 指向参数指针
 * @RETURN	void 
 * @RETVAL	
 */

void audio_set_smaple(void *handle, T_AUDIO_INPUT *input)
{
    pcm_pars_t ad_info;
 
    ad_info.samplebits = input->nBitsPerSample;
    ad_info.samplerate = input->nSampleRate;
    pcm_ioctl(handle, AD_SET_PARS, (void *)&ad_info);
}

/**
 * NAME        audio_set_mic_vol
 * @BRIEF	设置ad 采样音量
                  
 * @PARAM	void *handle,打开时返回的句柄
			 int vol，要设置的音量
 * @RETURN	void 
 * @RETVAL	
 */

/*
  * vol: 0~7, 数值越大音量越大
*/
void audio_set_mic_vol(void *handle, int vol)
{
    pcm_ioctl(handle, AD_SET_GAIN, (void *)&vol);
}


/**
 * NAME        audio_set_da_vol
 * @BRIEF	设置DA  采样音量
                  
 * @PARAM	void *handle,打开时返回的句柄
			 int vol，要设置的音量
 * @RETURN	void 
 * @RETVAL	
 */
/*
  * vol: 0~5, 数值越大音量越大
*/
void audio_set_da_vol(void *handle, int vol)
{
    pcm_ioctl(handle, DA_SET_GAIN, (void *)&vol);
}

/**
 * NAME        audio_read_ad
 * @BRIEF	读AD 缓冲区数据
                  
 * @PARAM	void *handle,打开时返回的句柄
			uint8 *pdata, 数据存放地址
			int buf_size, 要读的size
			unsigned long *ts, 时间戳
 * @RETURN	int  
 * @RETVAL	if return 0 success, otherwise failed
 */
int audio_read_ad(void *handle, uint8 *pdata, int buf_size, unsigned long *ts)
{
	int ret;
    
    ret = pcm_read(handle, pdata, buf_size);
	
	pcm_get_timer(handle, ts);

	return ret;
}

/**
 * NAME        audio_ad_close
 * @BRIEF	关闭AD
                  
 * @PARAM	void *handle,打开时返回的句柄

 * @RETURN	int  
 * @RETVAL	if return 0 success, otherwise failed
 */

int audio_ad_close(void *handle)
{
	return pcm_close(handle);
}


/**
 * NAME        audio_set_ad_agc_enable
 * @BRIEF	AD agc 自动增益控制开关
                  
 * @PARAM	int enable,	1，开；0，关
 * @RETURN	int  
 * @RETVAL	if return 0 success, otherwise failed
 */

int audio_set_ad_agc_enable(int enable)
{
	return pcm_set_nr_agc_enable(PCM_DEV_AD, enable);
}

/**
 * NAME        audio_set_da_agc_enable
 * @BRIEF	DA agc 自动增益控制开关
                  
 * @PARAM	int enable,	1，开；0，关
 * @RETURN	int  
 * @RETVAL	if return 0 success, otherwise failed
 */

int audio_set_da_agc_enable(int enable)
{
    return 0;
	return pcm_set_nr_agc_enable(PCM_DEV_DA, enable);
}

/**
 * NAME        audio_get_ad_status
 * @BRIEF	获取AD 状态
                  
 * @PARAM	int enable,	1，开；0，关
 * @RETURN	int  
 * @RETVAL	if return 0 success, otherwise failed
 */

int audio_get_ad_status(void)
{
	return pcm_get_status(PCM_DEV_AD);
}


/**
 * NAME        audio_get_da_status
 * @BRIEF	获取DA 状态
                  
 * @PARAM	void
 * @RETURN	int  
 * @RETVAL	if return 0 don't working, otherwise working.
 */
int audio_get_da_status(void)
{
	return pcm_get_status(PCM_DEV_DA);
}


