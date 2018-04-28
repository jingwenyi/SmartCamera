/*
**	filename: audio_analyses_algorithm.h
**  author: jingwenyi
**  date: 2016.07.04
**
*/

#ifndef _AUDIO_ANALYSES_ALGORITHM_H_
#define _AUDIO_ANALYSES_ALGORITHM_H_


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>


typedef void audio_analyses_callfunc(unsigned char *buffer, int len);


typedef enum{
	ANALYZE_NO_DATA,
	ANALYZE_START_RUNING,
	ANALYZE_RUNING,
	ANALYZE_END_RUNING
}analyze_stat;



enum{
	ERR_OK,
	ERR_MALLOC_FAILED,
	ERR_INPUT_NULL
};


typedef struct guess_data{
	uint8_t* data;
	int len;
	analyze_stat status;
	int start_flag;
	int end_flag;
	audio_analyses_callfunc *call_func;
}GUESS_DATA, *PGUESS_DATA; 


int audio_analyze_init(audio_analyses_callfunc *call_func);

void audio_analyze_exit();

void audio_bzero_voice_data();

int audio_frame_analyze1(uint8_t* frame, int frame_len);


int audio_frame_analyze2(uint8_t* frame, int frame_len);




#endif //_AUDIO_ANALYSES_ALGORITHM_H_
