#ifndef _audio_hd_h_
#define _audio_hd_h_
#include "basetype.h"
#include "audio_ctrl.h"

void * audio_da_open(T_AUDIO_INPUT *input);
int audio_write_da(void *handle, uint8 *pdata, int buf_size);
int audio_da_close(void *handle);
void *audio_ad_open(T_AUDIO_INPUT *input);

/*
  * vol: 0~7, 数值越大音量越大
*/
void audio_set_mic_vol(void *handle, int vol);

/*
  * vol: 0~5, 数值越大音量越大
*/
void audio_set_da_vol(void *handle, int vol);
int audio_read_ad(void *handle, uint8 *pdata, int buf_size, unsigned long *ts);
int audio_ad_close(void *handle);
void audio_set_smaple(void *handle, T_AUDIO_INPUT *input);
void audio_da_filter_enable(void *handle, int enable);
void audio_ad_filter_enable(void *handle, int enable);
void audio_da_spk_enable(int enable);
void audio_ad_clr_buffer(void *handle);
void audio_da_clr_buffer(void *handle);
int audio_set_ad_agc_enable(int enable);
int audio_set_da_agc_enable(int enable);
int audio_get_da_status(void);


#endif



