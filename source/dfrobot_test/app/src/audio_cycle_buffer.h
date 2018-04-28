/*
** filename: audio_cyle_buffer.h
** author: jingwenyi
** date: 2016.06.20
*/

#ifndef AUDIO_CYCLE_BUFFER_H_
#define AUDIO_CYCLE_BUFFER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>




#ifdef __cplusplus
extern "C" {
#endif


typedef struct CYCLE_BUFF
{
	int stepsize;
	int status;
	long int producte;
	long int consume;
	long int count;
	pthread_mutex_t mutex;
	uint8_t *data;

}dfrobot_cycle_buff, *Pdfrobot_cycle_buff;


dfrobot_cycle_buff* cycle_buff_init();

void cycle_buff_destroy(dfrobot_cycle_buff *cycle_buff);

//write a audio data
void write_cycle_buff(dfrobot_cycle_buff *cycle_buff, unsigned char *data, long int data_len);

//read a audio data
int read_cycle_buff(dfrobot_cycle_buff *cycle_buff, unsigned char *buff);



#ifdef __cplusplus
} /* end extern "C" */
#endif



#endif //AUDIO_CYCLE_BUFFER_H_






