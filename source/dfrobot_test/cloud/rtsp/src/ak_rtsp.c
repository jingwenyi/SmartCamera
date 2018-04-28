#include "common.h"
#include "anyka_com.h"


#ifdef CONFIG_RTSP_SUPPORT
extern void start_rtsp_service(void);  //include c++ api to call c++ function to start rtsp service
#endif

static int module_init = 0;


void *anyka_rtst_check_net(void)
{
	int rtst_started = 0;
	char cmd[64] = {0}, result[128] = {0};		
	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());

	while(1){
		/********* check current net using wireless or wire **********/
		sprintf(cmd, "ifconfig eth0 | grep RUNNING");
		do_syscmd(cmd, result);
		bzero(cmd, sizeof(cmd));

		if(strlen(result) > 0){
			/** wire model **/
			sprintf(cmd, "ifconfig eth0 | grep inet");
		} else {
			/** wireless model **/
			sprintf(cmd, "ifconfig wlan0 | grep inet");
		}
		bzero(result, sizeof(result));
		do_syscmd(cmd, result); //get ip address

		if(strlen(result) > 0){
			if(rtst_started == 0){
				rtst_started = 1;
				#ifdef CONFIG_RTSP_SUPPORT
				anyka_print("[%s:%d] call rtsp_main.cpp to start rtsp service\n", __func__, __LINE__);	
				start_rtsp_service();
				#endif
			}
			
		}else{
			anyka_print("[%s:%d] device has no ip address, wait 10s and then check next time\n", 
						__func__, __LINE__); 

			rtst_started = 0;
		}

		sleep(10);
	}

	return NULL;
}

/**
* * @brief 	anyka_rtsp_init
* * 		create a thread to check net, only if the device get a ip address we would start the rtsp service
* * @date 	2015/4
* * @param:	void
* * @return	int 
* * @retval 0 -> success, -1 -> failed	
* */

int anyka_rtsp_init(void)
{
	int ret;
	pthread_t rtsp_tid;

	if(module_init == 1){
		anyka_print("[%s:%d] the rtsp service has been started, please close it first\n", __func__, __LINE__);	
		return -1;
	}

	/** get sys config **/
    system_user_info * set_info = anyka_get_system_user_info();
	if(set_info){
		if(set_info->rtsp_support == 0){
			anyka_print("[%s:%d] current config unsupport rtsp service\n", __func__, __LINE__);	
			return -1;
		}
	} else {
		anyka_print("[%s:%d] the set_info is empty, please init it first\n", __func__, __LINE__); 
		return -1;
	}

	/** create a posix thread to check net, after the device get ip, start the rtsp service **/
	ret = anyka_pthread_create(&rtsp_tid, (anyka_thread_main *)anyka_rtst_check_net,
		   						(void *)NULL, ANYKA_THREAD_MIN_STACK_SIZE, 1);
	if(ret != 0){
		anyka_print("[%s:%d] pthread create failed\n", __func__, __LINE__);	
		return -1;
	}

	module_init = 1;

	/*
	  *	此处while1 是指当使用了rtsp 服务之后，
	  *   不启动其他云平台功能。
	*/
	//while(1){
		//sleep(10);
	//}

	return 0;
}
