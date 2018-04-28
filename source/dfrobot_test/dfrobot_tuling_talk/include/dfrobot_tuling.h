/*
** filename: dfrobot_tuling.h
** author: jingwenyi
** date: 2016.06.24
*/

#ifndef  _DFROBOT_TULING_H_
#define	 _DFROBOT_TULING_H_

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include "curl.h"

#define DFROBOT_BAIDU_APIID			"5882776"
#define	DFROBOT_BAIDU_APIKEY		"kxEPGh7TXL3javiTkCsoRlfG"
#define	DFROBOT_BAIDU_APISECRTKEY	"8fdc8211c0fadf4cfeab7f4e9647823c"
#define	DFROBOT_BAIDU_LANGUAGE		"zh"


#define DFROBOT_TULING_APIKEY  	"35fb2fb9567aec3f9d595bfef8d7cfc8"
#define	DFROBOT_TULING_USERKEY 	"88858"
#define TULING_HEADER_SIZE		100
#define	RECEIVE_BUFFER_SIZE		(100*1024)      //100K 

#ifdef __cplusplus
extern "C" {
#endif


typedef  size_t curl_callback_func(void *ptr, size_t size, size_t nmemb, char **result);


/*
** parameter
** pcm_data: user say to tuling audio data
** pcm_data_len: audio data length
** func: respond mp3 audio data
*/
void talk_to_talk_with_tuling(uint8_t *pcm_data, int pcm_data_len, curl_callback_func *func);

/*
** parameter
** pcm_data: audio pcm data
** pcm_data_len: audio date lenght
** return: baidu asr back text
*/
char *baidu_asr_pcm2text(uint8_t *pcm_data, int pcm_data_len);


/*
** parameter
** text:tuling only accept text 
** return: tuling back text
**
*/
char *talk_to_tuling(char *text);

/*
** parameter
** text: the text that need change to audio
** func: baidu tts is back mp3 audio data
** 
*/

void baidu_tts_text2mp3(char *text, curl_callback_func *func);


bool set_baidu_api(const char *ApiId, const char *ApiKey, const char *ApiSecrtkey);

bool set_tuling_api(const char *apikey, const char *userkey);

void tuling_exit();






#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif  //_DFROBOT_TULING_H_
