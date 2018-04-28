
#include "common.h"
#include "audio_hd.h"

//#define PER_AUDIO_DURATION		(32)		// AAC一帧128ms，分四次编码组成一帧
#define PER_AUDIO_DURATION		    20 		    // AMR一帧20ms

static Paudio_ctrl_handle paudio_ctrl = NULL;

void audio_encode_init(int oneBufsize);
T_S32 audio_encode_start( T_U32 oneBufsize, int encode_type);
T_VOID audio_encode_stop( int encode_type);


void *audio_malloc_pcm( int size)
{
    Paudio_pcm_queue_data pcm;
    
    pcm = (Paudio_pcm_queue_data)malloc(sizeof(audio_pcm_queue_data));
    if(pcm)
    {
        pcm->pcm_buffer = (uint8 *)malloc(size);
        if(pcm->pcm_buffer == NULL)
        {
            free(pcm);
            return NULL;
        }
        pcm->pcm_size = size;
    }
    return (void *)pcm;
}


void audio_free_pcm( void *pqueue_buffer)
{
    Paudio_pcm_queue_data pcm = (Paudio_pcm_queue_data)pqueue_buffer;

    if(pcm)
    {
        free(pcm->pcm_buffer);
        free(pcm);
    }
}

T_STM_STRUCT * audio_malloc_stream_ram(int size)
{
    T_STM_STRUCT *pStream;
    
    pStream = malloc(sizeof(T_STM_STRUCT));
    if(pStream)
    {
        pStream->buf = malloc(size);
        if(pStream->buf == NULL)
        {
            anyka_print("[%s]malloc %d failed\n", __func__, size);
            free(pStream);
            return NULL;
        }
    }

    return pStream;
}

void audio_free_stream_ram(void *pStream)
{
    T_STM_STRUCT *tmp = (T_STM_STRUCT *)pStream;

    if(pStream)
    {
        free(tmp->buf);
        free(tmp);
    }
}
/**
 * NAME         audio_insert_encode_data_send_queue
 * @BRIEF	 将编码后的音频数据放到相应的数据队列
 * @PARAM	head
                    pdata
                    size
                    time_stamp
 * @RETURN	void
 * @RETVAL	
 */


void audio_insert_encode_data_send_queue(Paudio_user_thread_handle head, uint8 *pdata, int size, int time_stamp)
{
    T_STM_STRUCT *encode_data_handle;

    while(head)
    {
        encode_data_handle = audio_malloc_stream_ram(size);
        if(encode_data_handle)
        {
            memcpy(encode_data_handle->buf, pdata, size);
            encode_data_handle->timestamp = time_stamp;
            encode_data_handle->size = size;
            if(0 == anyka_queue_push(head->queue_handle, (void *)encode_data_handle))
            {
                anyka_debug("[%s:%d] We will lost encode!\n", __func__, __LINE__);
                audio_free_stream_ram(encode_data_handle);
            }
        }
        sem_post(&head->audio_data_sem);
        head = head->next;       
    }
}


/**
 * NAME         audio_is_working
 * @BRIEF	 判断编码类型的工作标志位是否为真
 * @PARAM	void
 * @RETURN	int
 * @RETVAL	if working_flag is ture, return 1; else return 0
 */

int audio_is_working(void)
{
	int i;

	for(i = 0; i < SYS_AUDIO_ENCODE_MAX; i++)
	{
		if(paudio_ctrl->encode_queue_info[i].working_flag){
			return 1;
		}
	}
	
	return 0;
}

/**
 * NAME         audio_need_enable_agc
 * @BRIEF	根据工作标志位启动agc, 排除PCM类型数据
 * @PARAM	void
 * @RETURN	int
 * @RETVAL	if working_flag is ture, return 1; else return 0
 */

int audio_need_enable_agc(void)
{
	int i;

	for(i = 0; i < SYS_AUDIO_ENCODE_MAX; i++)
	{
		/**** skip pcm ****/
		if(i == SYS_AUDIO_RAW_PCM)
			continue;
		
		if(paudio_ctrl->encode_queue_info[i].working_flag){
			return 1;
		}
	}
	
	return 0;
}


/**
 * NAME         audio_read_ad_pcm
 * @BRIEF	 从AD中读出数据,并对数据进行编码 ，放到编码数据队列中
 * @PARAM	head
                    pdata
                    size
                    time_stamp
 * @RETURN	void
 * @RETVAL	
 */

static void * audio_read_ad_pcm( void * user )
{
	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());
	
	T_AUDIO_READ * handle = (T_AUDIO_READ*)(user);
	int i, ret = 0;
	unsigned char  *pRawBuffer = NULL;
	unsigned char  *pdata = NULL;
    //Paudio_pcm_queue_data pqueue_buf;
	unsigned long nDataLen, nTimeStamp = 0;
	unsigned long nRecTimeStamp[SYS_AUDIO_ENCODE_MAX] = {0};
	unsigned long nPcmLen;

//	struct timeval  tv2;

	if ( handle == NULL ) {
		return NULL;
	}

	//alloc the buffer to load the pcm data
	pRawBuffer = (T_U8*)malloc(handle->nOnceBufSize*2+AUDIO_INFORM_LENGTH);
	if ( NULL == pRawBuffer ) {
		return NULL;
	}
	pdata = pRawBuffer + AUDIO_INFORM_LENGTH; //point data save address
	bzero(pRawBuffer, handle->nOnceBufSize*2);
    anyka_print("[%s:%d] audio thread start processing\n", __func__, __LINE__);

	while(1)
	{
        anyka_print("[%s:%d] audio thread sleep\n", __func__, __LINE__);
	    audio_set_ad_agc_enable(0);
        audio_ad_filter_enable(paudio_ctrl->ad_handle, 0);        
		//sem_wait(&paudio_ctrl->audio_ad_sem);
        anyka_print("[%s:%d] audio thread wake\n", __func__, __LINE__);
        audio_ad_filter_enable(paudio_ctrl->ad_handle, 1);

		if(audio_need_enable_agc()){
			audio_set_ad_agc_enable(1);
		}
     	struct timeval tvs1;
		struct timeval tvs2;
		int		before;
		int		after;
		static int count = 1;
		char filename[100];
		sprintf(filename, "/mnt/pcm/test8k%d.pcm", count);
		
		long fd = open(filename,  O_CREAT | O_APPEND | O_TRUNC | O_WRONLY);
		if(fd < 0)
		{
			printf("open %s file failed\r\n",filename);
			return NULL;
		}
		
		while(1/*audio_is_working()*/)
        {
    		ret = audio_read_ad(paudio_ctrl->ad_handle, pdata, handle->nOnceBufSize, 
									(unsigned long *)&nTimeStamp); //get the pcm data

			
			gettimeofday(&tvs1, NULL);

			if(tvs1.tv_sec >= tvs2.tv_sec)
			{
				after = tvs1.tv_sec;
			}
			else
			{
				after = tvs1.tv_sec + 59;
			}

			
			if((after - tvs2.tv_sec) > 10)
			{
				close(fd);
				count++;
				tvs2.tv_sec = tvs1.tv_sec;
				sprintf(filename, "/mnt/pcm/test8k%d.pcm", count);
				printf("pcm new file is :%s\r\n",filename);
				fd = open(filename,  O_CREAT | O_APPEND | O_TRUNC | O_WRONLY);
				if(fd < 0)
				{
					printf("open %s file failed\r\n", filename);
					return NULL;
				}
				
			}

			
			write(fd, pdata,handle->nOnceBufSize);

			

			//printf("-------------pcm-size:%d----------\r\n",handle->nOnceBufSize);
    		if ( ret < 0 ) {
               // usleep(10000);
    			anyka_print("[%s:%d] recorder thread read audio from AD error! ret = %d\n",
    						__func__, __LINE__, ret);
    			continue;
    		}else if ( ret == 0 ) {
               // usleep(10000);
    			anyka_print("[%s:%d] AD running error! but already recover this error!\n",
    						__func__, __LINE__);
    			continue;
    		}
#if 1
    		//set the time
    		*((T_U32 *)pRawBuffer) = nTimeStamp;//bytenum * 1000 / rate;
    		nPcmLen = ret;
    		/*************************************************************/
            for(i = 0; i < SYS_AUDIO_ENCODE_MAX; i ++)
            {
                if(paudio_ctrl->encode_queue_info[i].working_flag == 0)
                {
                    continue;
                }
                pthread_mutex_lock(&paudio_ctrl->encode_queue_info[i].audio_ctrl_mutex);
    			if(paudio_ctrl->encode_queue_info[i].working_flag)
    			{
                    if(paudio_ctrl->encode_queue_info[i].encode_handle)
                    {
        				ret = audio_encode(paudio_ctrl->encode_queue_info[i].encode_handle, 
											pRawBuffer + AUDIO_INFORM_LENGTH, 
											nPcmLen, (T_U8*)paudio_ctrl->encode_buffer, 
											paudio_ctrl->encode_size * 2);
                    }
                    else
                    {
                        ret = nPcmLen;
                        memcpy(paudio_ctrl->encode_buffer, pRawBuffer + AUDIO_INFORM_LENGTH, nPcmLen);
                    }
    				
    					
    				if( ret > 0)
    				{
                        // we will add the code of record   
                        nDataLen = ret;
                        if(nRecTimeStamp[i])
                        {
                            audio_insert_encode_data_send_queue(
									paudio_ctrl->encode_queue_info[i].audio_user_handle, 
									paudio_ctrl->encode_buffer, nDataLen, nRecTimeStamp[i]);
                        }
                        else
                        {
                            audio_insert_encode_data_send_queue(
									paudio_ctrl->encode_queue_info[i].audio_user_handle, 
									paudio_ctrl->encode_buffer, nDataLen, nTimeStamp);
                        }
                        nRecTimeStamp[i] = 0;
    				}
                    else if(ret == 0)
                    {
                        nRecTimeStamp[i] = nTimeStamp;
                    }
    			}
                pthread_mutex_unlock(&paudio_ctrl->encode_queue_info[i].audio_ctrl_mutex);
            }
            /*************************************************************/
#endif
        }
	}

    anyka_print("[%s:%d] audio thread stop processing\n", __func__, __LINE__);

	

	free(pRawBuffer);
	return NULL;
}

/**
 * NAME         audio_modify_sample
 * @BRIEF	 修改采样率
 * @PARAM	input
 * @RETURN	void
 * @RETVAL	
 */

void audio_modify_sample(T_AUDIO_INPUT *input)
{
    if(paudio_ctrl)
    {
        audio_set_smaple(paudio_ctrl->ad_handle, input);
    }
}


/**
 * NAME         audio_modify_vol
 * @BRIEF	 修改gain
 * @PARAM	vol, 这个值 越小，音量越大
 * @RETURN	void
 * @RETVAL	
 */

void audio_modify_vol(int vol)
{
    if(paudio_ctrl)
    {
        audio_set_mic_vol(paudio_ctrl->ad_handle, vol);
    }
}

/**
 * NAME         audio_modify_filter
 * @BRIEF	 重采样的开关，
 * @PARAM	on_off
 * @RETURN	void
 * @RETVAL	
 */

void audio_modify_filter(int on_off)
{
    if(paudio_ctrl)
    {
        audio_ad_filter_enable(paudio_ctrl->ad_handle, on_off);
    }
}

/**
 * NAME         audio_start
 * @BRIEF	 开启音频
 * @PARAM	input
 * @RETURN	void
 * @RETVAL	
 */

int audio_start(T_AUDIO_INPUT *input)
{

	T_S32	ret = 0;
	T_U32 nBytesPerSample;
	T_U32 nActuallySR;
    
    if(paudio_ctrl)
    {
        return 0;
    }
    paudio_ctrl = (Paudio_ctrl_handle)malloc(sizeof(audio_ctrl_handle));
    if(NULL == paudio_ctrl)
    {
        anyka_print("[%s:%d] it fails to malloc\n", __func__, __LINE__);
        return -1;
    }

	paudio_ctrl->ad_handle = audio_ad_open(input);
    audio_ad_filter_enable(paudio_ctrl->ad_handle, 1);
	
	paudio_ctrl->gaudio.nSampleRate 	= input->nSampleRate;
	paudio_ctrl->gaudio.nBitsPerSample 	= input->nBitsPerSample;
	paudio_ctrl->gaudio.nChannels 		= input->nChannels;
	paudio_ctrl->gaudio.nGetPacketNum 	= 0;
	paudio_ctrl->gaudio.nPeriodDuration = PER_AUDIO_DURATION;
    nBytesPerSample = 2;
	
	//get the hardware samplerate
	switch (paudio_ctrl->gaudio.nSampleRate)
	{
		case 8000:
			nActuallySR = 7990;
		break;

		case 11025:
			nActuallySR = 10986;
		break;

		case 16000:
			nActuallySR = 15980;
			break;
		case 22050:
			nActuallySR = 21972;
			break;
		case 32000:
			nActuallySR = 31960;
			break;
		default:
			nActuallySR = paudio_ctrl->gaudio.nSampleRate;
	}
	//audio size then the once get
	paudio_ctrl->gaudio.nOnceBufSize = (paudio_ctrl->gaudio.nSampleRate * 
										paudio_ctrl->gaudio.nChannels * 
										paudio_ctrl->gaudio.nPeriodDuration *
										nBytesPerSample ) / 1000;

	if(paudio_ctrl->gaudio.nOnceBufSize & 1)
	{
		paudio_ctrl->gaudio.nOnceBufSize++;
	}
	
	audio_encode_init(paudio_ctrl->gaudio.nOnceBufSize+AUDIO_INFORM_LENGTH);

 	// paudio_ctrl->pcm_queue_handle = anyka_queue_init(AUDIO_QUEUE_MAX_ITEM);
  	// sem_init(&paudio_ctrl->pcm_data_sem, 0, 0);
    sem_init(&paudio_ctrl->audio_ad_sem, 0, 0);
	//set pcm data input device
	
	//create the capture data thread
	ret = anyka_pthread_create(&(paudio_ctrl->pcm_thread), audio_read_ad_pcm,
			(void *)&paudio_ctrl->gaudio, ANYKA_THREAD_MIN_STACK_SIZE, 85);
	if ((ret) != 0){
	//pthread_attr_destroy(&SchedAttr);
		anyka_print("[%s:%d] unable to create audio_read_ad_pcm thread , ret = %d!\n",
					__func__, __LINE__, (int)ret );
		return -1;	
	}
	//pthread_attr_destroy(&SchedAttr);

	return 0;
}




/**
 * NAME         audio_send_encode_data
 * @BRIEF	 将音频队列中的数据，通过回调送给用户
 * @PARAM	input
 * @RETURN	void
 * @RETVAL	
 */

static T_pVOID audio_send_encode_data( T_pVOID user )
{
	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());
	
    Paudio_user_thread_handle  cur = (Paudio_user_thread_handle)user;
    T_STM_STRUCT * pstream_data;

    while(cur->run_flag)
    {
        sem_wait(&cur->audio_data_sem);
        while(cur->run_flag && anyka_queue_not_empty(cur->queue_handle))
        {
            pstream_data = (T_STM_STRUCT *)anyka_queue_pop(cur->queue_handle);
            cur->callback(cur->param, pstream_data);
            audio_free_stream_ram(pstream_data);
        }
    }
	anyka_print("[%s:%d]: the audio send thread will exit(id:%ld)!\n", __func__, __LINE__, (long int)gettid());
	return NULL;
}


/**
 * NAME         audio_add
 * @BRIEF	应用启动音频一个实例，目前支持以下几种编码方式，SYS_AUDIO_ENCODE_AAC,SYS_AUDIO_ENCODE_G711,SYS_AUDIO_RAW_PCM,
 * @PARAM	encode_type   对应需要输出的编码格式
                    pcallback 采集到数据的回调函数
                    mydata    回调回传给应用的数据指针
 * @RETURN	-1-->fail; 0-->ok
 * @RETVAL	
 */

int audio_add(int  encode_type, AUDIO_SEND_CALLBACK *pcallback, void *mydata)
{
	T_S32	ret = 0;
    Paudio_user_thread_handle head, new_user;

    if(encode_type >= SYS_AUDIO_ENCODE_MAX)
    {        
        anyka_print("[%s:%d] it fails to add beacuse of encode_type:%d!\n", __func__, __LINE__, encode_type);
        return -1;
    }
    anyka_print("[%s:%d] system add an audio thread:(type:%d)!\n", __func__, __LINE__, encode_type);
	
    new_user = (Paudio_user_thread_handle)malloc(sizeof(audio_user_thread_handle));
    sem_init(&new_user->audio_data_sem, 0, 0);
    new_user->queue_handle = anyka_queue_init(AUDIO_QUEUE_MAX_ITEM);
    new_user->param = mydata;
    new_user->callback = pcallback;
    new_user->run_flag = 1;
    new_user->next = NULL;
    if ((ret = anyka_pthread_create(&(new_user->send_handle), audio_send_encode_data, (void *)new_user , ANYKA_THREAD_MIN_STACK_SIZE, -1)) != 0){
    	
        sem_destroy(&new_user->audio_data_sem);        
        anyka_queue_destroy(new_user->queue_handle, audio_free_stream_ram);
        free(new_user);
    	anyka_print("[%s:%d] unable to create thread audio_send_encode_data, ret = %ld!\n", __func__, __LINE__, ret );
    	return -1;	
    }
    
    pthread_mutex_lock(&paudio_ctrl->encode_queue_info[encode_type].audio_ctrl_mutex);
    head = paudio_ctrl->encode_queue_info[encode_type].audio_user_handle;
    if(NULL == head)
    {
        audio_encode_start(paudio_ctrl->gaudio.nOnceBufSize+AUDIO_INFORM_LENGTH, encode_type);
        paudio_ctrl->encode_queue_info[encode_type].audio_user_handle = new_user;
        audio_ad_clr_buffer(paudio_ctrl->ad_handle);
    }
    else
    {
        while(head->next)
        {
            head = head->next;
        }
        head->next = new_user;
    }
    paudio_ctrl->encode_queue_info[encode_type].working_flag = 1;
    pthread_mutex_unlock(&paudio_ctrl->encode_queue_info[encode_type].audio_ctrl_mutex);
    sem_post(&paudio_ctrl->audio_ad_sem);
    return 0;

}

/**
 * NAME         audio_del
 * @BRIEF	应用停止音频的使用，将释放为其申请的资源
 * @PARAM	encode_type   对应需要输出的编码格式
                    mydata    回调回传给应用的数据指针
 * @RETURN	NONE
 * @RETVAL	
 */

void audio_del(int  encode_type, void *mydata)
{
    Paudio_user_thread_handle head, pre;

    if(NULL == paudio_ctrl)
    {
        return ;
    }
    
    if(encode_type >= SYS_AUDIO_ENCODE_MAX)
    {        
        return ;
    }
	
	anyka_print("[%s:%d] now stop audio type: %d\n", __func__, __LINE__, encode_type);

	pthread_mutex_lock(&paudio_ctrl->encode_queue_info[encode_type].audio_ctrl_mutex);    
    head = paudio_ctrl->encode_queue_info[encode_type].audio_user_handle;
    pre = head;
    while(head && (head->param != mydata))
    {
        pre = head;
        head = head->next;
    }
    if(head)
    {
        if(pre == head)
        {
            paudio_ctrl->encode_queue_info[encode_type].audio_user_handle = head->next;
        }
        else
        {
            pre->next = head->next;
        }
        if(paudio_ctrl->encode_queue_info[encode_type].audio_user_handle == NULL)
        {
            paudio_ctrl->encode_queue_info[encode_type].working_flag = 0;
            audio_encode_stop(encode_type);
        }
        pthread_mutex_unlock(&paudio_ctrl->encode_queue_info[encode_type].audio_ctrl_mutex);
		
		//anyka_print("[%s:%d] waiting until thread is over!\n", __func__, __LINE__);
        head->run_flag = 0;
        sem_post(&head->audio_data_sem);
		pthread_join(head->send_handle, NULL);		//wait for the thread exit
		anyka_print("[%s:%d] The thread ID: send_handle had exited\n", __func__, __LINE__);
		
        sem_destroy(&head->audio_data_sem);       
        anyka_queue_destroy(head->queue_handle, audio_free_stream_ram);
		
        free(head);
    }
    else
    {
        pthread_mutex_unlock(&paudio_ctrl->encode_queue_info[encode_type].audio_ctrl_mutex);
    }
}

/**
 * NAME     audio_encode_init     
 * @BRIEF	oneBufsize
 * @RETURN	void 
 * @RETVAL	
 */

void audio_encode_init(int oneBufsize)
{
    paudio_ctrl->encode_queue_info[SYS_AUDIO_ENCODE_G711].audio_user_handle = NULL;
    paudio_ctrl->encode_queue_info[SYS_AUDIO_ENCODE_G711].working_flag =0;
    pthread_mutex_init(&paudio_ctrl->encode_queue_info[SYS_AUDIO_ENCODE_G711].audio_ctrl_mutex, NULL); 

    
    paudio_ctrl->encode_queue_info[SYS_AUDIO_ENCODE_AMR].audio_user_handle = NULL;
    paudio_ctrl->encode_queue_info[SYS_AUDIO_ENCODE_AMR].working_flag =0;
    pthread_mutex_init(&paudio_ctrl->encode_queue_info[SYS_AUDIO_ENCODE_AMR].audio_ctrl_mutex, NULL); 
    
    paudio_ctrl->encode_queue_info[SYS_AUDIO_ENCODE_AAC].audio_user_handle = NULL;
    paudio_ctrl->encode_queue_info[SYS_AUDIO_ENCODE_AAC].working_flag =0;
    pthread_mutex_init(&paudio_ctrl->encode_queue_info[SYS_AUDIO_ENCODE_AAC].audio_ctrl_mutex, NULL); 

    paudio_ctrl->encode_size = (oneBufsize - AUDIO_INFORM_LENGTH);
    paudio_ctrl->encode_buffer = (T_U8*)malloc(paudio_ctrl->encode_size * 2);
    if (AK_NULL == paudio_ctrl->encode_buffer)
    {
        anyka_print("[%s:%d] can't malloc ecnode buffer!\n", __func__, __LINE__);
        return ;
    }
    pthread_mutex_init(&paudio_ctrl->encode_queue_info[SYS_AUDIO_RAW_PCM].audio_ctrl_mutex, NULL); 
    paudio_ctrl->encode_queue_info[SYS_AUDIO_RAW_PCM].audio_user_handle = NULL;
    paudio_ctrl->encode_queue_info[SYS_AUDIO_RAW_PCM].encode_handle = NULL;
    paudio_ctrl->encode_queue_info[SYS_AUDIO_RAW_PCM].working_flag =0;    
            
}

/**
 * NAME     audio_encode_stop     
 * @BRIEF	encode_type
 * @RETURN	void 
 * @RETVAL	
 */

T_VOID audio_encode_stop(int encode_type)
{
    audio_enc_close(paudio_ctrl->encode_queue_info[encode_type].encode_handle);
}

/**
 * NAME         audio_encode_start
 * @BRIEF	初始化音频资源
 * @PARAM	pstEncIn   
                    pstEncOut   
                    oneBufsize
 * @RETURN	-1-->fail; 0-->ok
 * @RETVAL	
 */

T_S32 audio_encode_start( T_U32 oneBufsize, int encode_type)
{
	AudioEncIn	stEncIn;
	AudioEncOut	stEncOut;
    
	stEncIn.nChannels = 1;
	stEncIn.nBitsPerSample = 16;
	stEncIn.nSampleRate = 8000;


    switch(encode_type)
    {
        case SYS_AUDIO_ENCODE_G711:
        	stEncOut.enc_type = ENC_TYPE_G711;
        	paudio_ctrl->encode_queue_info[SYS_AUDIO_ENCODE_G711].encode_handle = audio_enc_open( &stEncIn, &stEncOut );
        	
        	if( paudio_ctrl->encode_queue_info[SYS_AUDIO_ENCODE_G711].encode_handle == NULL )
        	{
        		anyka_print("[%s:%d] can't open G711 codec\n", __func__, __LINE__);
        	}
            break;
        
        case SYS_AUDIO_ENCODE_AMR:    
        	stEncOut.enc_type = ENC_TYPE_AMR;
        	paudio_ctrl->encode_queue_info[SYS_AUDIO_ENCODE_AMR].encode_handle = audio_enc_open( &stEncIn, &stEncOut );
        	
        	if( paudio_ctrl->encode_queue_info[SYS_AUDIO_ENCODE_AMR].encode_handle == NULL )
        	{
        		anyka_print("[%s:%d] can't open AMR codec\n", __func__, __LINE__);
        	}
            break;
            
        case SYS_AUDIO_ENCODE_AAC:    
        	stEncOut.enc_type = ENC_TYPE_AAC;	
        	paudio_ctrl->encode_queue_info[SYS_AUDIO_ENCODE_AAC].encode_handle = audio_enc_open( &stEncIn, &stEncOut );
        	
        	if ( paudio_ctrl->encode_queue_info[SYS_AUDIO_ENCODE_AAC].encode_handle == NULL ) 
            {
        		anyka_print("[%s:%d] can't open the AAC SdCodec!\n", __func__, __LINE__);
        	}
            break;

        case SYS_AUDIO_RAW_PCM:               
            break;
    }

	return 0;
}

