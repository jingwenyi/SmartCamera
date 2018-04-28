
#include "common.h"
#include "sdcodec.h"
#include "audio_decode.h"


/************ define **************/
#define DECODE_SIZE 4096*4
#define CHANNAL 2
#define DA_TYPE ENC_TYPE_PCM
#define MAX_ITEM 30		//队列最大值
#define PATH_LEN 20

#define FILE_LIST 	"/tmp/alarm_audio_list"	
/*******************************************/

struct voice_tips
{
	int da_flag;//设置DA参数标志位，设置完一次，清零
	int run_flag;	//运行标志
	int destroy_flag;	//停止标志

	void *da_handle;	// da 句柄
	void *decode_handle; //解码句柄
	void *tips_queue;	//音频文件队列
	
	pthread_t msc_pthread;	//播放线程
	sem_t start;	//控制信号量
};

struct voice_tips voice_tip = {0};


/********************** MUSIC PLAYER *****************************/
/*
*	@param:	PATH 文件路径
*	voice_tips 打头的函数表示:anyka_voice_tips
*/
static int voice_tips_openfile(void *file_name)
{
	int voice_tips_fd;

	voice_tips_fd = open(file_name, O_RDONLY);
	if(voice_tips_fd < 0)
	{
		anyka_print("open music file failed, %s\n", strerror(errno));
		return -1;
	}
	
	return voice_tips_fd;
}


/**
* @brief 	voice_tips_decode_init
			初始化提示音解码模块功能
* @date 	2015/3
* @param:	int enc_type，要解码的提示音类型
* @return 	int
* @retval 	success --> 0; failed --> -1
*/

static int voice_tips_decode_init(int enc_type)
{	
	media_info pmedia;		//decode
	pmedia.m_AudioType = enc_type;
	//pmedia.m_AudioType = _SD_MEDIA_TYPE_MP3;
	pmedia.m_nChannels = 2;
	pmedia.m_nSamplesPerSec = 0;
	pmedia.m_wBitsPerSample = 0;
	
	voice_tip.decode_handle = audio_decode_open(&pmedia);
	if(voice_tip.decode_handle == NULL)
	{
		anyka_print("init decode failed\n");
		return -1;
	}
	
	audio_decode_change_mode(voice_tip.decode_handle);	

	return 0;
}


/**
* @brief 	voice_tips_da_init
			提示音模块 DA 初始化
* @date 	2015/3
* @param:	void
* @return 	int
* @retval 	success --> 0; failed --> -1
*/

static int voice_tips_da_init()
{
	T_AUDIO_INPUT param;		//音频参数
	
	/* 获取采样率等音频参数 */
	audio_decode_get_info(voice_tip.decode_handle, &param);				
	
	/*anyka_print("[BitsPerSample:%ld] [Channels:%ld] [SampleRate:%ld]\n", param.nBitsPerSample,
		param.nChannels, param.nSampleRate);*/
	
	//open DA
	voice_tip.da_handle = audio_da_open(&param);
	if(voice_tip.da_handle == NULL)
	{
		anyka_print("open DA failed\n");
		return -1;
	}
	
	audio_set_da_vol(voice_tip.da_handle, 2); //小音量播放提示音
	audio_da_clr_buffer(voice_tip.da_handle);

	voice_tip.da_flag = 0;
	
	return 0;
}


/**
* @brief 	voice_tips_play
			播放提示音，实际将数据写入DA
* @date 	2015/3
* @param:	void *handle , open DA 时返回的句柄
			void *parama, 解码后的数据
			int size, 数据长度
* @return 	void
* @retval 	
*/

static void voice_tips_play(void *handle , void *parama, int size)
{
	/* write da */	
	audio_write_da(handle, parama, size);
}


/**
* @brief 	voice_tips_decode_data
			提示音模块 数据解码
* @date 	2015/3
* @param:	void
* @return 	int
* @retval 	success --> 0; failed --> -1
*/

static int voice_tips_decode_data()
{
	int decode_size = 0, station = 0; 

	char *temp_buf = malloc(DECODE_SIZE * 2);
	if(temp_buf == NULL)
	{
		anyka_print("fail to malloc\n");
		return -1;
	}

	char *decode_outbuf = malloc(DECODE_SIZE * 2);
	if(decode_outbuf == NULL)
	{
		anyka_print("fail to malloc\n");		
		return -1;
	}

	while((decode_size = audio_decode_decode(voice_tip.decode_handle,(uint8*)decode_outbuf, CHANNAL)) > 0)
	{	
		if(voice_tip.da_flag)//初始化1次
		{		
			if(voice_tips_da_init() < 0)
			{
				anyka_print("[%s:%d]Initialize da failed.\n", __func__, __LINE__);
				break;
			}
		}
		
		//store data to temp buffer
		memcpy(temp_buf + station, decode_outbuf, decode_size); 
		station += decode_size;
		
		if(station < DECODE_SIZE)
		{
			continue;
		}
		
		//送数据进DA
		voice_tips_play(voice_tip.da_handle, temp_buf, station);
		break;
	}

	if((decode_size <= 0) && station)
	{
		voice_tips_play(voice_tip.da_handle, temp_buf, station);
	}


	free(temp_buf);
	free(decode_outbuf);
	
	return station;
}



/**
* @brief 	voice_tips_read_data
			提示音模块 读数据，添加到解码缓冲区，解码
* @date 	2015/3
* @param:	void
* @return 	void
* @retval 	
*/

static void voice_tips_read_data(int fd)
{
	int free_size, ret, end_flag = 1, encode_size;

	char *decode_buf = malloc(DECODE_SIZE);
	if(decode_buf == NULL)
	{
		anyka_print("[%s:%d]Fail to malloc\n", __func__, __LINE__);
		return;
	}

	voice_tip.da_flag = 1;
	
	while(voice_tip.run_flag)
	{	
		/****  检测free_size大小****/
		free_size = audio_decode_get_free(voice_tip.decode_handle);
		
		/**** free_size 大于DECODE_SIZE，则取DECODE_SIZE ****/
		if((free_size > DECODE_SIZE) && end_flag)
		{	
			free_size = DECODE_SIZE;
			ret = read(fd, decode_buf, free_size);
			if(ret > 0)
			{
				audio_decode_add_data(voice_tip.decode_handle, (uint8 *)decode_buf, ret);
			}
			else
			{
				//anyka_print("[%s:%d]Read to the end of audio file.\n", __func__, __LINE__);
				end_flag = 0;
			}
		}
		encode_size = voice_tips_decode_data();

		if((encode_size == 0) && (end_flag == 0))
		{
			//anyka_print("[%s:%d]Play audio finished.\n", __func__, __LINE__);
			break;
		}	
	}
	
	free(decode_buf);
	
	return;
}



/**
* @brief 	voice_tips_start_play
			提示音模块 开始播放线程函数
			等待添加数据后抛出的信号量，收到后从队列中读一个数据进行播放
* @date 	2015/3
* @param:	void
* @return 	int
* @retval 	success --> 0
*/
static int voice_tips_start_play()
{
	char *file_name;
	int fd;

	while(voice_tip.destroy_flag)
	{	
		sem_wait(&voice_tip.start);

		/****** 从播放列表pop一个数据出来 ******/
		file_name = (char *)anyka_queue_pop(voice_tip.tips_queue);
		if(file_name == NULL)
		{
			continue;
		}

		/********  open music file  ********/
		if((fd = voice_tips_openfile((void *)file_name)) < 0)
		{
			anyka_print("Can't open file: %s\n", (char *)file_name);	
			free(file_name);
			continue;
		}

		/*** decode init ***/
		voice_tips_decode_init(_SD_MEDIA_TYPE_MP3);
		
		/*** pa enable ***/
        audio_da_spk_enable(1);
		anyka_print("play voice tips: %s\n", (char *)file_name);
		
		/*read and decode, then write DA */
		voice_tips_read_data(fd);

		close(fd);
		free(file_name);
		sleep(3);
		/*** 延时，等待数据播完。关闭DA ， 关闭pa ***/
		audio_decode_close(voice_tip.decode_handle);
       	audio_da_spk_enable(0);
	}
	
	anyka_print("[%s:%d] going to exit\n", __func__, __LINE__);
	return 0;
}


/**
* @brief 	voice_tips_queue_free
			释放提示音队列资源
* @date 	2015/3
* @param:	void *data，指向要释放资源的指针 
* @return 	void
* @retval 	
*/

static void voice_tips_queue_free(void *data)
{
	void *tmp = data;

    if(tmp)
    {
        free(tmp);
    }
}

/***********以上为模块内部函数**************/

/******************************************************/
/*
*	以下为对外开放接口
*/  



/**
* @brief 	voice_tips_init
			提示音模块初始化函数
* @date 	2015/3
* @param:	void 
* @return 	int
* @retval 	success --> 0; failed --> -1
*/

/***************  queue init func  ********************/
/**enc_type:初始化时将要解码的文件的类型传入**/
int voice_tips_init()
{
	if(voice_tip.tips_queue != NULL){
		anyka_print("[%s] It has been initialized.\n", __func__);
		return -1;
	}
	
	memset(&voice_tip, 0, sizeof(voice_tip));
	voice_tip.tips_queue = anyka_queue_init(MAX_ITEM);
	if(voice_tip.tips_queue == NULL)
	{
		anyka_print("init queue failed\n");
		return -1;
	}

	sem_init(&voice_tip.start, 0, 0);
	voice_tip.destroy_flag = 1;
	voice_tip.run_flag = 1;

	/*****************  创建本模块独立线程 *****************/
	anyka_pthread_create(&voice_tip.msc_pthread, (void*)voice_tips_start_play,
							NULL, ANYKA_THREAD_MIN_STACK_SIZE, 60);
	
	return 0;
	
}

/**
* @brief 	voice_tips_add_music
			外部通过调用该接口添加要播放的文件
* @date 	2015/3
* @param:	void *file_name，音频文件文件名，绝对路径
* @return 	int
* @retval 	success --> 0; failed --> -1
*/

/************   添加文件到队列 ************/
int voice_tips_add_music(void *file_name)
{	
	if(voice_tip.tips_queue == NULL){
		anyka_print("[%s:%d] uninitial\n", __func__, __LINE__);
		return -1;
	}
	
	void *name = malloc(strlen(file_name) + 1);
	if(name == NULL)
	{
		anyka_print("[%s:%d] Fail to malloc\n", __func__, __LINE__);
		return -1;
	}
	strcpy(name, file_name);	//get file name

	
	/*******  将数据路劲压入队列****/
	if(!anyka_queue_push(voice_tip.tips_queue, name))
	{
		anyka_print("Fail to add music\n ");
		return -1;
	}
	sem_post(&voice_tip.start);

	return 0;
}


/**
* @brief 	voice_tips_stop
			提示音模块停止接口
* @date 	2015/3
* @param:	void 
* @return 	void
* @retval 	
*/

/***   被高优先级打断时调用***/
void voice_tips_stop()
{
	void *file_name;
	do
	{
		file_name = anyka_queue_pop(voice_tip.tips_queue);
		if(file_name)
			free(file_name);
	}while(file_name != NULL);
	
	//lock
	// pop all item from queue
	// unlock
	voice_tip.run_flag = 0;
}


/**
* @brief 	voice_tips_destroy
			提示音模块真正停止接口，通过调用它释放资源
* @date 	2015/3
* @param:	void 
* @return 	void
* @retval 	
*/

/***********  释放资源接口  ************/
void voice_tips_destroy()
{	
	voice_tips_stop();
	voice_tip.destroy_flag = 0;
	sem_post(&voice_tip.start);
	
	pthread_join(voice_tip.msc_pthread, NULL);
	
	audio_da_close(voice_tip.da_handle);
	audio_decode_close(voice_tip.decode_handle);
	anyka_queue_destroy(voice_tip.tips_queue, voice_tips_queue_free);
	sem_destroy(&voice_tip.start);
	
}

/**
* @brief 	voice_tips_get_file
			提示音模块获取要播放的文件名，主要是和脚本配合使用
* @date 	2015/3
* @param:	void 
* @return 	void
* @retval 	
*/

void voice_tips_get_file()
{
	FILE *fp = NULL;
	char audio_name[100] = {0}; 
	/** check file exist **/
	if(access(FILE_LIST, F_OK) != 0)
	{
		anyka_print("[%s:%d] the alarm audio list file isn't exist.\n", __func__, __LINE__);
		return;		
	}
	/*** open file ***/
	fp = fopen(FILE_LIST, "r");
	if(!fp)
	{
		anyka_print("[%s:%d]Open %s failed.\n", __func__, __LINE__, FILE_LIST);
		return;		
	}
	/** read data tu buf  **/
	fread(audio_name, sizeof(char), 99, fp);
	fclose(fp);
	
	audio_name[strlen(audio_name) - 1] = 0;	// delete the end-of-file lable '\n'
	voice_tips_add_music(audio_name);	//add to queue to play

	remove(FILE_LIST);	//remove the file

	return;
}

/****************** File End ********************/
