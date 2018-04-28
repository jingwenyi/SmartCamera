/*
** filename: audio_list.c
** author: jingwenyi
** date: 2016.06.21
**
*/

#include "audio_list.h"


dfrobot_auido_list_header* list_header_init()
{
	dfrobot_auido_list_header* header = (dfrobot_auido_list_header*)malloc(sizeof(dfrobot_auido_list_header));
	if(header == NULL)
	{
		return NULL;
	}
	header->head = NULL;
	header->tail = NULL;
	header->list_size = 0;
	pthread_mutex_init(&(header->mutex), NULL);
	return header;
}

dfrobot_audio_list* list_init(int len)
{
	if(len <= 0)
	{
		return NULL;
	}
	dfrobot_audio_list* list = (dfrobot_audio_list*)malloc(sizeof(dfrobot_audio_list));
	if(list == 	NULL)
	{
		return NULL;
	}
	list->data = (uint8_t*)malloc(len);
	if(list->data == NULL)
	{
		free(list);
		return NULL;
	}
	list->len = len;
	list->next = NULL;
	return list;
}

void list_add_tail(dfrobot_auido_list_header *header, dfrobot_audio_list *list)
{
	if((header == NULL) || (list == NULL))
	{
		return;
	}
	pthread_mutex_lock(&(header->mutex));
	
	if(header->list_size == 0)
	{
		header->head = list;
		header->tail = list;
	}
	else
	{
		header->tail->next = list;
		header->tail = list;
	}
	header->list_size++;
	
	pthread_mutex_unlock(&(header->mutex));
}

void list_delete_head(dfrobot_auido_list_header *header)
{
#if 0
	if(header == NULL) return;
	if(header->head == NULL) return;
	
	pthread_mutex_lock(&(header->mutex));
	
	dfrobot_audio_list* list = header->head;
	header->head = list->next;
	header->list_size--;
	if(header->list_size == 0)
	{
		header->head = NULL;
		header->tail = NULL;
	}
	list_destroy(list);
	list = NULL;
	pthread_mutex_unlock(&(header->mutex));
#endif
	dfrobot_audio_list* list = list_pop_head(header);
	list_destroy(list);
	list = NULL;

}

dfrobot_audio_list* list_pop_head(dfrobot_auido_list_header *header)
{
	if(header == NULL) return NULL;
	if(header->head == NULL) return NULL;
	
	pthread_mutex_lock(&(header->mutex));
	
	dfrobot_audio_list* list = header->head;
	header->head = list->next;
	header->list_size--;
	if(header->list_size == 0)
	{
		header->head = NULL;
		header->tail = NULL;
	}
	
	pthread_mutex_unlock(&(header->mutex));

	return list;
}

void list_destroy(dfrobot_audio_list* list)
{
	if(list == NULL) return;
	
	if(list->data != NULL)
	{
		free(list->data);
		list->data = NULL;
	}
	list->next = NULL;
	free(list);
	list = NULL;
}

void list_header_destroy(dfrobot_auido_list_header *header)
{
	if(header == NULL) return;
	while(header->head != NULL)
	{
		dfrobot_audio_list* list = header->head;
		header->head = list->next;
		list_destroy(list);
		list = NULL;
	}
	header->head = NULL;
	header->tail = NULL;
	pthread_mutex_destroy(&(header->mutex));
	free(header);
	header = NULL;
}

