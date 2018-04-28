/*
** filename:audio_cycle_buffer.c
** author: jingwenyi
** date: 2016.6.20
*/


#include "audio_cycle_buffer.h"

#define  MAIN_BUF_SIZE             		(1*1024*1024)



dfrobot_cycle_buff* cycle_buff_init()
{
	dfrobot_cycle_buff *cycle_buff = (dfrobot_cycle_buff*) malloc(sizeof(dfrobot_cycle_buff));
	if(cycle_buff == NULL)
	{
		return NULL;
	}
	cycle_buff->stepsize = 0;
	cycle_buff->status = 0;
	cycle_buff->consume = 0;
	cycle_buff->producte = 0;
	cycle_buff->count = 0;
	cycle_buff->data = (uint8_t*) malloc(MAIN_BUF_SIZE);
	if(cycle_buff->data == NULL)
	{
		free(cycle_buff);
		return NULL;
	}
	pthread_mutex_init(&(cycle_buff->mutex), NULL);
	return cycle_buff;
}

void cycle_buff_destroy(dfrobot_cycle_buff *cycle_buff)
{
	if(cycle_buff != NULL)
	{
		if(cycle_buff->data != NULL)
		{
			free(cycle_buff->data);
		}
		pthread_mutex_destroy(&(cycle_buff->mutex));
		free(cycle_buff);
	}
	
}


//write a audio data
void write_cycle_buff(dfrobot_cycle_buff *cycle_buff, unsigned char *data, long int data_len)
{
	if(cycle_buff->stepsize != data_len)
	{
		printf("write cycle buff data len error\r\n");
		return;
	}
	
	pthread_mutex_lock(&(cycle_buff->mutex));
	//TODO:
	if((cycle_buff->producte + cycle_buff->stepsize) < MAIN_BUF_SIZE)
	{
		memcpy(&cycle_buff->data[cycle_buff->producte], data, cycle_buff->stepsize);
		cycle_buff->producte += cycle_buff->stepsize;
		
		if((cycle_buff->status == 1) && (cycle_buff->producte > cycle_buff->consume))
		{
			if(cycle_buff->consume + cycle_buff->stepsize < MAIN_BUF_SIZE)
			{
				cycle_buff->consume += cycle_buff->stepsize;
			}
			else
			{
				cycle_buff->consume = cycle_buff->stepsize - (MAIN_BUF_SIZE - cycle_buff->consume);
				cycle_buff->status = 0;
			}
			cycle_buff->count--;
		}
	}
	else
	{
		
		int tmp_len = MAIN_BUF_SIZE - cycle_buff->producte;
		if(cycle_buff->status == 1)
		{
			cycle_buff->consume = cycle_buff->stepsize - (MAIN_BUF_SIZE - cycle_buff->consume);
			cycle_buff->status = 0;
			cycle_buff->count--;
		}
		memcpy(&cycle_buff->data[cycle_buff->producte], data, tmp_len);
		
		if(cycle_buff->stepsize-tmp_len > 0)
		{
			memcpy(&cycle_buff->data[0], &data[tmp_len],(cycle_buff->stepsize-tmp_len));
		}
		
		cycle_buff->producte = cycle_buff->stepsize-tmp_len;
		if(cycle_buff->producte > cycle_buff->consume)
		{
			cycle_buff->consume += cycle_buff->stepsize;
			cycle_buff->count--;
		}
		cycle_buff->status = 1;
	}
	cycle_buff->count++;
	pthread_mutex_unlock(&(cycle_buff->mutex));
	
}



//read a audio data
int read_cycle_buff(dfrobot_cycle_buff *cycle_buff, unsigned char *buff)
{
	
	pthread_mutex_lock(&(cycle_buff->mutex));
	//TODO:
	if(cycle_buff->status == 0)
	{
		//if(cycle_buff->producte > cycle_buff->consume)
		if(cycle_buff->count > 0)
		{
			memcpy(buff, &cycle_buff->data[cycle_buff->consume], cycle_buff->stepsize);
			cycle_buff->consume += cycle_buff->stepsize;
		}
		else
		{
			//printf("no audio data\r\n");
			pthread_mutex_unlock(&(cycle_buff->mutex));
			return 0;
		}
	}
	else
	{
		if((cycle_buff->consume + cycle_buff->stepsize) < MAIN_BUF_SIZE)
		{
			memcpy(buff, &cycle_buff->data[cycle_buff->consume], cycle_buff->stepsize);
			cycle_buff->consume += cycle_buff->stepsize;
		}
		else
		{
			int tmp_len = MAIN_BUF_SIZE - cycle_buff->consume;
			memcpy(buff, &cycle_buff->data[cycle_buff->consume], tmp_len);
			if((cycle_buff->stepsize - tmp_len) > 0)
			{
				memcpy(&buff[tmp_len], &cycle_buff->data[0], (cycle_buff->stepsize - tmp_len));
			}

			cycle_buff->consume = cycle_buff->stepsize - tmp_len;
			
			cycle_buff->status = 0;
		}
	}
	cycle_buff->count--;
	pthread_mutex_unlock(&(cycle_buff->mutex));

	return cycle_buff->stepsize;
}






