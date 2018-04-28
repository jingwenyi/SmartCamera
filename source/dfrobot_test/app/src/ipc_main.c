#include <signal.h>
#include <execinfo.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include "common.h"
#include "dana_interface.h"
#include "onvif_init.h"
#include "tencent_init.h"
#include "akuio.h"
#include "voice_tips.h"
#include "anyka_monitor.h"
#include "debug.h"
#include <ctype.h>
#include <math.h>

#ifdef CONFIG_RTSP_SUPPORT
#include "ak_rtsp.h"
#endif

#define AK_VERSION_SOFTWARE             "V1.0.13" 

#define SYS_START 						"/usr/share/anyka_camera_start.mp3"		//系统启动提示音
#define SYS_START_ONE_TIMES_FLAG		"/tmp/ak_sys_start_flag"
#define SD_STATUS						"/tmp/sd_status"
#define VOICE_WORD						"/tmp/ipc_rec/"


unsigned int ak_version_manage()
{
	int i = 0, j = 0;
	char src[100] = {0};
	unsigned int dest = 0, tmp;
	
	strcpy(src, AK_VERSION_SOFTWARE);
	while(src[i] != '\0') {
		if(isdigit(src[i])) {
			tmp = src[i] - 48;
			dest += (tmp * ((unsigned int)pow(10,(3 - j))));
			j ++;
		}
		i ++;
	}

	return dest;
}

/**
* @brief 	 ipc_exit_app
* 			app退出时的系统处理
* @date 	2015/3
* @param:	void
* @return 	void
* @retval 	
*/

void ipc_exit_app()
{
    camera_off();
    camera_close();
    PTZControlDeinit();
}


/**
* @brief 	 anyka_init_hardware
* 			硬件初始化
* @date 	2015/3
* @param:	void
* @return 	void
* @retval 	
*/

/********************************************
此处将初始化所有硬件相关的设备
包括马达,CAMERA,音频之类的
*********************************************/
void anyka_init_hardware(void)
{
    PTZControlInit();
	//init dma memory
	akuio_pmem_init();
	sd_init_status();
	//open camera	
	if (0 == camera_open())
		camera_ioctl(VIDEO_WIDTH_720P, VIDEO_HEIGHT_720P);

	pthread_t ircut_pth_id;
	anyka_pthread_create(&ircut_pth_id, (anyka_thread_main *)camera_ircut_pthread,
					(void *)NULL, ANYKA_THREAD_MIN_STACK_SIZE, 1);

#if 1
	pthread_t switch_fps_pth_id;
	anyka_pthread_create(&switch_fps_pth_id, (anyka_thread_main *)camera_change_fps_pthread,
					(void *)NULL, ANYKA_THREAD_MIN_STACK_SIZE, 1);
#endif
}

/**
* @brief 	 anyka_init_software
* 			软件初始化
* @date 	2015/3
* @param:	void
* @return 	void
* @retval 	
*/

/*************************************************
此处做一些必要的初始化软件设置,
目前设备解码库相关的初始化放在相应的消息里面.
**************************************************/
void anyka_init_software(void)
{
	T_AUDIO_INPUT audioInput;

    video_init();
	audioInput.nBitsPerSample = 16;
	audioInput.nChannels = 1;
	audioInput.nSampleRate = 8000;
    audio_start(&audioInput);
    anyka_fs_init();
    anyka_photo_init("/mnt/pic/");
}


/**
* @brief 	anyka_get_verion
* 			获取软件版本
* @date 	2015/3
* @param:	void
* @return 	char *
* @retval 	指向当前软件版本字符串指针
*/

char* anyka_get_verion(void)
{
    return AK_VERSION_SOFTWARE;
}


/**
* @brief 	sigprocess
* 			自定义用户信号处理，接收到异常信号后进入此接口
* @date 	2015/3
* @param:	int sig，信号值
* @return 	void
* @retval 	
*/


#if 1
static void sigprocess(int sig)
{
	// Fix me ! 20140828 ! Bug Bug Bug Bug Bug
#if 1
	anyka_print("**************************\n");
	anyka_print("\n##signal %d caught\n", sig);
	anyka_print("**************************\n");
	int ii = 0;
	void *tracePtrs[16];
	int count = backtrace(tracePtrs, 16);
	char **funcNames = backtrace_symbols(tracePtrs, count);
	for(ii = 0; ii < count; ii++)
		anyka_print("%s\n", funcNames[ii]);
	free(funcNames);
	fflush(stderr);
	fflush(stdout);

    #if 1
	if(sig == SIGINT || sig == SIGSEGV || sig == SIGTERM)
	{
		
	}
    #endif
	exit(1);
#endif
}


/**
* @brief 	user_sig_handler
* 			自定义用户信号处理，接收到信号时进入此接口
* @date 	2015/3
* @param:	int sig，信号值
* @return 	void
* @retval 	
*/
static void user_sig_handler(int sig)
{
	int fd = -1, status = 0;
	char status_buf[100] = {0};

	fd = open(SD_STATUS, O_RDONLY);
	if(fd < 0)
	{
		anyka_print("open file %s failed, %s\n", SD_STATUS, strerror(errno));
		return;
	}
	read(fd, status_buf, 9);
	close(fd);

	status = atoi(status_buf);
	anyka_print("[%s:%d] get status: %d\n", __func__, __LINE__, status);

	switch (status)
	{
		case 0:
			{
				sd_set_status(0);
				anyka_print("sd card is out\n");
				break;
			}
		case 1:
			{
				sd_set_status(1);
				anyka_print("sd card is in\n");	
#ifdef CONFIG_DANA_SUPPORT		
				record_replay_init();	// now, record replay only for dana
#endif
				break;
			}
#ifdef CONFIG_TENCENT_SUPPORT
		case 2:
			{
				anyka_print("we will start voice!\n");
				audio_record_start(VOICE_WORD, ".amr");
				break;
			}
		case 3:
			{
				char msg_file[100] = {0};
				anyka_print("we will send voice!\n");
				audio_record_stop(msg_file);
				ak_qq_send_voice_to_phone(msg_file, time(0));
				break;
			}
		case 10:
			{
				anyka_print("NOTICE: ak qq unbind all !\n");
				ak_qq_unbind();
				break;
			}
#endif
		case 4:
			{
				char voice_file[100] = {0};
				FILE *file=NULL;

				file = fopen("/tmp/voicefile", "rb");
				fscanf(file, "%s", voice_file);
				fclose(file);

				audio_play_start(voice_file, 0);
				anyka_print("we will play voice (%s)\n", voice_file);
				break;
			}
		case 100:
			{
				system_user_info * set_info = anyka_get_system_user_info();

				set_info->debuglog = set_info->debuglog?0:1;
				anyka_print("switch cloud logging %d\n", set_info->debuglog);
#ifdef CONFIG_DANA_SUPPORT
				if(set_info->debuglog)
				{
					dbg_on();
				}
				else
				{
					dbg_off();
				}
#endif
				break;
			}
		case 123:
			{
				camera_save_yuv();	//save yuv
				break;
			}
	}

}



/**
* @brief 	sig_register
* 			信号处理注册函数
* @date 	2015/3
* @param:	void
* @return 	int
* @retval 	0
*/

static int sig_register(void)
{
	signal(SIGSEGV, sigprocess);
	signal(SIGINT, sigprocess);
	signal(SIGTERM, sigprocess);
	signal(SIGUSR1, user_sig_handler);
	signal(SIGUSR2, voice_tips_get_file);	
	signal(SIGCHLD, SIG_IGN);

	return 0;
}

#endif


/**
* @brief 	 main
* 			主应用入口
* @date 	2015/3
* @param:	void
* @return 	int
* @retval 	0
*/

int main(void)
{
    Psystem_cloud_set_info sys_setting;
	system_user_info * set_info;

    #ifdef CONFIG_DANA_SUPPORT
    char * danale_path = "/etc/jffs2/";
    #endif

    anyka_print("*********************************************************\n");
    anyka_print("*************%s_build@%s_%s**********\n", AK_VERSION_SOFTWARE, __DATE__, __TIME__);
    anyka_print("*********************************************************\n");

	/** create a monitor thread  **/
	pthread_t detect_pth_id;
	anyka_pthread_create(&detect_pth_id, (anyka_thread_main *)monitor,
						(void *)NULL, ANYKA_THREAD_MIN_STACK_SIZE, -1);

	/** load config message, than init it **/
    anyka_init_setting(ak_version_manage());
	/** init the hardware **/
    anyka_init_hardware();
	/** init the software **/
	anyka_init_software();
	/** init the voice_tips model **/
	voice_tips_init();
	/** register signal **/
    sig_register();
	
	/** play system start voice tips **/
	if(anyka_check_first_run()) {
		voice_tips_add_music(SYS_START);
	}

	/** get system cloud setting **/
    sys_setting = anyka_get_sys_cloud_setting();
	/** get system user information **/
	set_info = anyka_get_system_user_info();
	
	#ifdef CONFIG_RTSP_SUPPORT
	if(set_info){
		if(set_info->rtsp_support){
			anyka_print("[%s:%d] init rtsp ......\n", __func__, __LINE__);
			anyka_rtsp_init();	//loop, don't  return
		}
	}
	#endif
    
	#ifdef CONFIG_DANA_SUPPORT
    //start danale cloud service
    if(sys_setting->dana)
    {
		/** init dana platform **/
        dana_init(danale_path);
    }
	//record replay
    record_replay_init();
    #endif
    
    #ifdef  CONFIG_ONVIF_SUPPORT
    //start ONVIF service
    if(sys_setting->onvif)
    {
		/** init onvif platform **/
        onvif_init();
		//while(1)
			//sleep(1000);
    }
    #endif

    #ifdef CONFIG_TENCENT_SUPPORT
    if(sys_setting->tencent)
    {
		/** init tencent platform **/
        tencent_init(ak_version_manage());
    }
    #endif

    //anyka_photo_start(60, 5, NULL);
    /** all application is dealwith in this func **/
    video_record_main_loop();
    
    //主循环处理计划录像功能
    return 0;
}
