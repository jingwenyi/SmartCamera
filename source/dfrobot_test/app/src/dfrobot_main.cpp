/*********************************************************
**
** author :	jingwenyi 
** date : 	2016.05.19
** company: 	dfrobot
** e-mail: 	1124427436@qq.com
**
************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <execinfo.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <termios.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/ioctl.h>


#include "video_encode.h"
#include "PTZControl.h"
#include "camera.h"
#include "akuio.h"
#include "basetype.h"
#include "anyka_types.h"
#include <json/json.h>
#include <json/json_api.h>
#include <socket_transfer_server.h>
#include "Condition.h"
#include "audio_ctrl.h"
#include "anyka_com.h"
#include "anyka_fs.h"
#include "voice_tips.h"
#include "audio_decode.h"
#include "sdcodec.h"
#include "pcm_lib.h"
#include "audio_hd.h"
#include "adts.h"
#include "audio_cycle_buffer.h"
#include "audio_list.h"
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "ADTSAudioLiveSource.hh"
#include "ADTSAudioLiveServerMediaSubsession.hh"
#include "curl.h"
#include "dfrobot_tuling.h"
#include "audio_analyses_algorithm.h"

#include "check.h"
#include "interface.h"
#include "config.h"
#include "SDcard.h"





#define  AUDIO_ENCODE_NEED
//#define AUDIO_ENCODE_SAVE_FILE
#define AUDIO_SEND_USE_RTSP
#define RUN_AUDIO_ANALYSES

#define DFROBOT_CONFIG_FILE		"/etc/jffs2/dfrobot_config"	
#define DFROBOT_CONFIG_SD_FILE 	"/mnt/DFROBOT/dfrobot_config"
#define DFROBOT_PHOTO_DIR       "/mnt/DFRoBOT/photo/"
typedef struct dfrobot_config{
	//broad name
	char *name;         
	//wifi
	char *ssid;
	char *password;
	//baidu tts
	char *baiduApiId;
	char *baiduApiKey;
	char *baiduApiSecrtkey;

	//tuling chat
	char *tulingApiKey;
	char *tulingUserKey;
	//chat language
	char *language;
} df_config;

struct dfrobot_config *global_config = NULL;



#define  MAIN_BUF_SIZE             		(1*1024*1024)
#define DECODE_SIZE 4096*4
#define APLAY_SIZE	3000
#define CHANNAL 2
#define PER_AUDIO_DURATION		    20 		    // AMR一帧20ms
#define	AUDIO_SAMPLERATE			16000//8000
#define	AUDIO_BITS_PER_SAMPLE		16
#define	AUDIO_CHANNEL				1
#define AUDIO_INFORM_LENGTH (8)



#define TTY_COMMUNICATE_ARDUINO   "/dev/ttySAK0"
static int main_func_run_flag=0;
unsigned char startCamera_addr = 0x0;
long uart_fd;
static int  uart_pipe[2];
#define  UART_PIPE_READ          uart_pipe[0]
#define  UART_PIPE_WRITE         uart_pipe[1]
Interface *iinterface = NULL;
static unsigned char voice_size = 5;
static int take_photo = 0;

static int heartbeat_pc = 0;
static int heartbeat_arduino = 0;

static unsigned char current_resolution = 0;


const char*  interface_err[ERR_MAX+1] = 
{
	"all start ok",   //0
	"voide no running", // 1
	"video start ok",  
	"video start failed",
	"video stop ok",
	"audio no running", //5
	"audio start ok", 
	"audio start failed",
	"audio stop ok",
	"tuling no running",  //9
	"tuling start ok", 
	"tuling start failed",
	"tuling stop ok",
	"talk2talk no running", // 13
	"talk2talk start ok",  
	"talk2talk start failed",
	"talk2talk stop ok",
	"aplay no running",  //17
	"aplay start ok",
	"aplay start failed",
	"aplay stop ok",
	"wifi no running", //21
	"wifi start ok",
	"wifi start failed",
	"wifi stop ok",
	"resolution set ok", //25
	"resolution set failed",
	"frame set ok", //27
	"frame set failed", 
	"voice set ok", //29
	"voice set failed",
	"take photo ok",//31
	"take photo failed",
	"init config failed",//33
	"SD card not exist",//34
	"name set ok", //35
	"name set failed",
	"ssid set ok",//37
	"ssid set failed",
	"password set ok",//39
	"password set failed",
	"max"
};




#define SYS_START 						"/usr/share/anyka_camera_start.mp3"	


dfrobot_cycle_buff *save_audio_data;
dfrobot_auido_list_header *g_audio_list;
static bool rtsp_status_flag = false;


typedef struct DFROBOT_VIDEO_HANDLE
{
	uint8    encode_use_flag;    //这个编程器是否在工作
    uint8    video_size_flag;    //720p与VGA的选择，TRUE为720P
    uint8    frames;
    uint8    index;
	T_VOID  *encode_handle;
}dfrobot_video_handle, *Pdfrobot_video_handle;

dfrobot_video_handle encode_handle;


TransferServer *myServer;

static Condition		g_conMetex;
static Condition		g_aplayMetex;
static Condition		g_tulingMetex;
static unsigned int audio_size;
static unsigned char *audio_buf;

static int send_flag = 0;
static unsigned int package_len;
static uint8_t *jpeg_buf;

static int tuling_len = 0;
static uint8_t *tuling_data;
static char need_aplay_mp3_buffer[200];
static int aplay_flag = 0;


T_ENC_INPUT *encode_info = NULL;

pthread_t dfrobot_video_pthread;
pthread_t dfrobot_audio_record_pthread;
pthread_t dfrobot_handle_audio_pthread;
pthread_t dfrobot_play_audio_pthread;
pthread_t dfrobot_colour_recognize_pthread;
pthread_t dfrobot_rtsp_server_pthread;
pthread_t dfrobot_tuling_talk_pthread;
pthread_t dfrobot_transfer_video_data_phtread;
pthread_t dfrobot_broadcast_pthread;

static int video_mode_flag = 0;
static int audio_mode_flag = 0;
static int tuling_mode_flag = 0;
static int aplay_mode_flag = 0;
static int broadcast_flag = 0;

void set_current_voice_size(unsigned char val)
{
	voice_size = val;
}

unsigned char get_current_voice_size()
{
	return voice_size;
}

unsigned char get_current_resolution()
{
	return current_resolution;
}

void set_current_resolution(unsigned char val)
{
	current_resolution = val;
}



static df_config* read_config()
{
	df_config *config = (df_config *)malloc(sizeof(df_config));
	char file[100] = {0};
	int sd_exist = 0;
	
	if(config == NULL)
	{
		printf("malloc config error\r\n");
		return NULL;
	}
	config->name = NULL;
	config->password = NULL;
	config->ssid = NULL;
	config->baiduApiId = NULL;
	config->baiduApiKey = NULL;
	config->baiduApiSecrtkey = NULL;
	config->tulingApiKey = NULL;
	config->tulingUserKey = NULL;
	config->language = NULL;

	//sd_init_status();
	sd_exist = sd_get_status();

	

	if(sd_exist)
	{
		sprintf(file, "%s", DFROBOT_CONFIG_SD_FILE);
	}
	else
	{
		sprintf(file, "%s", DFROBOT_CONFIG_FILE);
	}


	config->name = read_element("name", file);
	config->password = read_element("password", file);
	config->ssid = read_element("ssid", file);
	config->baiduApiId = read_element("baiduApiId", file);
	config->baiduApiKey = read_element("baiduApiKey", file);
	config->baiduApiSecrtkey = read_element("baiduApiSecrtkey", file);
	config->tulingApiKey = read_element("tulingApiKey", file);
	config->tulingUserKey = read_element("tulingUserKey",file);
	config->language = read_element("language", file);

	return config;
	
}



static void config_exit(df_config *config)
{
	if(config != NULL)
	{
		if(config->name != NULL) free(config->name);
		if(config->ssid != NULL) free(config->ssid);
		if(config->password != NULL) free(config->password);
		if(config->baiduApiId != NULL) free(config->baiduApiId);
		if(config->baiduApiKey != NULL) free(config->baiduApiKey);
		if(config->baiduApiSecrtkey != NULL) free(config->baiduApiSecrtkey);
		if(config->tulingApiKey != NULL) free(config->tulingApiKey);
		if(config->tulingUserKey != NULL) free(config->tulingUserKey);
		if(config->language != NULL) free(config->language);
		free(config);
	}
}




bool set_tuling_param(TULING_SET_PARAM * param)
{
	
	int sd_exist = 0;
	char file[100] = {0};
	int len;

	if((param->baidu_user_id == NULL) || (param->baidu_api_key == NULL) ||
	   (param->baidu_api_sercret == NULL) || (param->tuling_user_id == NULL) || (param->tuling_api_key == NULL))
	{
		printf("tuling something == null\r\n");
		return false;
	}

	if(global_config->language != NULL)
	{
		//printf("language:%s\r\n", global_config->language);

		if(!strcmp("chinese", global_config->language))
		{
			iinterface->stop_tuling_mode();
		}
		else if(!strcmp("english", global_config->language))
		{
			//stop simsimi
		}
		free(global_config->language);
	}

	
	len = strlen("chinese")+1;
	global_config->language = (char *)malloc(len);
	
	if(global_config->language == NULL)
	{
		printf("set_tuling_param malloc error1\r\n");
		return false;
	}
	
	memcpy(global_config->language, "chinese", len);
	
	sd_exist = sd_get_status();
		
	if(sd_exist)
	{
		sprintf(file, "%s", DFROBOT_CONFIG_SD_FILE);
	}
	else
	{
		sprintf(file, "%s", DFROBOT_CONFIG_FILE);
	}
	
	
	write_element("language", global_config->language, file);


	//printf("baiduid:%s\r\n",global_config->baiduApiId);

	if(global_config->baiduApiId != NULL) 
	{
		free(global_config->baiduApiId);
	}
	
	
	len = strlen(param->baidu_user_id) +1;
	global_config->baiduApiId = (char *)malloc(len);
	if(global_config->baiduApiId == NULL)
	{
		printf("-----set_tuling_param malloc error---\r\n");
		return false;
	}
	memcpy(global_config->baiduApiId, param->baidu_user_id, len);
	write_element("baiduApiId", global_config->baiduApiId, file);
	

	//printf("baiduapikey= %s\r\n", global_config->baiduApiKey);
	
	if(global_config->baiduApiKey != NULL) 
		free(global_config->baiduApiKey);

	len = strlen(param->tuling_api_key) + 1;
	global_config->baiduApiKey = (char *)malloc(len);
	if(global_config->baiduApiKey == NULL)
	{
		printf("-----set_tuling_param malloc error---\r\n");
		return false;
	}
	memcpy(global_config->baiduApiKey, param->baidu_api_key, len);
	write_element("baiduApiKey", global_config->baiduApiKey, file);

	//printf("secrtkey:%s\r\n",global_config->baiduApiSecrtkey);
	if(global_config->baiduApiSecrtkey != NULL) 
		free(global_config->baiduApiSecrtkey);

	len = strlen(param->baidu_api_sercret) + 1;
	global_config->baiduApiSecrtkey = (char *)malloc(len);
	if(global_config->baiduApiSecrtkey == NULL)
	{
		printf("-----set_tuling_param malloc error---\r\n");
		return false;
	}
	memcpy(global_config->baiduApiSecrtkey, param->baidu_api_sercret, len);
	write_element("baiduApiSecrtkey", global_config->baiduApiSecrtkey,file);

	

	//printf("tulingkey:%s\r\n", global_config->tulingApiKey);
	if(global_config->tulingApiKey != NULL) 
		free(global_config->tulingApiKey);
	len = strlen(param->tuling_api_key) + 1;
	global_config->tulingApiKey = (char *)malloc(len);
	if(global_config->tulingApiKey == NULL)
	{
		printf("-----set_tuling_param malloc error---\r\n");
		return false;
	}
	memcpy(global_config->tulingApiKey, param->tuling_api_key, len);
	write_element("tulingApiKey", global_config->tulingApiKey, file);



	//printf("userkey:%s\r\n", global_config->tulingUserKey);
	if(global_config->tulingUserKey != NULL)
		free(global_config->tulingUserKey);
	len = strlen(param->tuling_user_id)+1;
	global_config->tulingUserKey = (char *)malloc(len);
	if(global_config->tulingUserKey == NULL)
	{
		printf("-----set_tuling_param malloc error---\r\n");
		return false;
	}
	memcpy(global_config->tulingUserKey, param->tuling_user_id, len);
	write_element("tulingUserKey", global_config->tulingUserKey, file);
	


	iinterface->start_tuling_mode();

	return true;
	
}


static void appExit()
{
	//这里后面再添加需要释放的
	printf("app exit\r\n");
	broadcast_flag = 0;
	pthread_cancel(dfrobot_rtsp_server_pthread);
	video_encode_destroy();
	config_exit(global_config);
}

#if 1
static void sigprocess(int sig)
{
	int ii = 0;
	void *tracePtrs[16];
	int count = backtrace(tracePtrs, 16);
	char **funcNames = backtrace_symbols(tracePtrs, count);
	for(ii = 0; ii < count; ii++)
		printf("%s\n", funcNames[ii]);
	free(funcNames);
	fflush(stderr);
	printf("##signal %d caught\n", sig);
	fflush(stdout);
	
	if(sig == SIGINT || sig == SIGSEGV || sig == SIGTERM)
	{
		appExit();	
	}

	exit(1);
}

static int sig_init(void)
{
	signal(SIGSEGV, sigprocess);
	signal(SIGINT, sigprocess);
	signal(SIGTERM, sigprocess);
	return 0;
}

#endif


int CheckRunOnly()
{
	int fd;
	int lock_result;
	//struct flock lock;
	const char* pFileName = "/etc/jffs2/hello.sh";
	fd = open((const char*)pFileName, O_RDWR);
	if(fd < 0)
	{
		printf("Open lock file failed.\n");
		return -1;
	}

	lock_result = lockf(fd, F_TEST, 0);
	if(lock_result < 0)
	{
		perror("exec lockf test function failed.\n");
		return -1;
	}

	lock_result = lockf(fd, F_LOCK, 0);
	if(lock_result < 0)
	{
		perror("exec lockf function failed.\n");
		return -1;
	}
	printf("lock success\n");
	return 0;
}






static void settings_init(T_ENC_INPUT *config);
void dfrobot_play_file(char *filename, int enc_type);






void* video_encode_and_read_data(void *arg)
{
	void *pbuf, *cur_frame_info;
	unsigned long ts;
	long size;	

	void *encode_buffer;
	void *pencode_outbuf;
	int IPFrame_type;
	int encode_size;

	struct timeval tvs1;
	struct timeval tvs2;
	static int count = 0;

	
	camera_on();

	
	
	//while(1) 
	while(video_mode_flag)
	{
		
		if((cur_frame_info = camera_getframe()) == NULL)
		{
			printf("camera no data \r\n");
			continue;
		}

		count++;
		gettimeofday(&tvs1, NULL);
		if(tvs1.tv_sec != tvs2.tv_sec)
		{
			tvs2.tv_sec = tvs1.tv_sec;
			//printf("count=%d\r\n",count);
			count=0;
		}
		
		camera_get_camera_info(cur_frame_info, (void**)&pbuf, &size, &ts);
		//printf("----------size=%d\r\n",size);

		//进行编码
		encode_buffer = pbuf;

		encode_size = video_encode_frame(encode_handle.encode_handle, encode_buffer, &pencode_outbuf, &IPFrame_type);
		//printf("output size=%d\r\n",encode_size);
		
		if((encode_size > 0) && (send_flag == 0))
		{
			//put the jpeg datas to send buffer
			send_flag = 1;
			memcpy(jpeg_buf, pencode_outbuf, encode_size);
			package_len = encode_size;
			send_flag = 0;
			//send signal
			Condition_Lock( g_conMetex );
			Condition_Signal( &g_conMetex );
			Condition_Unlock( g_conMetex);
		}
		
		//printf("output size=%d\r\n",encode_size);
		#if 0 //write jpeg into file
			//T_pSTR filename;
			//filename = MakeFileName();
			static count=0;
			count++;
			char filename[100];
			sprintf(filename,"/mnt/DC1980011/%d.jpeg",count);
			
			printf("filename is %s\r\n",filename);
		
			long fid = open(filename,  O_CREAT | O_APPEND | O_TRUNC | O_WRONLY);
			if(fid <= 0)
			{
				printf("open file error\r\n");
			}
			write(fid, pencode_outbuf,encode_size);
			close(fid);
			//free(filename);
			
		#endif

		if(take_photo == 1)
		{
			take_photo = 0;
			time_t now = time(NULL);
			struct tm *timeinfo = localtime(&now);
			char buf[100] = {0};
			char buf2[100] = {0};
			strftime(buf, sizeof(buf),"%Y_%m_%d_%H_%M_%S.jpeg",timeinfo);
			sprintf(buf2, "%s%s",DFROBOT_PHOTO_DIR, buf);
			long fid = open(buf2,  O_CREAT | O_APPEND | O_TRUNC | O_WRONLY);
			if(fid <= 0)
			{
				printf("open file error\r\n");
			}
			else
			{
				write(fid, pencode_outbuf,encode_size);
				close(fid);
			}
		}

		camera_usebufok(cur_frame_info);
		
	}

	 camera_off();
}


void callfunc(unsigned char *buffer, int len)
{
	tuling_len = len;
	memcpy(tuling_data, buffer, len);
	Condition_Lock( g_tulingMetex );
	Condition_Signal( &g_tulingMetex );
	Condition_Unlock( g_tulingMetex);	
}


void *audio_record_data_write_queue(void *arg)
{
	//printf("auio record data and write queue pthread start \r\n");
	T_AUDIO_INPUT audioInput;
	void    *ad_handle;
	unsigned long 	audioOnceBufSize;
	unsigned char  	*pRawBuffer = NULL;
	unsigned long 	nTimeStamp = 0;
	unsigned char  *pdata = NULL;

	void    *audio_encode_handle = NULL;
	AudioEncIn	stEncIn;
	AudioEncOut	stEncOut;
	unsigned char *audio_encode_buffer = NULL;
	int audio_encode_size = 0;
	unsigned long nPcmLen;


	ADTSContext ctx;
	unsigned char adts_header[7];
	struct timeval tvs1;
	struct timeval tvs2;
	int		after;
	char filename[100];
	long fd = -1;
	int file_count = 1;

#ifdef RUN_AUDIO_ANALYSES
	if(0 != audio_analyze_init(&callfunc))
	{
		printf("audio analyze_init error\r\n");
		goto out;
	}
#endif

	
#ifdef AUDIO_ENCODE_NEED
	stEncIn.nChannels = AUDIO_CHANNEL;
	stEncIn.nBitsPerSample = AUDIO_BITS_PER_SAMPLE;
	stEncIn.nSampleRate = AUDIO_SAMPLERATE;

	stEncOut.enc_type = ENC_TYPE_AAC;

	setADTSContext(&ctx, 1, AUDIO_SAMPLERATE, AUDIO_CHANNEL);

	audio_encode_handle = audio_enc_open(&stEncIn, &stEncOut);
	if(audio_encode_handle == NULL)
	{
		printf("open aac codec failed\r\n");
		goto out;
	}
	
#endif

	audioInput.nBitsPerSample = AUDIO_BITS_PER_SAMPLE;
	audioInput.nChannels = AUDIO_CHANNEL;
	audioInput.nSampleRate = AUDIO_SAMPLERATE;

	

	

	audioOnceBufSize = (audioInput.nSampleRate * audioInput.nChannels * PER_AUDIO_DURATION)/1000;

	if(audioOnceBufSize & 1)
	{
		audioOnceBufSize++;
	}
#ifdef AUDIO_ENCODE_NEED
	audio_encode_size = audioOnceBufSize;
	audio_encode_buffer = (unsigned char *) malloc(2000/*audio_encode_size * 2*/);
	if(audio_encode_buffer == NULL)
	{
		printf("malloc audio encode buffer error\r\n");
		goto out;
	}
	
#endif
	

	ad_handle = audio_ad_open(&audioInput);
	if(ad_handle == NULL)
	{
		printf("audio ad open failed\r\n");
		goto out;
	}

	pRawBuffer = (unsigned char*)malloc(audioOnceBufSize*2 + AUDIO_INFORM_LENGTH);
	if(pRawBuffer == NULL)
	{
		printf("pRawBuffer malloc error\r\n");
		goto out;
	}

	pdata = pRawBuffer + AUDIO_INFORM_LENGTH; //point data save address
	bzero(pdata, audioOnceBufSize*2);
	

	audio_ad_filter_enable(ad_handle,1);
	audio_set_ad_agc_enable(1);

	int ret;
	save_audio_data->stepsize = audioOnceBufSize;

	//while(1)
	while(audio_mode_flag)
	{	
		if(aplay_flag == 1)//播放的时候不能录音，有回音
		{
#ifdef RUN_AUDIO_ANALYSES
			audio_bzero_voice_data();
#endif
			usleep(500*1000);
			continue;
		}
		//get the pcm data
		ret = audio_read_ad(ad_handle, pdata, audioOnceBufSize, (unsigned long *)&nTimeStamp);
		//printf("----audio:%d\r\n",ret);

		if(ret < 0)
		{
			printf("read audio from AD error!\r\n");
			continue;
		}
		else if(ret == 0)
		{
			printf("ad runing error! but already recover this error!\r\n");
			continue;
		}

#ifdef RUN_AUDIO_ANALYSES
		//TODO: write queue for audio handler
		//write_cycle_buff(save_audio_data, pdata, audioOnceBufSize);
		//printf("++++cycle buff size=%d\r\n",save_audio_data->count);
		audio_frame_analyze1(pdata, audioOnceBufSize);
#endif
		
		//encode audio to aac
#ifdef AUDIO_ENCODE_NEED
		
		//set the time
    	*((T_U32 *)pRawBuffer) = nTimeStamp;//bytenum * 1000 / rate;
    	nPcmLen = ret;
    	ret = audio_encode(audio_encode_handle, pRawBuffer + AUDIO_INFORM_LENGTH, nPcmLen, audio_encode_buffer, audio_encode_size*2);
		//printf("-----npcmlen=%d,--ret=%d-\r\n",nPcmLen,ret);
	#ifdef AUDIO_ENCODE_SAVE_FILE   //save to file for test
		if(fd < 0)
		{
			gettimeofday(&tvs1, NULL);
			tvs2.tv_sec = tvs1.tv_sec;
			sprintf(filename, "/mnt/pcm/test8k%d.aac", file_count);
			fd = open(filename,  O_CREAT | O_APPEND | O_TRUNC | O_WRONLY);
			if(fd < 0)
			{
				printf("open %s error\r\n",filename);
			}
		}

		gettimeofday(&tvs1, NULL);

		if(tvs1.tv_sec >= tvs2.tv_sec)
		{
			after = tvs1.tv_sec;
		}
		else
		{
			after = tvs1.tv_sec + 59;
		}

			
		if((after - tvs2.tv_sec) > 30)//30s save a signal file
		{
			close(fd);
			file_count++;
			tvs2.tv_sec = tvs1.tv_sec;
			sprintf(filename, "/mnt/pcm/test8k%d.aac", file_count);
			printf("pcm new file is :%s\r\n",filename);
			fd = open(filename,  O_CREAT | O_APPEND | O_TRUNC | O_WRONLY);
			if(fd < 0)
			{
					printf("open %s file failed\r\n", filename);
					goto out;
			}
				
		}
		
		if(ret > 0)
		{
			//add adts header
			printf("ret=%d\r\n",ret);
			adts_write_frame_header(&ctx, adts_header, ret, 0);
			write(fd, adts_header, 7);
			write(fd, audio_encode_buffer,ret);
		}
	
		
	#endif  //AUDIO_ENCODE_SAVE_FILE

	#ifdef AUDIO_SEND_USE_RTSP
		if(ret > 0)
		{
			//printf("g_audio_list size=%d\r\n",g_audio_list->list_size);
			//if(rtsp_status_flag == false)
			{
				while(g_audio_list->list_size >= 5)
				{
					list_delete_head(g_audio_list);
				}
			}
			
			dfrobot_audio_list* list = list_init(ret);
			memcpy(list->data, audio_encode_buffer, ret);
			list_add_tail(g_audio_list, list);
			list = NULL;
		}
	#endif
		
#endif  //AUDIO_ENCODE_NEED

	}

out:

	if(audio_encode_handle != NULL)
	{
		audio_enc_close(audio_encode_handle);
	}

	if(pRawBuffer != NULL)
	{
		free(pRawBuffer);
	}
	if(audio_encode_buffer != NULL)
	{
		free(audio_encode_buffer);
	}
	if(ad_handle != NULL)
	{
		audio_ad_filter_enable(ad_handle,0);
		audio_set_ad_agc_enable(0);
		audio_ad_close(ad_handle);
		
	}

	if(fd >= 0)
	{
		close(fd);
	}
#ifdef RUN_AUDIO_ANALYSES
	audio_analyze_exit();
#endif
}



UsageEnvironment* env;
Boolean reuseFirstSource = False;
Boolean iFramesOnly = False;
void setLiveSourceCallBack(ADTSAudioLiveSource* subsms);
void LiveSourceCloseNotify(ADTSAudioLiveSource* subsms);

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,
			   char const* streamName, char const* inputFileName);


RTSPServer* rtspServer = NULL;
ServerMediaSession* sms = NULL;


static void rtsp_init()
{
	
#if 1
		// Begin by setting up our usage environment:
		TaskScheduler* scheduler = BasicTaskScheduler::createNew();
		env = BasicUsageEnvironment::createNew(*scheduler);
		
		UserAuthenticationDatabase* authDB = NULL;
#ifdef ACCESS_CONTROL
		// To implement client access control to the RTSP server, do the following:
		authDB = new UserAuthenticationDatabase;
		authDB->addUserRecord("username1", "password1"); // replace these with real strings
		// Repeat the above with each <username>, <password> that you wish to allow
		// access to the server.
#endif
		// Create the RTSP server:
		rtspServer = RTSPServer::createNew(*env, 554, authDB);
		if (rtspServer == NULL) {
			*env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
			exit(1);
		}
	
#endif
}




void *rtsp_server(void *arg)
{

#if 0
	// Begin by setting up our usage environment:
  	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  	env = BasicUsageEnvironment::createNew(*scheduler);
	
	UserAuthenticationDatabase* authDB = NULL;
#ifdef ACCESS_CONTROL
	// To implement client access control to the RTSP server, do the following:
	authDB = new UserAuthenticationDatabase;
	authDB->addUserRecord("username1", "password1"); // replace these with real strings
	// Repeat the above with each <username>, <password> that you wish to allow
	// access to the server.
#endif
	// Create the RTSP server:
 	RTSPServer* rtspServer = RTSPServer::createNew(*env, 554, authDB);
 	if (rtspServer == NULL) {
   		*env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
   		exit(1);
 	}

#endif
	char const* descriptionString
    	= "Session streamed by \"testOnDemandRTSPServer\"";

	// An AAC audio stream (ADTS-format file):
  	{
    	char const* streamName = "aacAudioTest";
    	char const* inputFileName = "test.aac";
    	//ServerMediaSession* sms
     	sms = ServerMediaSession::createNew(*env, streamName, streamName,
				      descriptionString);
		ADTSAudioLiveServerMediaSubsession *subsms = ADTSAudioLiveServerMediaSubsession::createNew(*env, reuseFirstSource, 1, AUDIO_SAMPLERATE, AUDIO_CHANNEL);
		
    	sms->addSubsession(subsms);
		subsms->CallBackSubSms = setLiveSourceCallBack;
		subsms->CallBackSubSmsClose = LiveSourceCloseNotify;
    	rtspServer->addServerMediaSession(sms);

    	announceStream(rtspServer, sms, streamName, inputFileName);
  	}

	env->taskScheduler().doEventLoop(); // does not return

	
}

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,
			   char const* streamName, char const* inputFileName) {
  char* url = rtspServer->rtspURL(sms);
  UsageEnvironment& env = rtspServer->envir();
  env << "\n\"" << streamName << "\" stream, from the file \""
      << inputFileName << "\"\n";
  env << "Play this stream using the URL \"" << url << "\"\n";
  delete[] url;
}

static int rtspGetAudioData(unsigned char* buffer, int len)
{
	dfrobot_audio_list* list;
	list = list_pop_head(g_audio_list);

	if(list == NULL) return 0;
	if(list->data == NULL) 
	{
		list_destroy(list);
		list = NULL;
		return 0;
	}
	
	int size = len > list->len ? list->len : len;
		
	memcpy(buffer, list->data, size);
	list_destroy(list);
	list = NULL;

	return size;
}


void setLiveSourceCallBack(ADTSAudioLiveSource* subsms)
{
	subsms->getAudioData = rtspGetAudioData;
	printf("vlc read audio start\r\n");
	rtsp_status_flag = true;
}
void LiveSourceCloseNotify(ADTSAudioLiveSource* subsms)
{
	printf("vlc stop read\r\n");
	rtsp_status_flag = false;
}

static void close_session()
{
	rtsp_status_flag = false;
	if(sms != NULL)
		sms->deleteAllSubsessions();
}





size_t my_write_func_mp3(void *ptr, size_t size, size_t nmemb, char **result)
{
	if(ptr == NULL)
	{
		printf("ptr = NULL \r\n");
		return 0;
	}

	if(aplay_flag == 1)
	{
		printf("aplay running!!!\r\n");
		return 0;
	}
	time_t now = time(NULL);
	struct tm *timeinfo = localtime(&now);
	//char buf[100];
	strftime(need_aplay_mp3_buffer, sizeof(need_aplay_mp3_buffer),"/tmp/%Y_%m_%d_%H_%s.mp3",timeinfo);
	FILE *fd = fopen(need_aplay_mp3_buffer, "wb+");
	fwrite(ptr, 1, size*nmemb, fd);
	fclose(fd);

	Condition_Lock( g_aplayMetex );
	Condition_Signal( &g_aplayMetex );
	Condition_Unlock( g_aplayMetex);

	return (size*nmemb);
	
	
}



void *tuling_talk(void *arg)
{
	set_baidu_api(global_config->baiduApiId, global_config->baiduApiKey,global_config->baiduApiSecrtkey);

	set_tuling_api(global_config->tulingApiKey, global_config->tulingUserKey);
	
	while(tuling_mode_flag)
	{

#if 1
		Condition_Lock( g_tulingMetex );
		Condition_Wait( &g_tulingMetex );
		Condition_Unlock( g_tulingMetex );

		//talk to talk running ,not run tuling
		if(rtsp_status_flag == true) continue;  

		if(tuling_len > 0)
		{
			printf("-----------tuling_len=%d-------\r\n", tuling_len);

			talk_to_talk_with_tuling(tuling_data, tuling_len, my_write_func_mp3);
		}
		
		tuling_len = 0;
#endif

	}

	tuling_exit();

}






void *aplay_mp3_data(void *arg)
{
	printf("play audio data pthread start\r\n");

	//while(1)
	while(aplay_mode_flag)
	{
		Condition_Lock( g_aplayMetex );
		Condition_Wait( &g_aplayMetex );
		aplay_flag=1;
		printf("mp3 filename:%s\r\n",need_aplay_mp3_buffer);
		dfrobot_play_file(need_aplay_mp3_buffer, _SD_MEDIA_TYPE_MP3);
		char buffer[250];
		sprintf(buffer, "rm -rf %s",need_aplay_mp3_buffer);
		system(buffer);
		aplay_flag = 0;
		Condition_Unlock( g_aplayMetex );
	}
}


//play mp3 from file
//
void dfrobot_play_file(char *filename, int enc_type)
{
	long fd;
	void *decode_handle;
	void *da_handle;
	media_info pmedia;		//decode
	pmedia.m_AudioType = enc_type;
	//pmedia.m_AudioType = _SD_MEDIA_TYPE_MP3;
	pmedia.m_nChannels = 2;
	pmedia.m_nSamplesPerSec = 0;
	pmedia.m_wBitsPerSample = 0;

	fd = open(filename, O_RDONLY);
	if(fd < 0)
	{
		printf("open %s mp3 file failed\r\n",filename);
		return ;
	}
	
	decode_handle = audio_decode_open(&pmedia);
	if(decode_handle == NULL)
	{
		printf("init decode failed\r\n");
		close(fd);
		return ;
	}
	audio_decode_change_mode(decode_handle);

	audio_da_spk_enable(1);
	//decode and write ad
	{
		int free_size, end_flag = 1, ret, decode_size, da_init_flag, station = 0;
		unsigned char *decode_buf = (unsigned char*)malloc(DECODE_SIZE);
		unsigned char *temp_buf = (unsigned char*)malloc(DECODE_SIZE*2);
		unsigned char *decode_outbuf = (unsigned char*)malloc(DECODE_SIZE*2);
		if((decode_buf == NULL) || (temp_buf == NULL) || (decode_outbuf == NULL))
		{
			printf("malloc failed\r\n");
			if(decode_buf != NULL)
			{
				free(decode_buf);
			}
			if(temp_buf != NULL)
			{
				free(temp_buf);
			}
			if(decode_outbuf != NULL)
			{
				free(decode_outbuf);
			}
			goto out;
		}
		da_init_flag = 1;
		while(1)
		{
			
			free_size = audio_decode_get_free(decode_handle);
			
			if((free_size > DECODE_SIZE) && end_flag)
			{
				free_size = DECODE_SIZE;
				ret = read(fd, decode_buf, free_size);
				if(ret > 0)
				{
					audio_decode_add_data(decode_handle, decode_buf, ret);
				}
				else
				{
					//printf("[%s:%d]Read to the end of audio file.\n", __func__, __LINE__);
					end_flag = 0;
				}
			}
				
			station = 0;
			while((decode_size = audio_decode_decode(decode_handle,(uint8*)decode_outbuf, CHANNAL)) > 0)
			{
				if(da_init_flag)
				{
					T_AUDIO_INPUT param;
					audio_decode_get_info(decode_handle, &param);	
					//16bit 2c 16000
					//printf("--b:%d, c:%d, s:%d------\r\n",param.nBitsPerSample, param.nChannels,param.nSampleRate);
					da_handle = audio_da_open(&param);
					if(da_handle == NULL)
					{
						printf("open audio da file\r\n");
						free(temp_buf);
						free(decode_buf);
						free(decode_outbuf);
						goto out;
					}
					//audio_set_da_vol(da_handle, 5);
					audio_set_da_vol(da_handle, voice_size);//jingwenyi add 2016.8.9
					audio_da_clr_buffer(da_handle);
					da_init_flag = 0;
				}
				//store data to temp buffer
				memcpy(temp_buf + station, decode_outbuf, decode_size); 
				station += decode_size;
				if(station < DECODE_SIZE)
				{
					continue;
				}
				//printf("-----------station:%d---------\r\n",station);
				audio_write_da(da_handle, (uint8*)temp_buf, station);
					
				break;
			}
			if((decode_size <= 0) && station)
			{
				audio_write_da(da_handle, temp_buf, station);
			}

			if((station == 0) && (end_flag == 0))
			{
				break;
			}
	
		}
		free(temp_buf);
		free(decode_outbuf);
		free(decode_buf);
		
	}

out:
	close(fd);
	sleep(1);
	if(da_handle != NULL)
	{
		audio_da_close(da_handle);
	}
	audio_decode_close(decode_handle);
	audio_da_spk_enable(0);

}



void dfrobot_play_pcm(unsigned char *audio_buff, long len, T_AUDIO_INPUT* param)
{
	T_AUDIO_INPUT tmp_param;
	void *da_handle;
	unsigned char *tmp = audio_buff;
	long tmp_len = 0;
	long count_size = 0;
	if(param == NULL)
	{
		printf("audio param is null");
		return;
	}
	if(audio_buff == NULL)
	{
		printf("user input a null ptr\r\n");
		return;
	}

	tmp_param.nBitsPerSample = param->nBitsPerSample;
	tmp_param.nSampleRate = param->nSampleRate;
	tmp_param.nChannels = param->nChannels;

	audio_da_spk_enable(1);
	
	da_handle = audio_da_open(&tmp_param);
	if(da_handle == NULL)
	{
		printf("audio handle get error\r\n");
		goto out;
	}
	audio_set_da_vol(da_handle, 2);
	audio_da_clr_buffer(da_handle);

	tmp_len = len;
	count_size = 0;
	while(1)
	{
		if(tmp_len > APLAY_SIZE)
		{
			
			
			audio_write_da(da_handle, &audio_buff[count_size], APLAY_SIZE);
			tmp_len -= APLAY_SIZE;
			count_size += APLAY_SIZE;

		}
		else
		{
			audio_write_da(da_handle, &audio_buff[count_size], tmp_len);
			break;
		}
		
	}
	
out:
	
	sleep(1);
	if(da_handle != NULL)
	{
		audio_da_close(da_handle);
	}
	audio_da_spk_enable(0);
	
}


void dfrobot_play_8k_1c_16b_pcm(unsigned char *audio_buff,long len)
{
	T_AUDIO_INPUT param;
	param.nBitsPerSample = 16;
	param.nChannels = 1;
	param.nSampleRate = 8000;

	dfrobot_play_pcm(audio_buff, len, &param);
	
}


void dfrobot_play_16k_1c_16b_pcm(unsigned char *audio_buff,long len)
{
	T_AUDIO_INPUT param;
	param.nBitsPerSample = 16;
	param.nChannels = 1;
	param.nSampleRate = 16000;

	dfrobot_play_pcm(audio_buff, len, &param);
	
}


void dfrobot_audio_record(T_AUDIO_INPUT *input)
{
	T_AUDIO_INPUT config;
	void    *ad_handle;
	unsigned long 	audioOnceBufSize;
	unsigned char  	*pRawBuffer = NULL;
	unsigned long 	nTimeStamp = 0;
	long fd = -1;
	int ret;
	
	if(input == NULL)
	{
		printf("audio input param is null\r\n");
		return ;
	}

	config.nSampleRate = input->nSampleRate;
	config.nBitsPerSample = input->nBitsPerSample;
	config.nChannels = input->nChannels;

	audioOnceBufSize = (config.nSampleRate * config.nChannels * PER_AUDIO_DURATION)/1000;

	ad_handle = audio_ad_open(&config);
	if(ad_handle == NULL)
	{
		printf("audio ad open failed\r\n");
		goto out;
	}

	pRawBuffer = (unsigned char*)malloc(audioOnceBufSize*2);
	if(pRawBuffer == NULL)
	{
		printf("pRawBuffer malloc error\r\n");
		goto out;
	}

	audio_ad_filter_enable(ad_handle,1);
	audio_set_ad_agc_enable(1);

	struct timeval tvs1;
	struct timeval tvs2;
	int		after;
	static int count = 1;
	char filename[100];
	sprintf(filename, "/mnt/pcm/test16k%d.pcm", count);
	fd = open(filename,  O_CREAT | O_APPEND | O_TRUNC | O_WRONLY);
	if(fd < 0)
	{
		printf("open %s file failed\r\n",filename);
		goto out;
	}

	while(1)
	{
		//get the pcm data
		ret = audio_read_ad(ad_handle, pRawBuffer, audioOnceBufSize, (unsigned long *)&nTimeStamp);

		if(ret < 0)
		{
			printf("read audio from AD error!\r\n");
			continue;
		}
		else if(ret == 0)
		{
			printf("ad runing error! but already recover this error!\r\n");
			continue;
		}

		gettimeofday(&tvs1, NULL);

		if(tvs1.tv_sec >= tvs2.tv_sec)
		{
			after = tvs1.tv_sec;
		}
		else
		{
			after = tvs1.tv_sec + 59;
		}

			
		if((after - tvs2.tv_sec) > 10)//30s save a signal file
		{
			close(fd);
			count++;
			tvs2.tv_sec = tvs1.tv_sec;
			sprintf(filename, "/mnt/pcm/test16k%d.pcm", count);
			printf("pcm new file is :%s\r\n",filename);
			fd = open(filename,  O_CREAT | O_APPEND | O_TRUNC | O_WRONLY);
			if(fd < 0)
			{
					printf("open %s file failed\r\n", filename);
					goto out;
			}
				
		}

		write(fd, pRawBuffer,audioOnceBufSize);
	}

	

out:
	if(pRawBuffer != NULL)
	{
		free(pRawBuffer);
	}
	if(ad_handle != NULL)
	{
		audio_ad_filter_enable(ad_handle,0);
		audio_set_ad_agc_enable(0);
		audio_ad_close(ad_handle);
		
	}
	if(fd >= 0)
	{
		close(fd);
	}
	
}
	

void dfrobot_record_8k_1c_16b_audio()
{
	T_AUDIO_INPUT audioInput;
	audioInput.nBitsPerSample = 16;
	audioInput.nChannels = 1;
	audioInput.nSampleRate = 8000;

	dfrobot_audio_record(&audioInput);
}

void dfrobot_record_16k_1c_16b_audio()
{
	T_AUDIO_INPUT audioInput;
	audioInput.nBitsPerSample = 16;
	audioInput.nChannels = 1;
	audioInput.nSampleRate = 16000;

	dfrobot_audio_record(&audioInput);
}



void pc_voice_mp3_notify(const char *filename) //jwy add 2016.7.27
{
	if(aplay_flag == 1)
	{
		char buf[250];
		sprintf(buf, "rm -rf %s", filename);
		system(buf);
		return ;
	}
	memcpy(need_aplay_mp3_buffer, filename, (strlen(filename)+1));
	Condition_Lock( g_aplayMetex );
	Condition_Signal( &g_aplayMetex );
	Condition_Unlock( g_aplayMetex);
}



#if 0

int main()
{

	T_ENC_INPUT *encode_info = NULL;

	pthread_t dfrobot_video_pthread;
	pthread_t dfrobot_audio_record_pthread;
	pthread_t dfrobot_handle_audio_pthread;
	pthread_t dfrobot_play_audio_pthread;
	pthread_t dfrobot_colour_recognize_pthread;
	pthread_t dfrobot_rtsp_server_pthread;
	pthread_t dfrobot_tuling_talk_pthread;
	

	 
	 

	if(CheckRunOnly() < 0)
	{
		printf("!!!!dfrobot_test is running\n");
		return -1;
	}


	sig_init();

	Condition_Initialize( &g_conMetex );
	Condition_Initialize(&g_aplayMetex);
	Condition_Initialize(&g_tulingMetex);
	
	jpeg_buf = new uint8_t[MAIN_BUF_SIZE];

	if(jpeg_buf == NULL)
	{
		printf("jpeg_buf malloc failed\r\n");
		goto out;
	}
	audio_buf = new unsigned char[MAIN_BUF_SIZE];

	if(audio_buf == NULL)
	{
		printf("audio buf failed\r\n");
		goto out;
	}

	tuling_data = new uint8_t[MAIN_BUF_SIZE];
	if(tuling_data == NULL)
	{
		printf("tuling_data new error\r\n");
		goto out;
	}

	save_audio_data = cycle_buff_init();

	if(save_audio_data == NULL)
	{
		printf("cycle_buff_init error\r\n");
		goto out;
	}
	g_audio_list = list_header_init();
	if(g_audio_list == NULL)
	{
		printf("list init error\r\n");
		goto out;
	}


	//初始化硬件
	PTZControlInit();
	//init dma memory  //以后可能需要添加语音支持
	akuio_pmem_init();
	if (0 != camera_open())
	{
		printf("camera open failed\r\n");
		goto out;
	}
	
	camera_ioctl(1280, 720);
	printf("came open ok!\r\n");

	//初始化软件
	encode_info = (T_ENC_INPUT *)malloc(sizeof(T_ENC_INPUT));
	
	encode_handle.video_size_flag = 1;//720p
	encode_handle.frames = 30;//camera_get_max_frame();
	encode_handle.index = 0;
	encode_handle.encode_use_flag = 0;
	
	settings_init(encode_info);
	
	video_encode_init();
	
	encode_handle.encode_handle = video_encode_open(encode_info);

	dfrobot_play_file(SYS_START, _SD_MEDIA_TYPE_MP3);
	//dfrobot_play_file("/mnt/test.mp3", _SD_MEDIA_TYPE_MP3);
	

#if 0  //test for record audio
	//dfrobot_record_8k_1c_16b_audio();
	dfrobot_record_16k_1c_16b_audio();

	return 0;

#endif

#if 0  //test for playback audio
	//这里只能用8k 速率播放16k 采样的语音
	
	unsigned char* audio_buf = (unsigned char *)malloc(3*1024*1024);
	long fd = open("/mnt/test/test.pcm", O_RDONLY);
	if(fd < 0)
	{
		printf("open binyu8k16bit.pcm error\r\n");
		return -1;
	}
	long size = read(fd, audio_buf, 2*1024*1024);
	
	dfrobot_play_8k_1c_16b_pcm(audio_buf, size);
#endif

	

	
	myServer = TransferServer::Create();
  	myServer->start();
	

	//video record and encode pthread
	if (0 != anyka_pthread_create(&dfrobot_video_pthread,  &video_encode_and_read_data, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, 99)) 
    {
    }

	//audio record and write queue  pthread
	if(0 != anyka_pthread_create(&dfrobot_audio_record_pthread, &audio_record_data_write_queue, NULL, ANYKA_THREAD_MIN_STACK_SIZE, 85))
	{
	}

	
	//play audio data pthread
	if(0 != anyka_pthread_create(&dfrobot_play_audio_pthread, &aplay_mp3_data, NULL, ANYKA_THREAD_MIN_STACK_SIZE,85))
	{
	}
	//colour recognize pthread
	if(0 != anyka_pthread_create(&dfrobot_colour_recognize_pthread, &colour_recognize, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, 99))
	{
	}
	//rtsp server pthread
	if(0 != anyka_pthread_create(&dfrobot_rtsp_server_pthread, &rtsp_server, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, 90))
	{
	}
	//tuling talk  pthread
	if(0 != anyka_pthread_create(&dfrobot_tuling_talk_pthread, &tuling_talk, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, 90))
	{
	}
	

	
	//main pthread handle sending video jpeg data
	while(1)
	{
		Condition_Lock( g_conMetex );
		Condition_Wait( &g_conMetex );
		Condition_Unlock( g_conMetex );
		
		while(send_flag) continue;
		send_flag = 1;
		myServer->Package_data(jpeg_buf, package_len);
		send_flag = 0;

		myServer->SendJpegDataToClient();
		
	}

out:

	int *thread_ret = NULL;
	pthread_join(dfrobot_video_pthread, (void**)&thread_ret);	
	pthread_join(dfrobot_audio_record_pthread, (void**)&thread_ret);
	pthread_join(dfrobot_handle_audio_pthread, (void**)&thread_ret);
	pthread_join(dfrobot_play_audio_pthread, (void**)&thread_ret);
	pthread_join(dfrobot_colour_recognize_pthread, (void**)&thread_ret);
	pthread_join(dfrobot_rtsp_server_pthread, (void**)&thread_ret);
	pthread_join(dfrobot_tuling_talk_pthread, (void**)&thread_ret);
	
	Condition_Destroy( &g_conMetex );
	Condition_Destroy(&g_aplayMetex);
	Condition_Destroy(&g_tulingMetex);
	
	if(jpeg_buf != NULL)
	{
		delete []jpeg_buf;
		jpeg_buf = NULL;
	}
	
	if(audio_buf != NULL)
	{
		delete []audio_buf;
		audio_buf = NULL;
	}
	
	if(save_audio_data != NULL)
	{
		cycle_buff_destroy(save_audio_data);
		save_audio_data = NULL;
	}
	
	if(g_audio_list != NULL)
	{
		list_header_destroy(g_audio_list);
		g_audio_list = NULL;
	}

	if(tuling_data != NULL)
	{
		delete []tuling_data;
		tuling_data = NULL;
	}
	
	return 0;
}

#else 

void *video_encode_transfer_data(void *arg)
{
	while(video_mode_flag)
	{
		Condition_Lock( g_conMetex );
		Condition_Wait( &g_conMetex );
		Condition_Unlock( g_conMetex );
		
		while(send_flag) continue;
		send_flag = 1;
		myServer->Package_data(jpeg_buf, package_len);
		send_flag = 0;

		myServer->SendJpegDataToClient();
	}
}

#define DFROBOT_BROADCAST_PORT  8888


void *broadcast_ip(void *arg)
{
	int sock;
	struct sockaddr_in broadcast_addr;
	char buffer[1024] = {0};
	struct ifreq *ifr;
	struct ifconf ifc;
	int j=0;
	int ret=0;
	int so_broadcast = 1;

	char broadcast_content[200] = {0};
	Json::Value item;
	struct ifaddrs *ifAddrStrcut = NULL;
	void *tmpAddrPtr = NULL;
	
	getifaddrs(&ifAddrStrcut);

	while(ifAddrStrcut != NULL)
	{
		if(ifAddrStrcut->ifa_addr->sa_family == AF_INET) //ipv4
		{
			tmpAddrPtr = &((struct sockaddr_in*)ifAddrStrcut->ifa_addr)->sin_addr;
			char addressBuffer[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			printf("%s:%s\r\n", ifAddrStrcut->ifa_name, addressBuffer);
			if((strcmp("eth0", ifAddrStrcut->ifa_name) == 0) || (strcmp("wlan0", ifAddrStrcut->ifa_name) == 0))
			{
				//sprintf(broadcast_content, "dfrobotIP:%s;boardName:%s", addressBuffer, global_config->name);
				item["dfrobotIP"] = addressBuffer;
				//item["boardName"] = global_config->name;
				break;
			}
			
		}
	#if 0
		else if(ifAddrStrcut->ifa_addr->sa_family == AF_INET6)
		{
			tmpAddrPtr = &((struct sockaddr_in*)ifAddrStrcut->ifa_addr)->sin_addr;
			char addressBuffer[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
			printf("%s:%s\r\n", ifAddrStrcut->ifa_name, addressBuffer);
		}
	#endif

		ifAddrStrcut = ifAddrStrcut->ifa_next;
		
	}

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0)
	{
		perror("broad sock error\r\n");
		return NULL;
	}


	//获取嵌套字接口
	ifc.ifc_len = sizeof(buffer);
	ifc.ifc_buf = buffer;
	if(ioctl(sock, SIOCGIFCONF, (char*)&ifc) < 0)
	{
		perror("ioctl-conf error\r\n");
		return NULL;
	}

	ifr = ifc.ifc_req;

	for(j = ifc.ifc_len/sizeof(struct ifreq); --j >= 0; ifr++)
	{
		
		if((!strcmp(ifr->ifr_name, "eth0")) || (!strcmp(ifr->ifr_name, "wlan0")))
		{
			if(ioctl(sock, SIOCGIFFLAGS, (char*)ifr) < 0)
			{
				perror("ioctl-get flag failed:");
			}
			break;
		}
		
	}

	if(ioctl(sock, SIOCGIFBRDADDR, ifr)== -1)
	{
		perror("ioctl error\r\n");
		return NULL;
	}
	
	memcpy(&broadcast_addr, (char *)&ifr->ifr_broadaddr, sizeof(struct sockaddr_in));
	printf("broadcast addr:%s\r\n",inet_ntoa(broadcast_addr.sin_addr));
	//设置广播端口号
	broadcast_addr.sin_family = AF_INET;
	broadcast_addr.sin_port = htons(DFROBOT_BROADCAST_PORT);

	//设置socket 支持广播
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &so_broadcast, sizeof(so_broadcast));

	int cnt = 0;
	int heartbeat_cnt = 0;
	
#if 0
	struct err_stat err;

	err.cmd = CMD_HEARTBEAT_PACKET;
	err.data = NULL;
	err.len = 0;
#endif
	
	while(broadcast_flag)
	{
		item["boardName"] = global_config->name;
		std::string sendString = item.toStyledString();

		//printf("%s\r\n",sendString.c_str());
	
		sleep(1);
		//sendto(sock, broadcast_content, strlen(broadcast_content), 0,
		//		(struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
		sendto(sock, sendString.c_str(), sendString.size(), 0,
				(struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));

#if 0

		if(heartbeat_arduino == 1)
		{
			heartbeat_arduino = 0;
			//printf("----------arduino no heartbeat--------\r\n");
		}
		
		heartbeat_cnt ++;
		if(heartbeat_cnt >= 5)
		{
			heartbeat_cnt = 0;
			heartbeat_arduino = 1;
			//write(UART_PIPE_WRITE, &err, sizeof(err));
		}
#endif
	}
}



//video mode func
int main_start_video_mode()
{

	bool flag= false;
	struct err_stat err;
	flag = iinterface->get_video_mode_running_status();
	if(flag)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_START_VIDEO_OK];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return 0;
	}

	video_mode_flag = 1;


	//video record and encode pthread
	if (0 != anyka_pthread_create(&dfrobot_video_pthread,  &video_encode_and_read_data, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, 99)) 
    {
    	printf("---------start video thread---failed\r\n");
    	video_mode_flag = 0;
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_START_VIDEO_FAILED];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
    	return -1;
    }

#if 1

	//video data transfer
	if (0 != anyka_pthread_create(&dfrobot_transfer_video_data_phtread,  &video_encode_transfer_data, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, 99)) 
    {
    	printf("---------start transfer_vide failed\r\n");
    	video_mode_flag = 0;
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_START_VIDEO_FAILED];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
    	return -1;
    }
#endif
	
	iinterface->set_video_mode_running_status(true);
	err.cmd = CMD_DFROBOT_STATUS;
	err.data = (void*)interface_err[ERR_START_VIDEO_OK];
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
	printf("----------start video thread ok\r\n");
	return 0;

}

static int auto_start_video_mode()
{
	bool flag= false;
	flag = iinterface->get_video_mode_running_status();
	if(flag)
	{
		return 0;
	}

	video_mode_flag = 1;

	if (0 != anyka_pthread_create(&dfrobot_video_pthread,  &video_encode_and_read_data, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, 99)) 
    {
    	printf("---------start video thread---failed\r\n");
    	video_mode_flag = 0;
    	return -1;
    }

	//video data transfer
	if (0 != anyka_pthread_create(&dfrobot_transfer_video_data_phtread,  &video_encode_transfer_data, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, 99)) 
    {
    	printf("---------start transfer_vide failed\r\n");
    	video_mode_flag = 0;
    	return -1;
    }

	iinterface->set_video_mode_running_status(true);
	printf("----------start video thread ok\r\n");
	return 0;
	
}



int main_stop_video_mode()
{
	struct err_stat err;
	bool flag=false;
	flag = iinterface->get_video_mode_running_status();
	if(flag == false)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_VIDEO_NO_RUNNING];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return -1;
	}
	
	video_mode_flag = 0;
	iinterface->set_video_mode_running_status(false);
	err.cmd = CMD_DFROBOT_STATUS;
	err.data = (void*)interface_err[ERR_STOP_VIDEO_OK];
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
	printf("stop video mode\r\n");
	return 0;
}


static int auto_stop_video_mode()
{
	bool flag=false;
	flag = iinterface->get_video_mode_running_status();
	if(flag == false)
	{
		return 0;
	}
	
	video_mode_flag = 0;
	iinterface->set_video_mode_running_status(false);
	printf("stop video mode\r\n");
	return 0;
}
	

//audio mode func
int main_start_audio_mode()
{
	bool flag= false;
	
	struct err_stat err;
	flag = iinterface->get_audio_mode_running_status();
	if(flag)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_START_AUDIO_OK];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return 0;
	}

	audio_mode_flag = 1;
	if(0 != anyka_pthread_create(&dfrobot_audio_record_pthread, &audio_record_data_write_queue, NULL, ANYKA_THREAD_MIN_STACK_SIZE, 85))
	{
		printf("----------start audio failed\r\n");
		audio_mode_flag = 0;
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*) interface_err[ERR_START_AUDIO_FAILED];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return -1;
	}

	iinterface->set_audio_mode_running_status(true);
	err.cmd = CMD_DFROBOT_STATUS;
	err.data = (void*)interface_err[ERR_START_AUDIO_OK];
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
	printf("---------start audio ok\r\n");
	return 0;
	
}

static int auto_start_audio_mode()
{
	bool flag= false;
	flag = iinterface->get_audio_mode_running_status();
	if(flag)
	{
		return 0;
	}

	audio_mode_flag = 1;
	if(0 != anyka_pthread_create(&dfrobot_audio_record_pthread, &audio_record_data_write_queue, NULL, ANYKA_THREAD_MIN_STACK_SIZE, 85))
	{
		printf("----------start audio failed\r\n");
		audio_mode_flag = 0;
		
		return -1;
	}

	iinterface->set_audio_mode_running_status(true);
	printf("---------start audio ok\r\n");
	return 0;
}

int main_stop_audio_mode()
{
	struct err_stat err;
	bool flag = false;
	flag = iinterface->get_audio_mode_running_status();

	if(flag == false)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_AUDIO_NO_RUNNGING];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return -1;
	}
	audio_mode_flag = 0;
	iinterface->set_audio_mode_running_status(false);
	err.cmd = CMD_DFROBOT_STATUS;
	err.data = (void*)interface_err[ERR_STOP_AUDIO_OK];
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
	printf("stop audio mode \r\n");
	return 0;
}


static int auto_stop_audio_mode()
{
	bool flag = false;
	flag = iinterface->get_audio_mode_running_status();
	
	if(flag == false)
	{
		return 0;
	}
	audio_mode_flag = 0;
	iinterface->set_audio_mode_running_status(false);
	printf("stop audio mode \r\n");
	return 0;
}

//talk to talk
int main_start_talk2talk_mode()
{
	bool flag= false;
	struct err_stat err;
	flag = iinterface->get_talk2talk_mode_running_status();
	if(flag)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_START_TALK2TALK_OK];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return 0;
	}

	//rtsp server pthread
	if(0 != anyka_pthread_create(&dfrobot_rtsp_server_pthread, &rtsp_server, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, 90))
	{
		printf("-----------start talk2talk--failed\r\n");
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_START_TALK2TALK_FAILED];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return -1;
	}
	iinterface->set_talk2talk_mode_running_status(true);
	err.cmd = CMD_DFROBOT_STATUS;
	err.data = (void*)interface_err[ERR_START_TALK2TALK_OK];
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
	printf("-----------start talk2talk ok\r\n");
	return 0;
	
}

static int auto_start_talk2talk_mode()
{
	bool flag= false;
	flag = iinterface->get_talk2talk_mode_running_status();
	if(flag)
	{
		return 0;
	}

	//rtsp server pthread
	if(0 != anyka_pthread_create(&dfrobot_rtsp_server_pthread, &rtsp_server, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, 90))
	{
		printf("-----------start talk2talk--failed\r\n");
		return -1;
	}
	iinterface->set_talk2talk_mode_running_status(true);
	printf("-----------start talk2talk ok\r\n");
	return 0;
}

int main_stop_talk2talk_mode()
{
	struct err_stat err;
	bool flag = false;
	flag = iinterface->get_talk2talk_mode_running_status();
	if(flag == false)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_TALK2TALK_NO_RUNNING];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return -1;
	}
	//close_session();
	//kill rtsp thread
	pthread_cancel(dfrobot_rtsp_server_pthread);
	iinterface->set_talk2talk_mode_running_status(false);
	err.cmd = CMD_DFROBOT_STATUS;
	err.data = (void*)interface_err[ERR_STOP_TALK2TALK_OK];
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
	printf("stop talk2tallk ok\r\n");
	return 0;
}

static int auto_stop_talk2talk_mode()
{
	
	bool flag = false;
	flag = iinterface->get_talk2talk_mode_running_status();
	if(flag == false)
	{
		return 0;
	}
	//close_session();
	//kill rtsp thread
	pthread_cancel(dfrobot_rtsp_server_pthread);
	iinterface->set_talk2talk_mode_running_status(false);
	printf("stop talk2tallk ok\r\n");
	return 0;
}


//tuling mode func
int main_start_tuling_mode()
{
	struct err_stat err;
	bool flag = false;
	flag = iinterface->get_tuling_mode_running_status();
	if(flag)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_START_TULING_OK];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return 0;
	}

	tuling_mode_flag = 1;
	if(0 != anyka_pthread_create(&dfrobot_tuling_talk_pthread, &tuling_talk, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, 90))
	{
		tuling_mode_flag = 0;
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_START_TULING_FAILED];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return -1;
		
	}

	iinterface->set_tuling_mode_running_status(true);
	err.cmd = CMD_DFROBOT_STATUS;
	err.data = (void*)interface_err[ERR_START_TULING_OK];
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
	printf("tuling start ok\r\n");
	
	return 0;
}

static int auto_start_tuling_mode()
{
	bool flag = false;
	flag = iinterface->get_tuling_mode_running_status();
	if(flag)
	{
		return 0;
	}

	tuling_mode_flag = 1;
	if(0 != anyka_pthread_create(&dfrobot_tuling_talk_pthread, &tuling_talk, NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, 90))
	{
		tuling_mode_flag = 0;
		return -1;
		
	}

	iinterface->set_tuling_mode_running_status(true);
	printf("tuling start ok\r\n");
	
	return 0;
}

int main_stop_tuling_mode()
{
	struct err_stat err;
	bool flag = false;
	flag = iinterface->get_tuling_mode_running_status();
	if(flag == false)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_TULING_NO_RUNNING];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return -1;
	}
	tuling_mode_flag = 0;
	iinterface->set_tuling_mode_running_status(false);
	err.cmd = CMD_DFROBOT_STATUS;
	err.data = (void*)interface_err[ERR_STOP_TULING_OK];
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
	printf("tuling stop ok\r\n");
	return 0;
	
}

static int auto_stop_tuling_mode()
{
	bool flag = false;
	flag = iinterface->get_tuling_mode_running_status();
	if(flag == false)
	{
		return 0;
	}
	tuling_mode_flag = 0;
	iinterface->set_tuling_mode_running_status(false);
	printf("tuling stop ok\r\n");
	return 0;
}

//aplay audio
int main_start_aplay_mode()
{
	struct err_stat err;
	bool flag = false;
	flag = iinterface->get_aplay_mode_running_status();
	if(flag)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_START_APLAY_OK];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return 0;
	}

	aplay_mode_flag = 1;

	//play audio data pthread
	if(0 != anyka_pthread_create(&dfrobot_play_audio_pthread, &aplay_mp3_data, NULL, ANYKA_THREAD_MIN_STACK_SIZE,85))
	{
		aplay_mode_flag = 0;
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_START_APLAY_FAILED];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return -1;
	}

	iinterface->set_aplay_mode_running_status(true);
	err.cmd = CMD_DFROBOT_STATUS;
	err.data = (void*)interface_err[ERR_START_APLAY_OK];
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
	printf("aplay start ok\r\n");
	return 0;
	
}

static int auto_start_aplay_mode()
{
	bool flag = false;
	flag = iinterface->get_aplay_mode_running_status();
	if(flag)
	{
		return 0;
	}

	aplay_mode_flag = 1;

	//play audio data pthread
	if(0 != anyka_pthread_create(&dfrobot_play_audio_pthread, &aplay_mp3_data, NULL, ANYKA_THREAD_MIN_STACK_SIZE,85))
	{
		aplay_mode_flag = 0;
		return -1;
	}

	iinterface->set_aplay_mode_running_status(true);
	printf("aplay start ok\r\n");
	return 0;
}

int main_stop_aplay_mode()
{
	struct err_stat err;
	bool flag = false;

	flag = iinterface->get_aplay_mode_running_status();
	if(!flag)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_APLAY_NO_RUNNING];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return -1;
	}
	
	aplay_mode_flag = 0;
	iinterface->set_aplay_mode_running_status(false);
	
	err.cmd = CMD_DFROBOT_STATUS;
	err.data = (void*)interface_err[ERR_STOP_APLAY_OK];
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
	printf("aplay stop aplay\r\n");
	return 0;
}

static int auto_stop_aplay_mode()
{
	bool flag = false;

	flag = iinterface->get_aplay_mode_running_status();
	if(!flag)
	{
		return 0;
	}
	
	aplay_mode_flag = 0;
	iinterface->set_aplay_mode_running_status(false);
	printf("aplay stop aplay\r\n");
	return 0;
}


void main_start_wifi()
{

}

void main_restart()
{

}

void main_restartWifi()
{

}

void main_stop_wifi()
{

}
void main_set_config_name(const char *dst)
{
	int sd_exist = 0;
	char file[100] = {0};

	if(global_config->name != NULL)
		free(global_config->name);

	global_config->name = (char*) malloc(strlen(dst)+1);
	if(global_config->name == NULL)
	{
		printf("tmp malloc failed\r\n");
		return;
	}

	memset(global_config->name, 0, strlen(dst)+1);
	memcpy(global_config->name, dst, strlen(dst));

	sd_exist = sd_get_status();
	
	if(sd_exist)
	{
		sprintf(file, "%s", DFROBOT_CONFIG_SD_FILE);
	}
	else
	{
		sprintf(file, "%s", DFROBOT_CONFIG_FILE);
	}

	
	write_element("name", global_config->name, file);

}

//set cmd
void main_arduino_set_ssid(const char *ssid)
{
	struct err_stat err;
	int sd_exist = 0;
	char file[100] = {0};
	int ret = 0;
	if(global_config == NULL)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_INIT_CONFIG_FAILED];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return ;
	}

	if(global_config->ssid != NULL)
		free(global_config->ssid);

	global_config->ssid = (char*) malloc(strlen(ssid)+1);
	if(global_config->ssid == NULL)
	{
		printf("tmp malloc failed\r\n");
		return;
	}
	
	memset(global_config->ssid, 0, strlen(ssid)+1);
	memcpy(global_config->ssid, ssid, strlen(ssid));
	
	sd_exist = sd_get_status();
	if(sd_exist)
	{
		sprintf(file, "%s", DFROBOT_CONFIG_SD_FILE);
	}
	else
	{
		sprintf(file, "%s", DFROBOT_CONFIG_FILE);
	}

	ret = write_element("ssid", global_config->ssid, file);

	err.cmd = CMD_DFROBOT_STATUS;
	if(ret != 0)
	{
		err.data = (void*)interface_err[ERR_SSID_SET_FAILED];
	}
	else
	{
		err.data = (void*)interface_err[ERR_SSID_SET_OK];
	}
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
}

void main_arduino_set_password(const char *passwrd)
{
	struct err_stat err;
	int sd_exist = 0;
	char file[100] = {0};
	int ret = 0;
	if(global_config == NULL)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_INIT_CONFIG_FAILED];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return ;
	}

	if(global_config->password != NULL)
		free(global_config->password);

	global_config->password = (char*) malloc(strlen(passwrd)+1);
	if(global_config->password== NULL)
	{
		printf("tmp malloc failed\r\n");
		return;
	}
	
	memset(global_config->password, 0, strlen(passwrd)+1);
	memcpy(global_config->password, passwrd, strlen(passwrd));
	
	sd_exist = sd_get_status();
	if(sd_exist)
	{
		sprintf(file, "%s", DFROBOT_CONFIG_SD_FILE);
	}
	else
	{
		sprintf(file, "%s", DFROBOT_CONFIG_FILE);
	}

	ret = write_element("password", global_config->password, file);

	err.cmd = CMD_DFROBOT_STATUS;
	if(ret != 0)
	{
		err.data = (void*)interface_err[ERR_PASSWORD_SET_FAILED];
	}
	else
	{
		err.data = (void*)interface_err[ERR_PASSWORD_SET_OK];
	}
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
}

void switch_resolution(int w, int h)
{
	do
	{
		{
			iinterface->stop_video_mode();
			usleep(300*1000);
			video_encode_close(encode_handle.encode_handle);
			camera_close();
		}

		usleep(500*1000);

		{
			camera_open(w, h);
			camera_ioctl(w, h);
			encode_info->width = w;
			encode_info->height = h;
			encode_info->lumWidthSrc = w;
			encode_info->lumHeightSrc = h;

			encode_handle.encode_handle = video_encode_open(encode_info);
			usleep(100*1000);
			iinterface->start_video_mode();
			
		}

	}while(0);
}

void main_arduino_set_resolution(unsigned char res)
{
	int w = 0, h = 0;

	bool flag = false;
	struct err_stat err;
	flag = iinterface->get_video_mode_running_status();
	if(!flag)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_VIDEO_NO_RUNNING];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return ;
	}

	if(current_resolution == res)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_RESOLUTION_SET_OK];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return;
	}

	switch(res)
	{
		case R1280X720:
		{
			current_resolution = R1280X720;
			w = 1280;
			h = 720;
			break;
		}
		case R640X480:
		{
			current_resolution = R640X480;
			w = 640;
			h = 480;
			break;
		}
		case R320X240:
		{
			//no support
			break;
		}
		default:
			break;
	}

	if((w == 0) || (h == 0))
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_RESOLUTION_SET_FAILED];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return ;
	}
#if 0
	do
	{
		{
			iinterface->stop_video_mode();
			usleep(300*1000);
			video_encode_close(encode_handle.encode_handle);
			camera_close();
		}

		usleep(500*1000);

		{
			camera_open(w, h);
			camera_ioctl(w, h);
			encode_info->width = w;
			encode_info->height = h;
			encode_info->lumWidthSrc = w;
			encode_info->lumHeightSrc = h;

			encode_handle.encode_handle = video_encode_open(encode_info);
			usleep(100*1000);
			iinterface->start_video_mode();
			
		}

	}while(0);
#else
	switch_resolution(w,h);

#endif

	err.cmd = CMD_DFROBOT_STATUS;
	err.data = (void*)interface_err[ERR_RESOLUTION_SET_OK];
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
	
}

void main_arduino_set_voice(unsigned char voice)
{
	struct err_stat err;
	if(voice > 5)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_VOICE_SET_FAILED];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return ;
	}
	
	voice_size = voice;
	err.cmd = CMD_DFROBOT_STATUS;
	err.data = (void*)interface_err[ERR_VOICE_SET_OK];
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
}

void main_arduino_set_name(const char *name)
{
	struct err_stat err;
	int sd_exist = 0;
	char file[100] = {0};
	int ret = 0;
	if(global_config == NULL)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_INIT_CONFIG_FAILED];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return ;
	}

	if(global_config->name != NULL)
		free(global_config->name);

	global_config->name = (char*) malloc(strlen(name)+1);
	if(global_config->name == NULL)
	{
		printf("tmp malloc failed\r\n");
		return;
	}
	
	memset(global_config->name, 0, strlen(name)+1);
	memcpy(global_config->name, name, strlen(name));
	
	sd_exist = sd_get_status();
	if(sd_exist)
	{
		sprintf(file, "%s", DFROBOT_CONFIG_SD_FILE);
	}
	else
	{
		sprintf(file, "%s", DFROBOT_CONFIG_FILE);
	}

	ret = write_element("name", global_config->name, file);

	err.cmd = CMD_DFROBOT_STATUS;
	if(ret != 0)
	{
		err.data = (void*)interface_err[ERR_NAME_SET_FAILED];
	}
	else
	{
		err.data = (void*)interface_err[ERR_NAME_SET_OK];
	}
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
	

}

void main_arduino_set_frame(unsigned char frame)
{
	int res = 0;
	bool flag= false;
	struct err_stat err;
	flag = iinterface->get_video_mode_running_status();
	if(!flag)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_VIDEO_NO_RUNNING];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return ;
	}
	
	res = camera_set_fps(frame);
	
	err.cmd = CMD_DFROBOT_STATUS;
	if(res == -1)
	{
		err.data = (void*)interface_err[ERR_FRAME_SET_FAILED];
	}
	else
	{
		err.data = (void*)interface_err[ERR_FRAME_SET_OK];
	}
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
}

//get cmd
void main_arduino_get_ssid()
{
	struct err_stat err;
	if(global_config == NULL)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_INIT_CONFIG_FAILED];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return ;
	}
	err.cmd = CMD_HANDLE_GET_SSID;
	err.data = (void*)global_config->ssid;
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
}

void main_arduino_get_password()
{
	struct err_stat err;
	if(global_config == NULL)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_INIT_CONFIG_FAILED];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return ;
	}
	err.cmd = CMD_HANDLE_GET_PASSWORD;
	err.data = (void*)global_config->password;
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
}

void main_arduino_get_resolution()
{
	bool flag= false;
	struct err_stat err;
	flag = iinterface->get_video_mode_running_status();
	if(!flag)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_VIDEO_NO_RUNNING];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return ;
	}
	
	err.cmd = CMD_HANDLE_GET_RESOLUTION;
	err.data = (void*)(&current_resolution);
	err.len = 1;
	write(UART_PIPE_WRITE, &err, sizeof(err));
}

void main_arduino_get_frame()
{
	static int frame = 0; 
	bool flag= false;
	struct err_stat err;
	flag = iinterface->get_video_mode_running_status();
	if(!flag)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_VIDEO_NO_RUNNING];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return ;
	}
	
	frame = camera_get_fps();
	err.cmd = CMD_HANDLE_GET_FRAME;
	err.data = (void*)(&frame);
	err.len = 1;
	write(UART_PIPE_WRITE, &err, sizeof(err));
}

void main_arduino_get_name()
{
	struct err_stat err;
	if(global_config == NULL)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_INIT_CONFIG_FAILED];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return ;
	}
	err.cmd = CMD_HANDLE_GET_NAME;
	err.data = (void*)global_config->name;
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));
}

void main_arduino_get_voice()
{
	struct err_stat err;
	err.cmd = CMD_HANDLE_GET_VOICE;
	err.data = (void*)(&voice_size);
	err.len = 1;
	write(UART_PIPE_WRITE, &err, sizeof(err));
}

//other cmd
void main_arduino_save_photo()
{
	bool flag= false;
	struct err_stat err;
	int sd_exist = 0;
	flag = iinterface->get_video_mode_running_status();
	if(!flag)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_VIDEO_NO_RUNNING];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return ;
	}

	sd_exist = sd_get_status();
	if(!sd_exist)
	{
		err.cmd = CMD_DFROBOT_STATUS;
		err.data = (void*)interface_err[ERR_SD_NO_EXIST];
		err.len = strlen((char*)err.data);
		write(UART_PIPE_WRITE, &err, sizeof(err));
		return ;
	}
	
	take_photo = 1;
	err.cmd = CMD_DFROBOT_STATUS;
	err.data = (void*)interface_err[ERR_TAKE_PHOTO_OK];
	err.len = strlen((char*)err.data);
	write(UART_PIPE_WRITE, &err, sizeof(err));

}

void main_arduino_heartbeat_packet()
{
	heartbeat_arduino = 0;
}




void main_start_all_mode()
{
#if 0
	iinterface->start_video_mode();
	usleep(300*1000);
	iinterface->start_audio_mode();
	usleep(300*1000);
	iinterface->start_talk2talk_mode();
	usleep(300*1000);
	if(!strcmp("chinese", global_config->language))
	{
		iinterface->start_tuling_mode();
	}
	else if(!strcmp("english", global_config->language))
	{
		//start english chat
	}
	usleep(300*1000);
	iinterface->start_aplay_mode();
#else

	int video_flag = 0;
	int audio_flag = 0;
	int talk2talk_flag = 0;
	int chat_flag = 0;
	int aplay_flag = 0;
	const char *video_err = "video start error;";
	const char *audio_err = "audio start error;";
	const char *talk2talk_err = "talk2talk start error;";
	const char *chat_err = "chat start error;";
	const char *aplay_err = "aplay start error;";
	const char *auto_ok = "all mode auto ok";

	static char buf[100];

	struct err_stat err;
	int len = 0;
	char *tmp = buf;
	memset(buf, 0 ,100);

	video_flag = auto_start_video_mode();
	usleep(300*1000);
	audio_flag = auto_start_audio_mode();
	usleep(300*1000);
	talk2talk_flag = auto_start_talk2talk_mode();
	usleep(300*1000);
	if(!strcmp("chinese", global_config->language))
	{
		chat_flag = auto_start_tuling_mode();
	}
	else if(!strcmp("english", global_config->language))
	{
		//start english chat
	}
	usleep(300*1000);
	aplay_flag = auto_start_aplay_mode();
		
	if(video_flag != 0)
	{
		tmp = &buf[len];
		len += strlen(video_err);
		memcpy(tmp, video_err, strlen(video_err));
	}

	if(audio_flag != 0)
	{
		tmp = &buf[len];
		len += strlen(audio_err);
		memcpy(tmp, audio_err, strlen(audio_err));
	}

	if(talk2talk_flag != 0)
	{
		tmp = &buf[len];
		len += strlen(talk2talk_err);
		memcpy(tmp, talk2talk_err, strlen(talk2talk_err));
	}

	if(chat_flag != 0)
	{
		tmp = &buf[len];
		len += strlen(chat_err);
		memcpy(tmp, chat_err, strlen(chat_err));
	}

	if(aplay_flag != 0)
	{
		tmp = &buf[len];
		len += strlen(aplay_err);
		memcpy(tmp, aplay_err, strlen(aplay_err));
	}

	if(len == 0)
	{
		tmp = &buf[len];
		len += strlen(auto_ok);
		memcpy(tmp, auto_ok, strlen(auto_ok));
	}

	err.cmd = CMD_DFROBOT_STATUS;
	err.data = (void*)buf;
	err.len = len;
	write(UART_PIPE_WRITE, &err, sizeof(err));
#endif

}
void main_stop_all_mode()
{
#if 0
	iinterface->stop_video_mode();
	usleep(100*1000);
	iinterface->stop_audio_mode();
	usleep(100*1000);
	iinterface->stop_talk2talk_mode();
	usleep(100*1000);
	
	if(!strcmp("chinese", global_config->language))
	{
		iinterface->stop_tuling_mode();
	}
	else if(!strcmp("english", global_config->language))
	{
		//stop english chat
	}
	usleep(100*1000);
	iinterface->stop_aplay_mode();
#else

	struct err_stat err;
	static const char *all_stop_ok = "all mode stop ok";
	auto_stop_video_mode();
	usleep(100*1000);
	auto_stop_audio_mode();
	usleep(100*1000);
	auto_stop_talk2talk_mode();
	usleep(100*1000);
	if(!strcmp("chinese", global_config->language))
	{
		auto_stop_tuling_mode();
	}
	else if(!strcmp("english", global_config->language))
	{
		//stop english chat
	}
	usleep(100*1000);
	auto_stop_aplay_mode();
	
	err.cmd = CMD_DFROBOT_STATUS;
	err.data = (void*)all_stop_ok;
	err.len = strlen(all_stop_ok);
	write(UART_PIPE_WRITE, &err, sizeof(err));

#endif
}




/*
** package  header   |   addr   |    data len  |    cmd  |    data   | crc
**		    2 byte        1byte          1byte          1byte      ...	     1byte
**
*/
unsigned char *uart_read_data(long fd, unsigned char addr)
{
	//read header
	unsigned char header[2];
#if 0
	if((read(fd, header, 2) != 2) 
		|| (header[0] != 0x55) || (header[1] != 0xaa)) 
	{
		printf("header:%x,%x\r\n", header[0], header[1]);
		return NULL;
	}
#else
	if((read(fd, &header[0], 1) != 1) || header[0] != 0x55 ) 
	{
		printf("header[0]:%x\r\n",header[0]);
		return NULL;
	}
	//printf("header[0]:%x\r\n",header[0]);
	if((read(fd, &header[1], 1) != 1) || header[1] != 0xaa)
	{
		printf("header[1]:%x\r\n",header[1]);
		return NULL;
	}
	//printf("header[1]:%x\r\n",header[1]);
	

#endif

	//read dev addr
	unsigned char dev_addr;
	if((read(fd, &dev_addr, 1) != 1) 
			|| (dev_addr != addr)) 
	{
		printf("addr:%x\r\n", dev_addr);
		//return NULL;
	}
	//printf("addr:%x\r\n", dev_addr);

	//read data size
	unsigned char size;
	if(read(fd, &size, 1) != 1) return NULL;

	//printf("size=%d\r\n",size);

	unsigned char *data = (unsigned char *)malloc(size+6);
	if(data == NULL)
	{
		printf(" data malloc error\r\n");
		return NULL;
	}
	data[0] = header[0];
	data[1] = header[1];
	data[2] = dev_addr;
	data[3] = size;
	
	int count = 0;
	int i=0;
	int res=0;
	while((count < (size+2)) && (i<5))
	{
		res = read(fd, &data[count+4], (size + 2 - count));
		count += res;
		if(res == 0)
		{
			i++;
		}
	}
	
	if(count != (size+2))
	{
		free(data);
		return 	NULL;
	}

	

	return  data;
	
}


int main()
{
	struct termios  oldtio;
	int i;
	int maxfd = -1;
	fd_set  allset;
	fd_set  fdset;

	if(CheckRunOnly() < 0)
	{
		printf("!!!!dfrobot_test is running\n");
		return -1;
	}


	sig_init();

	Condition_Initialize( &g_conMetex );
	Condition_Initialize(&g_aplayMetex);
	Condition_Initialize(&g_tulingMetex);
	
	jpeg_buf = new uint8_t[MAIN_BUF_SIZE];

	if(jpeg_buf == NULL)
	{
		printf("jpeg_buf malloc failed\r\n");
		goto out;
	}
	audio_buf = new unsigned char[MAIN_BUF_SIZE];

	if(audio_buf == NULL)
	{
		printf("audio buf failed\r\n");
		goto out;
	}

	tuling_data = new uint8_t[MAIN_BUF_SIZE];
	if(tuling_data == NULL)
	{
		printf("tuling_data new error\r\n");
		goto out;
	}

	
	save_audio_data = cycle_buff_init();
	
	if(save_audio_data == NULL)
	{
		printf("cycle_buff_init error\r\n");
		goto out;
	}
	g_audio_list = list_header_init();
	if(g_audio_list == NULL)
	{
		printf("list init error\r\n");
		goto out;
	}

	sd_init_status();

	global_config = read_config();



	//初始化硬件
	PTZControlInit();
	//init dma memory  //以后可能需要添加语音支持
	akuio_pmem_init();



	current_resolution = R1280X720;
	if (0 != camera_open(1280, 720))
	{
		printf("camera open failed\r\n");
		goto out;
	}
	
	camera_ioctl(1280, 720);
	printf("came open ok!\r\n");




	//初始化软件
	encode_info = (T_ENC_INPUT *)malloc(sizeof(T_ENC_INPUT));
	
	encode_handle.video_size_flag = 1;//720p
	encode_handle.frames = 30;//camera_get_max_frame();
	encode_handle.index = 0;
	encode_handle.encode_use_flag = 0;
	
	settings_init(encode_info);
	
	video_encode_init();
	
	encode_handle.encode_handle = video_encode_open(encode_info);
	usleep(200*1000);


	//broadcast ip
	broadcast_flag = 1;
	if(0 != anyka_pthread_create(&dfrobot_broadcast_pthread,  &broadcast_ip, NULL, ANYKA_THREAD_MIN_STACK_SIZE, 85))
	{
		
	}
	

	iinterface = new Interface();
	if(iinterface == NULL)
	{
		printf("iinterface new error\r\n");
		goto out;
	}
	

	if(pipe(uart_pipe) < 0)
	{
		printf("uart pipe error\r\n");
		goto out;
	}


	myServer = TransferServer::Create();
  	myServer->start();
	rtsp_init();

	

#if 0

	//iinterface->start_video_mode();
	//iinterface->start_audio_mode();
	//iinterface->start_talk2talk_mode();
	iinterface->start_all_mode();
	
	while(1)
	{
	#if 0  //video test
		iinterface->start_video_mode();
		//pause();
		sleep(10);
		iinterface->stop_video_mode();
		sleep(3);
	#endif

	#if 0  //audio test
		iinterface->start_audio_mode();
		sleep(10);
		iinterface->stop_audio_mode();
		sleep(2);
	#endif

	#if 0
		iinterface->start_talk2talk_mode();
		sleep(5);
		iinterface->stop_talk2talk_mode();
		sleep(2);

	#endif

	#if 0

		iinterface->start_all_mode();

		sleep(10);

		iinterface->stop_all_mode();
		sleep(5);

	#endif

	#if 0
		iinterface->start_video_mode();
		sleep(1);
		iinterface->start_audio_mode();
		sleep(1);
		iinterface->start_talk2talk_mode();
		sleep(1);
		iinterface->start_tuling_mode();
		sleep(1);
		iinterface->start_aplay_mode();
		sleep(10);

		iinterface->stop_video_mode();
		iinterface->stop_audio_mode();
		iinterface->stop_talk2talk_mode();
		iinterface->stop_tuling_mode();
		iinterface->stop_aplay_mode();
		sleep(1);
		

	#endif

	#if 0 //test set resolution
		sleep(20);
		{
			iinterface->stop_video_mode();
			usleep(300*1000);
			video_encode_close(encode_handle.encode_handle);
			camera_close();
		}

		usleep(500*1000);

		{
			camera_open(640, 480);
			camera_ioctl(640, 480);
			encode_info->width = 640;
			encode_info->height = 480;
			encode_info->lumWidthSrc = 640;
			encode_info->lumHeightSrc = 480;

			encode_handle.encode_handle = video_encode_open(encode_info);
			usleep(100*1000);
			iinterface->start_video_mode();
			
		}

		sleep(20);

		{
			iinterface->stop_video_mode();
			usleep(300*1000);
			video_encode_close(encode_handle.encode_handle);
			camera_close();

		}

		usleep(500*1000);
		{
			camera_open(1280, 720);
			camera_ioctl(1280, 720);
			encode_info->width = 1280;
			encode_info->height = 720;
			encode_info->lumWidthSrc = 1280;
			encode_info->lumHeightSrc = 720;

			encode_handle.encode_handle = video_encode_open(encode_info);
			usleep(100*1000);
			iinterface->start_video_mode();
			
		}
		

	#endif


	#if 0
		TULING_SET_PARAM test;
		test.baidu_api_key = "11";
		test.baidu_api_sercret = "22";
		test.baidu_user_id = "33";
		test.tuling_api_key = "44";
		test.tuling_user_id = "55";
		
		set_tuling_param(&test);
	sleep(5);
	#endif

		pause();
		sleep(10);
	}

#else
	uart_fd = open(TTY_COMMUNICATE_ARDUINO, O_RDWR | O_NOCTTY );
	if(uart_fd < 0 )
	{
		printf("uart  open error\r\n");
		goto out;
	}
	
	if(tcgetattr(uart_fd, &oldtio) != 0)
	{
		printf("tcgettattr failed \r\n");
		return -1;
	}

	oldtio.c_cflag |= CLOCAL | CREAD;
	oldtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	oldtio.c_oflag &= ~OPOST;
	oldtio.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

	cfsetispeed(&oldtio, B115200);

	if(tcsetattr(uart_fd, TCSANOW, &oldtio) != 0)
	{
		printf("tcsetattr failed \r\n");
		goto out;
	}

	printf("set uart ok\r\n");

	main_func_run_flag = 1;
	
	FD_ZERO(&allset);
	FD_SET(uart_fd, &allset);
	FD_SET(UART_PIPE_READ, &allset);
	maxfd = DFROBOT_MAX(uart_fd, UART_PIPE_READ);


	while(main_func_run_flag)
	{
		fdset = allset;
		if (select(maxfd+1, &fdset, NULL, NULL, NULL) > 0)
		{
			if(FD_ISSET(uart_fd, &fdset))
			{
				unsigned char *data = NULL;

		#if 0 //test 
			//printf("--------\r\n");
			int iii = 0;
			char buf2[100] = {0};
			iii = read(uart_fd, buf2, 100);
			//printf("recv:%s\r\n",buf2);
			for(i=0; i<iii; i++)
			{
				printf("%x\t",buf2[i]);
			}
			printf("\n");
			continue;
	
		#endif

				data = uart_read_data(uart_fd, startCamera_addr);

				
				if(CheckCrc2(data) == false)
				{
					if(data != NULL) 
					{
						free(data);
						data = NULL;
					}
					continue;
				}

				printf("cmd:%x\r\n",data[4]);

			#if 0 //test
				free(data);
				continue;
			#endif

				switch(data[4])
				{
					case CMD_START_MODE:
					{
					
						char *tmp = (char*)malloc(data[3]+1);
						if(tmp == NULL)
						{
							printf("tmp malloc error\r\n");
							break;
						}

						memset(tmp, 0, (data[3]+1));
						memcpy(tmp, &data[5], data[3]);
						printf("data:%s\r\n",tmp);
						

						if(strcmp("auto", tmp) == 0)
						{
							#if 0  // only test 
							unsigned char test[] = "all mode ok";
							unsigned char *tmp_test = NULL;
							int test_len;
							tmp_test = package_data(CMD_DFROBOT_STATUS, startCamera_addr, test, 11, &test_len);
							write(uart_fd, tmp_test, test_len);
							int k=0;
							for(k=0; k<test_len; k++)
							{
								printf("%x\r\n",tmp_test[k]);
							}
							free(tmp_test);
							tmp_test = NULL;
							#endif
							iinterface->start_all_mode();
						}
						else if(strcmp("video", tmp) == 0)
						{
							iinterface->start_video_mode();
						}
						else if(strcmp("audio", tmp) == 0)
						{
							iinterface->start_audio_mode();
						}
						else if(strcmp("tuling", tmp) == 0)
						{
							iinterface->start_tuling_mode();
						}
						else if(strcmp("talk2talk", tmp) == 0)
						{
							iinterface->start_talk2talk_mode();
						}
						else if(strcmp("aplay", tmp) == 0)
						{
							iinterface->start_aplay_mode();
						}
						else if(strcmp("wifi", tmp) == 0)
						{
							iinterface->start_wifi();
						}
						else if(strcmp("restart", tmp) == 0)
						{
							iinterface->restart();
						}
						else if(strcmp("restartWifi",tmp) == 0)
						{
							iinterface->restartWifi();
						}

						free(tmp);
						break;
					}

					case CMD_STOP_MODE:
					{
						char *tmp = (char*)malloc(data[3]+1);
						if(tmp == NULL)
						{
							printf("tmp malloc error\r\n");
							break;
						}

						memset(tmp, 0, (data[3]+1));
						memcpy(tmp, &data[5], data[3]);
						printf("data:%s\r\n",tmp);

						if(strcmp("auto", tmp) == 0)
						{
							iinterface->stop_all_mode();
						}
						else if(strcmp("video", tmp) == 0)
						{
							iinterface->stop_video_mode();
						}
						else if(strcmp("audio", tmp) == 0)
						{
							iinterface->stop_audio_mode();
						}
						else if(strcmp("tuling", tmp) == 0)
						{
							iinterface->stop_tuling_mode();
						}
						else if(strcmp("talk2talk", tmp) == 0)
						{
							iinterface->stop_talk2talk_mode();
						}
						else if(strcmp("aplay", tmp) == 0)
						{
							iinterface->stop_aplay_mode();
						}
						else if(strcmp("wifi", tmp) == 0)
						{
							iinterface->stop_wifi();
						}
						

						free(tmp);
						break;
					}
					//set cmd
					case CMD_SET_SSID:
					{
						char *tmp = (char*)malloc(data[3]+1);
						if(tmp == NULL)
						{
							printf("tmp malloc error\r\n");
							break;
						}

						memset(tmp, 0, (data[3]+1));
						memcpy(tmp, &data[5], data[3]);
						printf("data:%s\r\n",tmp);

						iinterface->arduino_set_ssid(tmp);
						free(tmp);

						break;
					}
					case CMD_SET_PASSWORD:
					{
						char *tmp = (char*)malloc(data[3]+1);
						if(tmp == NULL)
						{
							printf("tmp malloc error\r\n");
							break;
						}

						memset(tmp, 0, (data[3]+1));
						memcpy(tmp, &data[5], data[3]);
						printf("data:%s\r\n",tmp);
						iinterface->arduino_set_password(tmp);
						free(tmp);

						break;
					}
					case CMD_SET_RESOLUTION:
					{
						iinterface->arduino_set_resolution(data[5]);
						break;
					}
					case CMD_SET_VOICE:
					{
						iinterface->arduino_set_voice(data[5]);
						break;
					}
					case CMD_SET_NAME:
					{
						char *tmp = (char*)malloc(data[3]+1);
						if(tmp == NULL)
						{
							printf("tmp malloc error\r\n");
							break;
						}

						memset(tmp, 0, (data[3]+1));
						memcpy(tmp, &data[5], data[3]);
						printf("data:%s\r\n",tmp);
						iinterface->arduino_set_name(tmp);
						free(tmp);
						break;
					}
					case CMD_SET_FRAME:
					{
						iinterface->arduino_set_frame(data[5]);
						break;
					}
					//get cmd
	#if 0
					case  CMD_GET_PARAM:
					{
						char *tmp = (char*)malloc(data[3]+1);
						if(tmp == NULL)
						{
							printf("tmp malloc error\r\n");
							break;
						}

						memset(tmp, 0, (data[3]+1));
						memcpy(tmp, &data[5], data[3]);
						printf("data:%s\r\n",tmp);

						if(strcmp("ssid", tmp) == 0)
						{
							iinterface->arduino_get_ssid();
						}
						else if(strcmp("password", tmp) == 0)
						{
							iinterface->arduino_get_password();
						}
						else if(strcmp("resolution", tmp) == 0)
						{
							iinterface->arduino_get_resolution();
						}
						else if(strcmp("frame", tmp) == 0)
						{
							iinterface->arduino_get_frame();
						}
						else if(strcmp("name", tmp) == 0)
						{
							iinterface->arduino_get_name();
						}
						else if(strcmp("voice", tmp) == 0)
						{
							iinterface->arduino_get_voice();
						}
						
						free(tmp);
						break;
					}
#endif
					case CMD_GET_SSID:
					{
						iinterface->arduino_get_ssid();
						break;
					}
					case CMD_GET_PASSWORD:
					{
						iinterface->arduino_get_password();
						break;
					}
					case CMD_GET_RESOLUTION:
					{
						iinterface->arduino_get_resolution();
						break;
					}
					case CMD_GET_FRAME:
					{
						iinterface->arduino_get_frame();
						break;
					}
					case CMD_GET_NAME:
					{
						iinterface->arduino_get_name();
						break;
					}
					case CMD_GET_VOICE:
					{
						iinterface->arduino_get_voice();
						break;
					}
					//other
					case CMD_SAVE_FRAME:
					{
						iinterface->arduino_save_photo();
						break;
					}
					//heartbeat packet
					case CMD_HEARTBEAT_PACKET:
					{
						//iinterface->arduino_heartbeat_packet();
						struct err_stat err;

						err.cmd = RESPOND_HRARTBREAT_PACKET;
						err.data = NULL;
						err.len = 0;
						write(UART_PIPE_WRITE, &err, sizeof(err));
						
						break;
					}

					default:
						break;
				}

				
				free(data);
			}
			else if(FD_ISSET(UART_PIPE_READ, &fdset))
			{
				struct err_stat msg_err;
				unsigned char *tmp_send = NULL;
				int len_send = 0;

				read(UART_PIPE_READ, &msg_err, sizeof(msg_err));

				switch(msg_err.cmd)
				{
					case CMD_DFROBOT_STATUS:
					//handle get
					case CMD_HANDLE_GET_NAME:
					case CMD_HANDLE_GET_RESOLUTION:
					case CMD_HANDLE_GET_FRAME:
					case CMD_HANDLE_GET_VOICE:
					case CMD_HANDLE_GET_SSID:
					case CMD_HANDLE_GET_PASSWORD:
					//heartbeat packet
					case CMD_HEARTBEAT_PACKET:
					{
						tmp_send = package_data(msg_err.cmd, startCamera_addr, 
							(unsigned char*)msg_err.data, msg_err.len, &len_send);
						break;
					}
					
					default:
						break;
						
				}

				if(tmp_send != NULL)
				{
					write(uart_fd, tmp_send, len_send);
					free(tmp_send);
					tmp_send = NULL;
				}
			}
		}
		
		
	}
#endif

	
out:
	
	Condition_Destroy( &g_conMetex );
	Condition_Destroy(&g_aplayMetex);
	Condition_Destroy(&g_tulingMetex);
	if(myServer != NULL)
		delete myServer;
	
	if(jpeg_buf != NULL)
	{
		delete []jpeg_buf;
		jpeg_buf = NULL;
	}
	
	if(audio_buf != NULL)
	{
		delete []audio_buf;
		audio_buf = NULL;
	}
	
	if(save_audio_data != NULL)
	{
		cycle_buff_destroy(save_audio_data);
		save_audio_data = NULL;
	}
	
	if(g_audio_list != NULL)
	{
		list_header_destroy(g_audio_list);
		g_audio_list = NULL;
	}

	if(tuling_data != NULL)
	{
		delete []tuling_data;
		tuling_data = NULL;
	}

	
	if(iinterface != NULL)
	{
		delete iinterface;
	}
	if(uart_pipe[0] >= 0)
	{
		close(uart_pipe[0]);
	}
	if(uart_pipe[1] >= 0)
	{
		close(uart_pipe[1]);
	}

	if(uart_fd >= 0)
	{
		close(uart_fd);
	}


	return 0;
}



#endif




static void settings_init(T_ENC_INPUT *config)
{
	memset( config, 0, sizeof(T_ENC_INPUT));
	config->width = 1280;
	config->height = 720;
	config->lumWidthSrc = 1280;
	config->lumHeightSrc = 720;
	config->qpHdr = 0;  //do not care
	config->iqpHdr = 0;  //do not care
	config->minQp = 20;
	config->maxQp = 36;
	config->framePerSecond = 10;
	config->gopLen = 20;
	config->bitPerSecond = 256;
	config->encode_type = MJPEG_ENC_TYPE;
	config->video_mode = 0;
}


                  
