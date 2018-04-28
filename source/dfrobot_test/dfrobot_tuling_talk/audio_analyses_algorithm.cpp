/*
** filename: audio_analyses_algorithm.c
** author: jingwenyi
** date: 2016.07.04
**
*/

#include "audio_analyses_algorithm.h"

#define NOISE_THRESHOLD	150000
#define VOICE_MAX_SIZE  (1*1024*1024)
#define	VOICE_START_OK	5
#define VOICE_END_OK	20

GUESS_DATA *voice_data = NULL;



int audio_analyze_init(audio_analyses_callfunc *call_func)
{
	voice_data = (GUESS_DATA*)malloc(sizeof(GUESS_DATA));
	if(voice_data == NULL)
	{
		printf("voice_data malloc error\r\n");
		return ERR_MALLOC_FAILED;
	}

	voice_data->len = 0;
	voice_data->data = (uint8_t*)malloc(VOICE_MAX_SIZE);
	voice_data->status = ANALYZE_NO_DATA;
	voice_data->end_flag = 0;
	voice_data->start_flag = 0;
	if(voice_data->data == NULL)
	{
		printf("voice_data data malloc error\r\n");
		free(voice_data);
		return ERR_MALLOC_FAILED;
	}

	voice_data->call_func = call_func;
	
	return ERR_OK;
}


void audio_analyze_exit()
{
	if(voice_data != NULL)
	{
		if(voice_data->data != NULL)
		{
			free(voice_data->data);
			voice_data->data = NULL;
		}
		free(voice_data);
		voice_data = NULL;
	}
}

void audio_bzero_voice_data()
{
	voice_data->status = ANALYZE_NO_DATA;
	voice_data->end_flag = 0;
	voice_data->len = 0;
	voice_data->start_flag = 0;
}

int audio_frame_analyze1(uint8_t* frame, int frame_len)
{
	 long long int sum = 0;
	 if(frame == NULL)
	 {
		printf("frame is null\r\n");
		return ERR_INPUT_NULL;
	 }
	
	 int i;
	 uint8_t *tmp = frame;
	 unsigned char *sample = (unsigned char *)malloc(2);
	 for(i=0; i<frame_len; )
	 {
	 	sample[0] = tmp[i++];
		sample[1] = tmp[i++];
	 	short int *samplenum = NULL;
	 	samplenum = (short *)sample;
		//printf("samplenum = %d\r\n",*samplenum);
		sum += abs(*samplenum);
	 }
	 free(sample);

	 if(sum >= NOISE_THRESHOLD)
	 {
	 	if(voice_data->status == ANALYZE_RUNING)
	 	{
			memcpy(&voice_data->data[voice_data->len], frame, frame_len);
			voice_data->len += frame_len;
		}
	 	else if(voice_data->status == ANALYZE_START_RUNING)
		{
			memcpy(&voice_data->data[voice_data->len], frame, frame_len);
			voice_data->len += frame_len;
			if((++voice_data->start_flag)> VOICE_START_OK)
			{
				voice_data->status = ANALYZE_RUNING;
			}
		}
		else if(voice_data->status== ANALYZE_NO_DATA)
		{
			voice_data->status = ANALYZE_START_RUNING;
			memcpy(&voice_data->data[voice_data->len], frame, frame_len);
			voice_data->len += frame_len;
			voice_data->start_flag++;
		}
		else if(voice_data->status == ANALYZE_END_RUNING)
		{
			memcpy(&voice_data->data[voice_data->len], frame, frame_len);
			voice_data->len += frame_len;
			if((--voice_data->end_flag) <= 0)
			{
				voice_data->end_flag = 0;
				voice_data->status = ANALYZE_RUNING;
			}
		}
		
	 }
	 else
	 {
		if(voice_data->status == ANALYZE_START_RUNING)
		{
			voice_data->status = ANALYZE_NO_DATA;
			voice_data->start_flag = 0;
			voice_data->len = 0;
		}
		else if(voice_data->status == ANALYZE_END_RUNING)
		{
			memcpy(&voice_data->data[voice_data->len], frame, frame_len);
			voice_data->len += frame_len;
			if((++voice_data->end_flag) > VOICE_END_OK)
			{
				#if 0
				char buf[50] = {0};
				sprintf(buf, "file_cut_out_voice%d.pcm", file_count);
				FILE *fd = fopen(buf, "wb+");
				fwrite(voice_data->data, 1, voice_data->len, fd);
				fclose(fd);
				#endif
				voice_data->call_func(voice_data->data, voice_data->len);
				
				voice_data->status = ANALYZE_NO_DATA;
				voice_data->end_flag = 0;
				voice_data->len = 0;
				voice_data->start_flag = 0;
			}
		}
		else if(voice_data->status == ANALYZE_RUNING)
		{
			voice_data->status  = ANALYZE_END_RUNING;
			memcpy(&voice_data->data[voice_data->len], frame, frame_len);
			voice_data->len += frame_len;
			voice_data->end_flag++;
		}
		
	 }
	 
	 return ERR_OK;
	
}


int audio_frame_analyze2(uint8_t* frame, int frame_len)
{
	
	
}



