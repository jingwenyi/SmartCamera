/*
**filename:audio_list.h
**author:jingwenyi
**date: 2016.06.20
*/

#ifndef _AUDIO_LIST_H_
#define _AUDIO_LIST_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct AUDIO_LIST
{
	uint8_t* data;
	long int len;
	struct AUDIO_LIST* next;
}dfrobot_audio_list, *Pdfrobot_audio_list;


typedef struct AUDIO_LIST_HEADER
{
	dfrobot_audio_list* head;
	dfrobot_audio_list* tail;
	pthread_mutex_t mutex;
	int list_size;
}dfrobot_auido_list_header, *Pdfrobot_audio_list_header;

dfrobot_auido_list_header* list_header_init();

dfrobot_audio_list* list_init(int len);

void list_add_tail(dfrobot_auido_list_header *header, dfrobot_audio_list *list);

void list_delete_head(dfrobot_auido_list_header *header);

void list_destroy(dfrobot_audio_list* list);

void list_header_destroy(dfrobot_auido_list_header *header);

dfrobot_audio_list* list_pop_head(dfrobot_auido_list_header *header);


#ifdef __cplusplus
} /* end extern "C" */
#endif


#endif //_AUDIO_LIST_H_

