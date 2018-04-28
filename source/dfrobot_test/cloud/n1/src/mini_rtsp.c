/*************************************************************************
	> File Name: minirtsp_main.c
	> Author: kaga
	> Mail: kejiazhw@163.com 
	> Created Time: 2014å¹?1æœ?6æ—?æ˜ŸæœŸå›?10æ—?4åˆ?1ç§? ************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<stdint.h>

#include <sys/time.h>

#include "minirtsp.h"
#include "ja_type.h"
#include "common.h"
#include "ak_n1_interface.h"



#define MINIRTSP_AUDIO


typedef struct 
{
	uint32_t	conn_id;
    uint32_t	audio_ts_ms;
	uint32_t	video_ts_ms;
}MINIRTSP_USR_STRUCT;


static N1_CTRL *pMiniRtsp = NULL;


static int _stream_init(lpMINIRTSP_STREAM s,const char *name) 
{
	MINIRTSP_USR_STRUCT *ctx = NULL;
	int i = 0;

	if (CONNECT_NUM_MAX == pMiniRtsp->conn_cnt)
	{
		anyka_print("************************conn_cnt is %d! ******************\n", pMiniRtsp->conn_cnt);
		return rMINIRTSP_FAIL;
	}
	
	ctx = (MINIRTSP_USR_STRUCT*)calloc(1, sizeof(MINIRTSP_USR_STRUCT));
	memset(ctx, 0, sizeof(MINIRTSP_USR_STRUCT));

	memset(s, 0, sizeof(stMINIRTSP_STREAM));
	
	strcpy(s->name, name);

	s->timestamp = 0;
	s->data = NULL;
	s->size = 0;
	
	pthread_mutex_lock(&pMiniRtsp->lock);

	for (i=0; i<CONNECT_NUM_MAX; i++)
	{
	    pthread_mutex_lock(&pMiniRtsp->conn[i].lock);
	    
		if (AK_FALSE == pMiniRtsp->conn[i].bValid)
		{
			ctx->conn_id = i;
			pMiniRtsp->conn[i].bValid = AK_TRUE;
			pMiniRtsp->conn_cnt++;
			pthread_mutex_unlock(&pMiniRtsp->conn[i].lock);
			break;
		}

		pthread_mutex_unlock(&pMiniRtsp->conn[i].lock);
	}
	
	pthread_mutex_unlock(&pMiniRtsp->lock);

	anyka_print("************************init stream %s *******conn : %d***********\n", name, ctx->conn_id);

	s->param = ctx;

	ak_n1_get_video_setting(&g_video_set);

	pthread_mutex_lock(&pMiniRtsp->lock);

	pthread_mutex_lock(&pMiniRtsp->conn[i].lock);

	if ('0' == name[4])
	{
		pMiniRtsp->conn[i].bMainStream = AK_TRUE;
		pMiniRtsp->conn[i].video_queue = anyka_queue_init(100);
		pMiniRtsp->conn[i].video_run_flag = 1;		
	}
	else if ('1' == name[4])
	{
		pMiniRtsp->conn[i].bMainStream = AK_FALSE;
		pMiniRtsp->conn[i].video_queue = anyka_queue_init(110);
        pMiniRtsp->conn[i].video_run_flag = 1;
	}

	pMiniRtsp->conn[i].biFrameflag = AK_FALSE;

#ifdef MINIRTSP_AUDIO

	pMiniRtsp->conn[i].audio_queue = anyka_queue_init(200);
	pMiniRtsp->conn[i].audio_run_flag = 1;

	anyka_print("conn_cnt = %d\n", pMiniRtsp->conn_cnt);
#endif
	

	pthread_mutex_unlock(&pMiniRtsp->conn[i].lock);

	pthread_mutex_unlock(&pMiniRtsp->lock);

		
	anyka_print("rtsp stream(%s) init success.\n", name);
	return rMINIRTSP_OK;
}

static int _stream_lock(lpMINIRTSP_STREAM s) 
{
	MINIRTSP_USR_STRUCT *ctx = s->param;
	int i = 0;

	if (ctx) 
	{
		i = ctx->conn_id;
	}
	else
	{
        anyka_print("_stream_lock invalid\n");    
		return rMINIRTSP_FAIL;
	}
	
	pthread_mutex_lock(&pMiniRtsp->conn[i].streamlock);
	return rMINIRTSP_OK;
}

static int _stream_unlock(lpMINIRTSP_STREAM s) 
{
	MINIRTSP_USR_STRUCT *ctx = s->param;
	int i = 0;

	if (ctx) 
	{
		i = ctx->conn_id;
	}
	else
	{
        anyka_print("_stream_unlock invalid\n");    
		return rMINIRTSP_FAIL;
	}
	
	pthread_mutex_unlock(&pMiniRtsp->conn[i].streamlock);
	return rMINIRTSP_OK;
}

static int _stream_next(lpMINIRTSP_STREAM s) 
{	
	T_STM_STRUCT *pstream;
	MINIRTSP_USR_STRUCT *ctx = s->param;
	int i = 0;

	s->size = 0;

	if (ctx) 
	{
		i = ctx->conn_id;
	}
	else
	{
        anyka_print("_stream_next invalid\n");    
		return rMINIRTSP_FAIL;
	}


#ifdef MINIRTSP_AUDIO
	if (ctx->video_ts_ms < ctx->audio_ts_ms)
	{
#endif
		if(pMiniRtsp->conn[i].video_run_flag)
	    {
	        if((pstream = (T_STM_STRUCT *)anyka_queue_pop(pMiniRtsp->conn[i].video_queue)) == NULL)
	        {
	            //anyka_print("pop fail\n");
	            return rMINIRTSP_DATA_NOT_AVAILABLE;
	        }
	    }
		else
		{
			anyka_print("run_flag 0\n");
			return rMINIRTSP_FAIL;
		}

		if (!pMiniRtsp->conn[i].biFrameflag)
		{
			if (0 == pstream->iFrame)
			{
				anyka_free_stream_ram(pstream);
				return rMINIRTSP_DATA_NOT_AVAILABLE;
			}
			else
			{
				pMiniRtsp->conn[i].biFrameflag = AK_TRUE;
			}
		}

		s->type = MINIRTSP_MD_TYPE_H264;
		ctx->video_ts_ms = pstream->timestamp;
#ifdef MINIRTSP_AUDIO
	}
	else
	{
		if(pMiniRtsp->conn[i].audio_run_flag)
        {
            if((pstream = (T_STM_STRUCT *)anyka_queue_pop(pMiniRtsp->conn[i].audio_queue)) == NULL)
            {
                return rMINIRTSP_DATA_NOT_AVAILABLE;
            }
        }
		else
		{
			anyka_print("run_flag 0\n");
			return rMINIRTSP_FAIL;
		}

		s->type = MINIRTSP_MD_TYPE_ALAW;
		ctx->audio_ts_ms = pstream->timestamp;
	}
#endif
	
	s->size = pstream->size;


	if (NULL != s->data)
	{
		free(s->data);
		s->data = NULL;
	}

	s->data = (T_pDATA)malloc(pstream->size);
	
	if (NULL == s->data)
	{
		anyka_print("s->data malloc failed!\n");
		anyka_free_stream_ram(pstream);
		return rMINIRTSP_FAIL;
	}

    memcpy(s->data, pstream->buf, pstream->size);

	
	
	s->isKeyFrame = pstream->iFrame;
	s->timestamp = pstream->timestamp * 1000;

	anyka_free_stream_ram(pstream);

	
	//anyka_print("get stream size:%d ts:%llu\n",pstream->size,s->timestamp);
	return rMINIRTSP_OK;
}

static int _stream_destroy(lpMINIRTSP_STREAM s) 
{
	MINIRTSP_USR_STRUCT *ctx = s->param;
	int i = 0;

	if(s->data) free(s->data);

	if (0 == pMiniRtsp->conn_cnt)
	{
		return rMINIRTSP_FAIL;
	}

	
	if (ctx) 
	{
		i = ctx->conn_id;
	}
	else
	{
        anyka_print("_stream_destroy invalid\n");    
		return rMINIRTSP_FAIL;
	}

	pthread_mutex_lock(&pMiniRtsp->lock);
	pthread_mutex_lock(&pMiniRtsp->conn[i].lock);


#ifdef MINIRTSP_AUDIO
    if (NULL != pMiniRtsp->conn[i].audio_queue)
    {
		anyka_print("[%s:%d]#####  Mini Rtsp stop audio %d #####\n", __func__, __LINE__, i);
	    pMiniRtsp->conn[i].audio_run_flag = 0;

	    anyka_queue_destroy(pMiniRtsp->conn[i].audio_queue, anyka_free_stream_ram);
	    pMiniRtsp->conn[i].audio_queue = NULL;
	}

		
#endif

    if (NULL != pMiniRtsp->conn[i].video_queue)
    {
        pMiniRtsp->conn[i].video_run_flag = 0;
        
        if ('0' == s->name[4])
    	{
    	    anyka_print("[%s:%d]#####  Mini Rtsp stop 720p %d #####\n", __func__, __LINE__, i);
    	}
    	else if ('1' == s->name[4])
    	{
    		anyka_print("[%s:%d]#####  Mini Rtsp stop  vga %d #####\n", __func__, __LINE__, i);
    	}

    	anyka_queue_destroy(pMiniRtsp->conn[i].video_queue, anyka_free_stream_ram);
        pMiniRtsp->conn[i].video_queue = NULL;

    	pMiniRtsp->conn_cnt--;
		pMiniRtsp->conn[i].bValid = AK_FALSE;
    }

	free(ctx);
	ctx = NULL;
	
    pthread_mutex_unlock(&pMiniRtsp->conn[i].lock);
	pthread_mutex_unlock(&pMiniRtsp->lock);

	anyka_print("_stream_destroy  conn:%d\n", i);
	
	return rMINIRTSP_OK;
}

static int _stream_reset(lpMINIRTSP_STREAM s) 
{
	return rMINIRTSP_OK;
}

lpMINIRTSP rtsp_server_run()
{
	stMINIRTSP_STREAM_FUNC funcs;

	pMiniRtsp = pn1_ctrl;
    if(NULL == pMiniRtsp)
    {
        return NULL;
    }

	lpMINIRTSP thiz = MINIRTSP_server_new();

	funcs.open = (fMINIRTSP_STREAM_OPEN)_stream_init;
	funcs.lock = (fMINIRTSP_STREAM_LOCK)_stream_lock;
	funcs.next = (fMINIRTSP_STREAM_NEXT)_stream_next;
	funcs.unlock = (fMINIRTSP_STREAM_UNLOCK)_stream_unlock;
	funcs.close = (fMINIRTSP_STREAM_CLOSE)_stream_destroy;
	funcs.get_avc = NULL;
	funcs.reset = (fMINIRTSP_STREAM_RESET)_stream_reset;
	MINIRTSP_server_set_stream_func(funcs);

	MINIRTSP_server_start(554);

	anyka_print("minirtsp version: %s\n", MINIRTSP_server_version());
	
    system_user_info * set_info = anyka_get_system_user_info();
    if(set_info->debuglog)
	    MINIRTSP_set_loglevel(thiz, 5);

	return thiz;
}

/*********************************************************************
*********************************************************************/


