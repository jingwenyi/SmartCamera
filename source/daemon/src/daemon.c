/*
 *  Copyright (c) 2012-2013 wangsheng gao
 *  Copyright (c) 2012-2013 Anyka, Inc
 *
 */

/********** modify 2014-9-16 ************/
#include "daemon_inc.h"

#define FIFO_PATH			"/tmp/daemon_fifo"
#define	RESTART_AK_IPC		"/usr/sbin/anyka_ipc.sh"
#define UPDATE_SH			"/usr/sbin/update.sh"
#define SERVICE				"/usr/sbin/service.sh"

#define FIFO_SZ (50)
#define INTERVAL (5) //5seconds

#define KEY_GPIO_DEV		"/dev/input/event0"
#define SDP1_DEV_NAME       "/dev/mmcblk0p1"
#define SD_DEV_NAME         "/dev/mmcblk0"

#define SD_TOOL				"dsk_repair"
#if 0
#define MOUNT_SDP1			"/bin/mount -rw /dev/mmcblk0p1 /mnt"
#define MOUNT_SD			"/bin/mount -rw /dev/mmcblk0 /mnt" 
#define UMOUNT_SD			"/bin/umount /mnt"
#else
#define MOUNT_SDP1			"mount -rw /dev/mmcblk0p1 /mnt" 
#define MOUNT_SD			"mount -rw /dev/mmcblk0 /mnt" 
#define UMOUNT_SD			"umount /mnt -l"
#endif

#define UPDATE_IMAGE			10 //10 seconds
#define UEVENT_BUFFER_SIZE      2048
/****** key 1 : reset *****/
#define RECOVER_DEV				5
#define UPDATE_IMAGE			10

#define TCP_PORT 		6789
#define TCP_HB_PORT		6790
#define UPD_PORT 		8192
#define SUCCESS			"1"
#define FAILURE			"0"
#define TEST_STAT 		"/var/stat"
#define TCP_BUF_SIZE 	1024
//#define DEBUG 	//for test print
#define QQ_VOICE_MODE_SWITCH

/****************  watch_dog *********************/
#ifdef WATCHDOG_ENABLE
#define WATCHDOG_IOCTL_BASE 'W'
#define WDIOC_KEEPALIVE     _IOR(WATCHDOG_IOCTL_BASE, 5, int)
#define	WDIOC_SETTIMEOUT    _IOWR(WATCHDOG_IOCTL_BASE, 6, int)
#define WDIOC_GETTIMEOUT    _IOR(WATCHDOG_IOCTL_BASE, 7, int)
static int watchdog_fd = 0;

static pthread_t watchdog_id;
#endif
static int watchdog_en = 1;
//pthread_t daemon_pth_id;
static int monitor_runflags = 0;
static int test_running = 0;
static int run_flag = 0;//timer run flag
static struct timeval start_time;
char last_file[20] = {0};

/********************** Function *************************/

/**
 * *  @brief		enable_watch_dog pthread
 * *  @author	chen yanhong   
 * *  @date		2014-09-17
 * *  @param	not used 
 * *  @return		void  *
 * */
#ifdef WATCHDOG_ENABLE
void* daemon_anyka_feed_watchdog(void* param)
{	
	int time_val = -1;
	struct timeval tv;
	unsigned long ts_interval = 0;

	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());

	gettimeofday(&tv, NULL);
	ts_interval = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	while(watchdog_en)
	{	
        ioctl(watchdog_fd, WDIOC_KEEPALIVE, 0);
		sleep(2);

		gettimeofday(&tv, NULL);
		if( (tv.tv_sec * 1000 + tv.tv_usec / 1000) - ts_interval > 2500 )
			anyka_print("[%s:%d] schedule to long, interval: %lu\n", __func__, __LINE__, 
								(tv.tv_sec * 1000 + tv.tv_usec / 1000) - ts_interval);
		ts_interval = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
    }

	anyka_print("Going to Close WATCH_DOG\n");
	ioctl(watchdog_fd, WDIOC_SETTIMEOUT, &time_val);

	if(close(watchdog_fd) < 0)
	{
		perror("close watch_dog:");
	}

	anyka_print("[%s:%d] Watch_dog closed, thread id: %ld\n", __func__, __LINE__, (long int)gettid());

    return NULL;
}

/**
 * *  @brief		enable_watch_dog
 * *  @author	chen yanhong   
 * *  @date		2014-09-17
 * *  @param	null 
 * *  @return		void
 * */
void daemon_watchdog_enable()
{
	anyka_print("********** Watch Dog Enabled! **********\n");
	
    watchdog_fd = open("/dev/watchdog", O_RDONLY);
    int timeout = 0;
    if(watchdog_fd < 0)
    {
        anyka_print("open watchdog_fd failed\n");
    }
    else
    {
		watchdog_en = 1;
		fcntl(watchdog_fd, F_SETFD, FD_CLOEXEC);	
        ioctl(watchdog_fd, WDIOC_GETTIMEOUT, &timeout);
        anyka_print("watchdog timeout = %d(s)\n", timeout);
        anyka_pthread_create(&watchdog_id, 
							(anyka_thread_main *)daemon_anyka_feed_watchdog, 
							NULL, ANYKA_THREAD_MIN_STACK_SIZE, 99);
    }
}
#endif

/**
 * *  @brief		close_watch_dog && close_monitor_anyka_ipc
 * *  @author	chen yanhong   
 * *  @date		2016-03-30
 * *  @param	signal id 
 * *  @return		void
 * */
void daemon_close_watch_dog(int signo)
{
	watchdog_en = 0;
	return ;
}


static int do_syscmd(char *cmd, char *result)
{
	char buf[512];	
	FILE *filp;
	
	filp = popen(cmd, "r");
	if (NULL == filp){
		anyka_print("%s->%d: popen fail!\n", __func__, __LINE__);
		return -2;
	}

	/* fgets(buf, sizeof(buf)-1, filp); */
	memset(buf, '\0', sizeof(buf));
	fread(buf, sizeof(char), sizeof(buf)-1, filp);
	
	sprintf(result, "%s", buf);
	
	pclose(filp);
	return strlen(result);	
}

int key_press_status = -1;

/**********************  timer  ************************/
/**
 * *  @brief		set_timer
 * *  @author	chen yanhong   
 * *  @date		2014-09-17
 * *  @param	sec, second you want to set 
 * *  @return		void
 * */
void daemon_set_timer(int sec)
{
	struct itimerval s_time;

	s_time.it_value.tv_sec  = sec;
	s_time.it_value.tv_usec = 0;
	
	s_time.it_interval.tv_sec  = 0; //是否需要多次使用?	
	s_time.it_interval.tv_usec = 0;

	setitimer(ITIMER_REAL, &s_time, NULL);

	return ;
}

/*******************************************************/
/**
 * *  @brief		alarm_print,signal SIGALRM callback
 * *  @author	chen yanhong   
 * *  @date		2014-09-17
 * *  @param	msg
 * *  @return		void
 * */
static void daemon_alarm_print(int msg)
{
	if(run_flag == 0)
		return;
	
	switch(msg){
		case SIGALRM:
            if(key_press_status == 1)
            {
                system("echo \"/usr/share/anyka_di_cn.mp3\" > /tmp/alarm_audio_list");
                system("killall -12 anyka_ipc");
                key_press_status = 2;
                daemon_set_timer(5);
    			anyka_print("****************************************************\n");
    			anyka_print("Press down KEY0 is over 5 second,You can loosen it!\n");
    			anyka_print("****************************************************\n");	
            }
            else
            {
                system("echo \"/usr/share/anyka_di_cn.mp3\" > /tmp/alarm_audio_list");
                system("killall -12 anyka_ipc");
                //daemon_set_timer(2);
    			anyka_print("****************************************************\n");
    			anyka_print("Press down KEY0 is over 10 second,You can loosen it!\n");
    			anyka_print("****************************************************\n");	
            }		
			break;
			
		default:
			break;
	}
	return ;
}

/**
 * *  @brief		creat_fifo
 * *  @author		chen yanhong   
 * *  @date		2014-09-17
 * *  @param		void 
 * *  @return		void
 * */
/********************* 创建管道**************************/
void daemon_create_fifo()
{
	if(access(FIFO_PATH, F_OK) < 0)
	{
		mkfifo(FIFO_PATH, 0777);
	}
	return ;
}


/**
 * *  @brief		monitor
 * *  @author		chen yanhong   
 * *  @date		2014-09-17
 * *  @param		void 
 * *  @return		NULL
 * */
/******************* 监控线程 ***********************/
static void *daemon_monitor()
{
	char cmd[128] = {0};
	char recv_hb[FIFO_SZ] = {0};
	int fifo_fd, rd_ret;		//chech heart beat fd
	
	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());

	daemon_create_fifo();
	anyka_print("MainAPP monitor started, interval:%dSec, fifo[%s].size=%d\n", INTERVAL, FIFO_PATH, FIFO_SZ);

	fifo_fd = open(FIFO_PATH, O_RDONLY);	//open fifo 

	fcntl(fifo_fd, F_SETFD, FD_CLOEXEC);	//set attrubitu
	
	while(monitor_runflags)
	{	
		rd_ret = read(fifo_fd, recv_hb, FIFO_SZ);
		
		if(watchdog_en && (rd_ret == 0))
		{
			anyka_print("*********************************\n");
			anyka_print("The MainApp was dead, restart it!\n");
			
			sprintf(cmd, "%s %s", RESTART_AK_IPC, "restart");
			anyka_print("call: %s\n", cmd);
			system(cmd);
		}
		
		sleep(INTERVAL);
	}
	
	close(fifo_fd);
	
	anyka_print("[%s:%d] Exit the monitor thread, id : %ld\n", __func__, __LINE__, (long int)gettid());
	return NULL;
}



/**
 * *  @brief       diff timeval
 * *  @author      gao wangsheng
 * *  @date        2012-10-15
 * *  @param[in]   struct timeval *old, *new
 * *  @return      diff time
 * */
static double daemon_difftimeval (struct timeval *old, struct timeval *new)
{
	return (new->tv_sec - old->tv_sec) * 1000. + (new->tv_usec - old->tv_usec) / 1000;
}


/**
 * *  @brief    daemon_get_lastest_file
 			根据时间排序找出修改之间最晚的文件
 * *  @author   
 * *  @date        
 * *  @param[in]   char *file_name，存放文件名，绝对路径
 * *  @return      int， 0 失败，1 成功
 * */

int daemon_get_lastest_file(char *file_name)
{
	anyka_print("[%s:%d] get last file\n", __func__, __LINE__);

	system("ls -t /mnt/alarm_record_dir/*.mp4 >> /tmp/record_lastest.filelist");
	FILE *file_list = fopen("/tmp/record_lastest.filelist", "r");

	if(file_list == NULL){
		return 0;
	}

	if(NULL == fgets(file_name, 100, file_list)){
		fclose(file_list);
		remove("/tmp/record_lastest.filelist");
		return 0;
	}
	fclose(file_list);
	remove("/tmp/record_lastest.filelist");
	return 1;

}

/**
 * *  @brief       daemon_check_sd_prepare
 				write something to the TF card
 * *  @author      
 * *  @date        2015-5-18
 * *  @param[in]     void
 * *  @return      
 * */

void daemon_test_sd_ro(void)
{
	system("dd if=/dev/zero of=/mnt/test_tf bs=1024 count=1");
	system("rm -rf /mnt/test_tf");
}


/**
 * *  @brief       daemon_check_sd_ro
 				check the sd card 
 * *  @author      
 * *  @date        2015-5-18
 * *  @param[in]     int flag, 1->mmcblkp0,    0->mmcblk 
 * *  @return      
 * */

void daemon_check_sd_ro(int mmcblk0_p1)
{
	char cmd[128] = {0}, result[256], badfile[100] ={0}, dev_name[20] = {0};

	daemon_test_sd_ro();
	
	if(mmcblk0_p1)
		sprintf(dev_name, "%s", SDP1_DEV_NAME);		//"/dev/mmcblk0p1"
	else
		sprintf(dev_name, "%s", SD_DEV_NAME);		//"/dev/mmcblk0"

	sprintf(cmd, "mount | grep %s", dev_name); //do "mount | grep dev"

	do_syscmd(cmd, result);
	
	if(strlen(result) > 0){
		if(strstr(result, "rw,relatime") == NULL){			//if not read write	
			if(strstr(result, "ro,relatime") != NULL){			//find ro lable, the card was read only

				anyka_print("[%s:%d] The SD Card is Read Only, repair it\n", __func__, __LINE__);
			
				if(daemon_get_lastest_file(badfile)){		//get the last modify file name	
				
					anyka_print("[%s:%d] The badfile in sd card is: %s\n", __func__, __LINE__, badfile);
				
					bzero(cmd, sizeof(cmd));
					sprintf(cmd, "%s %s %s", SD_TOOL, badfile, dev_name);
					system(cmd);		//delete the badfile 
					sync();
				}
			}
		}else{
			anyka_print("[%s:%d] The SD Card is OK\n", __func__, __LINE__);
			return;
		}
	}
}

int system_start_flag = 1;

/**
 * *  @brief       daemon_mount_sd
 * *  @author     
 * *  @date        
 * *  @param[in]   int flag, 1->mmcblkp0,    0->mmcblk 
 * *  @return      void
 * */

/* create a sd_test dir and mount the sd card in it */
void daemon_mount_sd(int flag)
{
	char cmd[128], status[20] = {0};
	int mmcblk0_p1;

    if(flag == 0)
    {
        if(access(SDP1_DEV_NAME, R_OK) >= 0)
        {
            anyka_print("**********we will skip mount /dev/mmcblk0*******\n");
            return;
        }
    }

	if (flag){
		mmcblk0_p1 = 1;
		sprintf(cmd, "%s", MOUNT_SDP1);
	}
	else{
		mmcblk0_p1 = 0;
		sprintf(cmd, "%s", MOUNT_SD);
	}
    system(cmd);

	//check sdcard readonly or not, and repair it.
	daemon_check_sd_ro(mmcblk0_p1);

	/*
	* when the sd card message is not come first time, 
	* it means hot plug and we will send sig to anyka_ipc 
	*/
    if(system_start_flag == 0)
    {
    	//send message to anyka_ipc that card is ready
		sprintf(status, "echo %d > %s", 1, "/tmp/sd_status");
		system(status);
        system("killall -10 anyka_ipc"); //send signal to ipc
    }
	anyka_print("[%s:%d] *** mount the sd to /mnt ***\n", __func__, __LINE__);
	
}

/**
 * *  @brief    daemon_umount_sd
 			卸载SD 卡
 * *  @author   
 * *  @date        
 * *  @param[in]   void
 * *  @return      void
 * */

/* umount the sd card and delete the sd_test dir */
void daemon_umount_sd(void)
{
	char cmd[128] = {0};

	/** notice the anyka_ipc that  **/
	sprintf(cmd, "echo %d > %s; killall -10 anyka_ipc", 0, "/tmp/sd_status");
	system(cmd);

	// the card had extracted, so the sync is no use actually
#if 0
	/*
	 * sync() function case all information in memory than updates file systems to be scheduled for writing 
	 * out to all file systems. The writing although scheduled, is not necessarily complete upon return form sync().
	 * So that, we sleep a moments
	 */
	sync();
	usleep(500*1000);
#endif

    system(UMOUNT_SD);
	anyka_print("[%s:%d] *** umount the sd ***\n", __func__, __LINE__);
	
}


/**
 * *  @brief    daemon_init_hotplug_sock
 			热插拔检测
 * *  @author   
 * *  @date        
 * *  @param[in]   void
 * *  @return      int， -1， 失败；else 成功
 * */

/* create the socket to recevie the uevent */
static int daemon_init_hotplug_sock(void)
{
	struct sockaddr_nl snl;
    const int buffersize = 2048;
    int retval;

    memset(&snl, 0x00, sizeof(struct sockaddr_nl));

    snl.nl_family = AF_NETLINK;
    snl.nl_pid = getpid();
    snl.nl_groups = 1;

	int hotplug_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if (hotplug_sock == -1)	{
        anyka_print("[%s:%d] socket: %s\n", __func__, __LINE__, strerror(errno));
        return -1;
    }

    /* set receive buffersize */
    setsockopt(hotplug_sock, SOL_SOCKET, SO_RCVBUFFORCE, &buffersize, sizeof(buffersize));

    retval = bind(hotplug_sock, (struct sockaddr *)&snl, sizeof(struct sockaddr_nl));
    if (retval < 0) {
	    anyka_print("[%s:%d] bind failed: %s\n", __func__, __LINE__, strerror(errno));
        close(hotplug_sock);
        hotplug_sock = -1;
    }

    return hotplug_sock;
}


/* waitting the uevent and do something */
void *daemon_pth_func(void *data)
{	
    char buf[UEVENT_BUFFER_SIZE * 2] = {0};
    char temp_buf[20];	//find action
    char *p;	//find blk
    int i;
    int hotplug_sock;	//hotplug socket handle
    int p1_removed = 0;
	
	anyka_print("[%s:%d] This thread id: %ld\n", __func__, __LINE__, (long int)gettid());

	hotplug_sock = daemon_init_hotplug_sock();	
    if (hotplug_sock < 0)
		return NULL;
	
    system_start_flag = 1;
	sleep(1);
    if (access (SDP1_DEV_NAME, F_OK) == 0)
		daemon_mount_sd(1);
	else if (access (SD_DEV_NAME, F_OK) == 0)
		daemon_mount_sd(0);
    system_start_flag = 0;
	
    while(1) {
		//clear buf
        memset(buf, 0, sizeof(buf));

		//block here to wait signal
        recv(hotplug_sock, &buf, sizeof(buf), 0);
        p = strrchr(buf, '/');	//get block name

		//get action
        for (i = 0; buf[i] != '@' && buf[i] != 0; i++)
            temp_buf[i] = buf[i];
        temp_buf[i] = 0;

		if (strcmp(temp_buf, "change"))
			anyka_print("%s\n", buf);

		//card insert
		if (!strcmp(temp_buf, "add")) {
			if (!strcmp(p, "/mmcblk0p1")) {
				//sleep(1);
				daemon_mount_sd(1);
			} else if (!strcmp(p, "/mmcblk0")) {
				//sleep(1);
				daemon_mount_sd(0);
			}
			p1_removed = 0;
			continue;
		}

		//card extract
		if (!strcmp(temp_buf, "remove")) {

			//if p1 removed, than we do need to umount k0
			if(!strcmp(p, "/mmcblk0p1")) {
				daemon_umount_sd();
				p1_removed = 1;
			} else if((!strcmp(p, "/mmcblk0")) && (!p1_removed))
				daemon_umount_sd();
		}
    }

	anyka_print("[%s:%d] Exit thread, id : %ld\n", __func__, __LINE__, (long int)gettid());
	return NULL;
}

/**
 * *  @brief       do key_0
 * *  @author      gao wangsheng
 * *  @date        2012-10-15
 * *  @param[in]   pressing period
 * *  @return      0 on success
 * */
static int daemon__do_gpio_key_1(double period)
{
	if (period < RECOVER_DEV)
	{
		anyka_print("[%s:%d] switch wps.\n", __func__, __LINE__);
		/*do switch wps*/

	}
	else if(period >= RECOVER_DEV && period < UPDATE_IMAGE)		
	{	
		anyka_print("[%s:%d] *** recover system config ***\n", __func__, __LINE__);
		/***** recover device *****/
		system("/usr/sbin/recover_cfg.sh");
		system("reboot");
	}
	else
	{
		/***** update device *****/	
		anyka_print("[%s:%d] update device, call:%s\n", __func__, __LINE__, UPDATE_SH);
		system(UPDATE_SH);	
	}

	return 0;
}
/**
 * *  @brief		do_gpio_key_1
 * *  @author		chen yanhong   
 * *  @date		2014-11-25
 * *  @param		input_event struct 
 * *  @return		key press down,return 1;else return 0.
 * */
static int daemon_do_gpio_key_1(struct input_event *event)
{
	double period  = 0;

	anyka_print("[%s:%d] ### soft reset key put down !!!\n", __func__, __LINE__);
	/***** key put down *****/
	if (event->value == 1)
	{
		start_time.tv_sec = event->time.tv_sec;
		start_time.tv_usec = event->time.tv_usec;	
        key_press_status = 1;
		daemon_set_timer(5);	//启动定时器
		run_flag = 1;		
	}
	/***** key put up *****/
	else if (event->value == 0)
	{
        run_flag = 0;
		period = daemon_difftimeval(&start_time, &event->time) / 1000;
		anyka_print("key 1 put down, period = %f(s).\n", period);
		daemon__do_gpio_key_1(period);
		return 0;
	}
	return 1;
}

#ifndef QQ_VOICE_MODE_SWITCH

/**
 * *  @brief       do key_0
 * *  @author      gao wangsheng
 * *  @date        2012-10-15
 * *  @param[in]   pressing period
 * *  @return      0 on success
 * */
static int daemon__do_gpio_key_0(double period)
{
	if (period < UPDATE_IMAGE)
	{
		run_flag = 0;
		if(period < 0)
		{
			perror("Error period");
			return -1;
		}
	}
	//else if(period > UPDATE_IMAGE)		
	else
	{
		anyka_print("update call:%s\n", UPDATE_SH);
		system(UPDATE_SH);	
	}

	return 0;
}
#endif
/**
 * *  @brief		do_gpio_key_0
 * *  @author		chen yanhong   
 * *  @date		2014-09-17
 * *  @param		input_event struct 
 * *  @return		key press down,return 1;else return 0.
 * */
static int daemon_do_gpio_key_0(struct input_event *event)
{
	#ifndef QQ_VOICE_MODE_SWITCH
	double period  = 0;
	#else
    char cmd[40];
	#endif

	if (event->value == 1)
	{
	#ifdef QQ_VOICE_MODE_SWITCH
		sprintf(cmd, "echo %d > %s", 2, "/tmp/sd_status");
		system(cmd);
		system("killall -10 anyka_ipc");
	#else
		daemon_set_timer(10);	//启动定时器
		run_flag = 1;
		start_time.tv_sec = event->time.tv_sec;
		start_time.tv_usec = event->time.tv_usec;	
	#endif

	}
	else if (event->value == 0)
	{
	#ifdef QQ_VOICE_MODE_SWITCH
		sprintf(cmd, "echo %d > %s", 3, "/tmp/sd_status");
		system(cmd);
		system("killall -10 anyka_ipc");
		//run_flag = 0;
	#else
		period = daemon_difftimeval(&start_time, &event->time) / 1000;
		anyka_print("period = %f(s).\n", period);
		daemon__do_gpio_key_0(period);
	#endif
		return 0;

	}

	return 1;
}
#ifndef QQ_VOICE_MODE_SWITCH

/**
 * *  @brief			standby mode
 * *  @author		chen yanhong   
 * *  @date			2014-12-08
 * *  @param		input_event struct 
 * *  @return		void.
 * */
static int daemon_do_standby(struct input_event *event)
{
#define PRESS_STANDBY_TIME	2
	double period;

	if (event->value == 1)	//按下开始计时
	{
		start_time.tv_sec = event->time.tv_sec;
		start_time.tv_usec = event->time.tv_usec;			
	}
	else if (event->value == 0)	//松开停止计时，获取按键时长
	{
		//run_flag = 0;
		period = daemon_difftimeval(&start_time, &event->time) / 1000;
		anyka_print("period = %f(s).\n", period);
		if (period >= PRESS_STANDBY_TIME) {
			/* 设置RTC alarm唤醒时间,单位秒 */
			rtc_wakeup_standby(60 * 2);

			/* 进入standby */
			system("/usr/sbin/standby.sh &");
		}
	}

	return 0;
}

#endif

/**
 * *  @brief       do_key
 * *  @author      gao wangsheng
 * *  @date        2012-12-19
 * *  @param[in]   struct input_event *event, int key event count
 * *  @return      0 on success
 * */
static int daemon_do_key(struct input_event *key_event, int key_cnt)
{
	int i = 0;
	int ret = -1;
	struct input_event *event;

	if(test_running)
		return 0;

	if (key_cnt < (int) sizeof(struct input_event)) {
		anyka_print("expected %d bytes, got %d\n", (int) sizeof(struct input_event), key_cnt);
		return -1;
	}

	for (i = 0; (i < key_cnt/sizeof(struct input_event)); i++) 
	{
		event = (struct input_event *)&key_event[i];
		if (EV_KEY != event->type)
		{
			continue;
		}

		anyka_print("count = %d, code = %d, value = %d!\n", 
				key_cnt/sizeof(struct input_event), event->code, event->value);
		
		anyka_print("[%s:%d] handler event:", __func__, __LINE__);

#if 1
		switch(event->code)
		{
			case KEY_0:
				anyka_print(" KEY_0\n");
				ret = daemon_do_gpio_key_0(event);

				/* standby 测试*/
				#ifndef QQ_VOICE_MODE_SWITCH
				daemon_do_standby(event);
				#endif
				break;

			case KEY_1:
				anyka_print("KEY_1\n");
				ret = daemon_do_gpio_key_1(event);
				break;

			default:
				anyka_print("[%s:%d] event->code: %d, Error key code!", __func__, __LINE__, event->code);
				ret = -1;
				break;
		}
#endif

		if (!ret){
			break;
		}
	}
	return ret;
}

/**
 * *  @brief       daemon_sig_fun, now no used
 * *  @author      chen yanhong
 * *  @date        2014-10-28
 * *  @param[in]   send buf, a pointer to data
 * *  @return      void
 * */

void daemon_sig_fun(int signo)
{
	anyka_print("SIGUSR1 call /usr/sbin/hello.sh\n");
	system("/usr/sbin/hello.sh");
}


/**
 * *  @brief       cp the struct data pointer by data to the buf
 * *  @author      chen yanhong
 * *  @date        2014-10-28
 * *  @param[in]   send buf, a pointer to data
 * *  @return      void
 * */
void daemon_cp_to_buf(char * send_buf, Pdata_struct data)
{
	//len
	memcpy(send_buf, &data->len, sizeof(data->len));
	send_buf+=sizeof(data->len);
	//type
	memcpy(send_buf, &data->type, sizeof(data->type));
	send_buf+=sizeof(data->type);
	//auto_test
	memcpy(send_buf, &data->auto_test, sizeof(data->auto_test));
	send_buf+=sizeof(data->auto_test);
	//file len
	memcpy(send_buf, &data->file_len, sizeof(data->file_len));
	send_buf+=sizeof(data->file_len);

	//file name
	if(data->file_len > 0)
	{
		memcpy(send_buf, data->file_name, data->file_len);
		send_buf+=data->file_len;
	}
	memcpy(send_buf, &data->arg_len, sizeof(data->arg_len));	//arg len
	send_buf+=sizeof(data->arg_len);

	if(data->arg_len > 0)
	{
		memcpy(send_buf, data->arg_data, data->arg_len);	//arg-data
		send_buf+=data->arg_len;
	}
	memcpy(send_buf, &data->check_sum, sizeof(data->check_sum));	//check sum data
	send_buf+=sizeof(data->check_sum);

	return ;
}


/**
 * *  @brief       calculate the data's sum which pointer by data 
 * *  @author      chen yanhong
 * *  @date        2014-10-28
 * *  @param[in]   a pointer to data
 * *  @return      result after calculate
 * */
int daemon_calc_checksum(Pdata_struct data)
{
	int i;
	
	data->check_sum = (data->type)+(data->auto_test)+(data->file_len)+(data->arg_len);
	for(i = 0; i < (data->file_len); i++)
	{
		data->check_sum+=(data->file_name[i]);
	}
	for(i = 0; i < (data->arg_len); i++)
	{
		data->check_sum+=(unsigned short)(data->arg_data[i]);
	}
	
	return data->check_sum;
}

/**
 * *  @brief       	 cp the struct data pointer by data to the buf
 * *  @author      	 chen yanhong
 * *  @date       	 2014-10-28
 * *  @param[in]   a descripter which you want to send,  when send success info,result = 1,else 0
 * *  @return        result = 0,success; -1,failure
 * */
int daemon_response(int send_to_fd, int result)
{	
	char send_buf[200] = {0};
	int send_fd = send_to_fd;
	int ret;
	
	Pdata_struct response_data = (Pdata_struct)malloc(sizeof(data_struct));
	if(response_data == NULL)
	{
		anyka_print("[%s:%d] fail to malloc.\n", __func__, __LINE__);
		return -1;
	}

	response_data->type = RESPONSE;		//message type
	response_data->auto_test = 1;
	response_data->file_len = 0;
	response_data->file_name = NULL;
	response_data->arg_len = 1;		//just report one bit, 1 -- > success, 0, failed 
	
	response_data->len = (sizeof(response_data->len))+(sizeof(response_data->type))
		+(sizeof(response_data->auto_test))+(sizeof(response_data->file_len))
		+(sizeof(response_data->arg_len))+(response_data->file_len)+(response_data->arg_len)
		+(sizeof(response_data->check_sum));
	//success
	if(result == 0)
	{
		response_data->arg_data = (char *)malloc(2);
		strncpy(response_data->arg_data, SUCCESS, 1);
	}
	else//failure
	{
		response_data->arg_data = (char *)malloc(2);
		strncpy(response_data->arg_data, FAILURE, 1);		
	}
	response_data->check_sum = daemon_calc_checksum(response_data);		//get check sum
	//anyka_print("[%s:%d] check_sum : %d\n", __func__, __LINE__, response_data->check_sum);

	daemon_cp_to_buf(send_buf, response_data);	//copy the message to the buf according specific relus.

#ifdef DEBUG
	int i;
	for (i = 0; i < 10; i++)
	{
		anyka_print("%02x ", send_buf[i]);
	}
	anyka_print("\n");
#endif

	/*** send message ***/
	ret = send(send_fd, send_buf, response_data->len, 0);
	if(ret < 0)
	{
		anyka_print("[%s:%d] send response data faild, error:%s\n", __func__, __LINE__, strerror(errno));
	}
	
#ifdef DEBUG
	anyka_print("respons len : %d\n", response_data->len);
	anyka_print("respons type : %d\n", response_data->type);
	anyka_print("respons auto_test : %d\n", response_data->auto_test);
	anyka_print("respons file_len : %d\n", response_data->file_len);
	anyka_print("respons arg_len : %d\n", response_data->arg_len);	
	anyka_print("respons chenck_sum : %d\n", response_data->check_sum);
#endif

	//release resource
	if(response_data->arg_data)
		free(response_data->arg_data);	
	if(response_data->file_name)
		free(response_data->file_name);
	free(response_data);

	return ret;
}


/**
 * *  @brief       	 get the test program's execute result
 * *  @author      	 chen yanhong
 * *  @date       	 2014-10-28
 * *  @param[in]   void
 * *  @return        result = 0,success; -1,failure
 * */
int daemon_get_exec_result()
{		
	char res[4] = {0};
	if(access(TEST_STAT, F_OK) < 0)
	{
		anyka_print("[%s:%d] the program has not run.\n", __func__, __LINE__);
		return -1;
	}
	
	int fd = open(TEST_STAT, O_RDONLY);
	if(fd < 0)
	{
		anyka_print("[%s:%d] open result file failed.\n", __func__, __LINE__);
		return -1;
	}

	if(read(fd, res, sizeof(res)) < 0)
	{
		anyka_print("[%s:%d] read result file failed.\n", __func__, __LINE__);
		return -1;
	}

	anyka_print("[%s:%d] after execute test program, return value is : %d\n", 
					__func__, __LINE__, atoi(res));
	return atoi(res);
}

/**
 * *  @brief       	 fill the info to the struct
 * *  @author      	 chen yanhong
 * *  @date       	 2014-10-28
 * *  @param[in]   info buf which will be filled to the struct
 * *  @return        a pointer to data
 * */
Pdata_struct daemon_create_struct_info(char *buf)
{
	Pdata_struct data = (Pdata_struct)malloc(sizeof(data_struct));
	if(data == NULL)
	{
		anyka_print("[%s:%d] fail to malloc.\n", __func__, __LINE__);
		return NULL;
	}
	memset(data, 0, sizeof(data_struct));
	//len
	memcpy(&data->len, buf, sizeof(data->len));
	buf+=sizeof(data->len);
	//type
	memcpy(&data->type, buf, sizeof(data->type));
	buf+=sizeof(data->type);
	//auto ?
	memcpy(&data->auto_test, buf, sizeof(data->auto_test));
	buf+=sizeof(data->auto_test);
	//file len
	memcpy(&data->file_len, buf, sizeof(data->file_len));
	buf+=sizeof(data->file_len);
	//file name
	if(data->file_len > 0)
	{
		data->file_name = (char *)malloc(data->file_len + 1);
		memcpy(data->file_name, buf, data->file_len);
		data->file_name[data->file_len] = 0;
		buf+=data->file_len;
	}
	//argument len
	memcpy(&data->arg_len, buf, sizeof(data->arg_len));
	buf+=sizeof(data->arg_len);
	//argument data
	if(data->arg_len > 0)
	{
		data->arg_data = (char *)malloc(data->arg_len + 1);
		memcpy(data->arg_data, buf, data->arg_len);
		data->arg_data[data->arg_len] = 0;
		buf+=data->arg_len;
	}
	//check sum
	memcpy(&data->check_sum, buf, sizeof(data->check_sum));
	buf+=sizeof(data->arg_len);

	return data;
}


/**
 * *  @brief       	 free the struct memery
 * *  @author      	 chen yanhong
 * *  @date       	 2014-10-28
 * *  @param[in]   a pointer to data while you want to free it's memery
 * *  @return        void
 * */
void daemon_destroy_struct_info(Pdata_struct data)
{
	if(data->file_name)
		free(data->file_name);
	if(data->arg_data)
		free(data->arg_data);
	if(data)
		free(data);
	return ;
}

/**
 * *  @brief       	 parse receive data, then execute it
 * *  @author      	 chen yanhong
 * *  @date       	 2014-10-28
 * *  @param[in]   info buf which was receive by system call recv, client socket file descriptor
 * *  @return        execute result, 0 succed ;else failure
 * */
int daemon_parse_exec(char *buf, int cfd)
{	
	char cmd_buf[256] = {0};
	short check_sum_test;
	int ret = 0;
	Pdata_struct parse_data;

	parse_data = daemon_create_struct_info(buf);
	if(parse_data == NULL)
		return -1;

	/** debug print **/
#ifdef DEBUG
	anyka_print("[%s:%d] recv len : %d\n", 		 __func__, __LINE__, parse_data->len);
	anyka_print("[%s:%d] recv type : %d\n", 	 __func__, __LINE__, parse_data->type);
	anyka_print("[%s:%d] recv auto_ts : %d\n", 	 __func__, __LINE__, parse_data->auto_test);
	anyka_print("[%s:%d] recv file_len : %d\n",  __func__, __LINE__, parse_data->file_len);
	anyka_print("[%s:%d] recv arg_len : %d\n", 	 __func__, __LINE__, parse_data->arg_len);
	anyka_print("[%s:%d] recv check : %d\n", 	 __func__, __LINE__, parse_data->check_sum);
	anyka_print("[%s:%d] recv file name : %s\n", __func__, __LINE__, parse_data->file_name);
	anyka_print("[%s:%d] recv arg name : %s\n",  __func__, __LINE__, parse_data->arg_data);
#endif
	//check len
	if(parse_data->len == 0)
	{
		anyka_print("[%s:%d] receive data error, data size is 0\n", __func__, __LINE__);
		ret = -1;
		goto bye_bye;
	}
	//check sum
	check_sum_test = daemon_calc_checksum(parse_data);
	if(check_sum_test != parse_data->check_sum)
	{
		anyka_print("[%s:%d] receive data error, the check sum is not coincident.\n", __func__, __LINE__);
		ret = -1;
		goto bye_bye;
	}

	/** change word dir the /tmp **/
	chdir("/tmp/");

	//kill last program
	if(last_file[0]) 
	{
		bzero(cmd_buf, sizeof(cmd_buf));
		
		sprintf(cmd_buf, "/usr/sbin/kill_pro.sh %s", last_file);
		system(cmd_buf);	//kill program

		bzero(last_file, sizeof(last_file)); 
		bzero(cmd_buf, sizeof(cmd_buf));
	}
	
	//check finish
	if(parse_data->type == FINISH)
	{
		anyka_print("[%s:%d] going to restart service\n", __func__, __LINE__);
		ret = 2;
		goto bye_bye;
	}

	if(access(parse_data->file_name, F_OK) < 0)
	{
		anyka_print("[%s:%d] the executable file: %s not exist.\n",
						 __func__, __LINE__, parse_data->file_name);
		ret = -1;
		goto bye_bye;
	}


	/* 1 */
	sprintf(cmd_buf, "chmod u+x %s", parse_data->file_name);
	system(cmd_buf);
	bzero(cmd_buf, sizeof(cmd_buf));
	/* 2 */
	sprintf(cmd_buf, "rm -rf %s", TEST_STAT);
	system(cmd_buf);
	bzero(cmd_buf, sizeof(cmd_buf));

	/* 3 */
	//manual test mode, response message to PC
	if(!(parse_data->auto_test))
	{
		daemon_response(cfd, 0);	//response the message immediately on manual test mode

		bzero(cmd_buf, sizeof(cmd_buf));
		if(parse_data->arg_data)
		{
			//run background
			sprintf(cmd_buf, "./%s %s %s %s", parse_data->file_name, 
						parse_data->arg_data, TEST_STAT, "&");
		}
		else
		{	//run background
			sprintf(cmd_buf, "./%s %s %s", parse_data->file_name, TEST_STAT, "&");
		}
		ret = 1;
	}
	else
	{
		if(parse_data->arg_data)	//has arg
		{
			sprintf(cmd_buf, "./%s %s %s", parse_data->file_name, parse_data->arg_data, TEST_STAT);
		}
		else	//has not arg
		{
			sprintf(cmd_buf, "./%s %s", parse_data->file_name, TEST_STAT);
		}
	}
	anyka_print("[%s:%d]cmd: %s\n", __func__, __LINE__, cmd_buf);
	strcpy(last_file, parse_data->file_name);	//save last file name
	anyka_print("[%s] last file name: %s\n", __func__, last_file);

	system(cmd_buf);	//execute the command 
	
	if(ret != 1)
	{
		ret = daemon_get_exec_result();	//get the command execute result
	}
	
bye_bye:
	daemon_destroy_struct_info(parse_data);		//free the message resource
	return ret;
}

/**
 * *  @brief       	 send heart beat to client
 * *  @author      	 chen yanhong
 * *  @date       	 2014-10-28
 * *  @param[in]   struct which include client socket file descriptor and a pthread run flags
 * *  @return        void *
 * */
void *daemon_send_hb_func(Psocket_data pc_data)
{		
	char send_buf[200] = {0};
	data_struct hb;
	int sock_fd = -1, pc_sock, sinsize, send_size = 0;
	int *run_flag = (int *)&pc_data->pth_runflags;
	struct sockaddr_in my_addr, peer_addr; 

	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());

	//fill the info to the struct
	hb.type = HEARTBEAT;
	hb.auto_test = 1;
	hb.file_len = 0;
	hb.arg_len = 0;
	hb.file_name = NULL;
	hb.arg_data = NULL;
	hb.check_sum = daemon_calc_checksum((Pdata_struct)&hb); //calculate the check sum
	hb.len = (sizeof(hb.len))+(sizeof(hb.type))+(sizeof(hb.auto_test))+(sizeof(hb.file_len))
			+(sizeof(hb.arg_len))+(hb.file_len)+(hb.arg_len)+(sizeof(hb.check_sum));

	//copy to send buf
	daemon_cp_to_buf(send_buf, &hb);

	//init tcp struct
	memset(&peer_addr, 0, sizeof(struct sockaddr));
	memset(&my_addr, 0, sizeof(struct sockaddr));
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(TCP_HB_PORT); 
	my_addr.sin_addr.s_addr = INADDR_ANY;

	//create send heart beat socket
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd == -1)
	{
		anyka_print("[%s:%d] socket: %s, exit thread, Id: %ld\n", 
					__func__, __LINE__, strerror(errno), (long int)gettid());
		return NULL;
	}
	
	//set socket options
	sinsize = 1;
	if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &sinsize, sizeof(int)) != 0)
	{
		anyka_print("[%s:%d] setsockopt: %s\n", __func__, __LINE__, strerror(errno));
		goto close_sk;
	}
	//set close-on-exec flag
	if(fcntl(sock_fd, F_SETFD, FD_CLOEXEC) == -1)
	{
		anyka_print("[%s:%d] fcntl: %s\n", __func__, __LINE__, strerror(errno));
		goto close_sk;
	}
	 	
	/*** bind ***/
	if(bind(sock_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
	{
		anyka_print("[%s:%d] bind: %s.\n", __func__, __LINE__, strerror(errno));
		goto close_sk;
	}

	/**** listen ****/
	if(listen(sock_fd, 1) == -1)
	{		
		anyka_print("[%s:%d] listen: %s\n", __func__, __LINE__, strerror(errno));		
		goto close_sk;
	}

	/**** accept ****/	
	sinsize = sizeof(struct sockaddr);
	pc_sock = accept(sock_fd, (struct sockaddr *)&peer_addr, (socklen_t *)&sinsize);
	if(pc_sock == -1)
	{
		anyka_print("[%s:%d] accept: %s\n", __func__, __LINE__, strerror(errno)); 	
		goto close_sk;
	}
	anyka_print("[%s:%d] start send heart beat.\n", __func__, __LINE__);

	while (*run_flag)
	{
		send_size = send(pc_sock, send_buf, hb.len, 0); //send the heart beat package pointer by send_buf to PC
		if(send_size < 0)
		{
			anyka_print("[%s:%d] send: %s.\n", __func__, __LINE__, strerror(errno));
			break;
		}
		sleep(2);	// two sec interval
	}
	
close_sk:
	close(sock_fd);
  
	anyka_print("[%s:%d] Exit the send heart beat thread, id : %ld\n", __func__, __LINE__, (long int)gettid());
	return NULL;
}

/**
 * *  @brief       	 tcp pthread
 * *  @author      	 chen yanhong
 * *  @date       	 2014-10-28
 * *  @param[in]   monitor pthread id
 * *  @return        void *
 * */
void *daemon_tcp_pth(void *monitor_pid)
{
	int sock_fd;	//sock fd
	int sinsize;	
	int cmd_ret = 0;
	int recv_size;
	//int close_flags = 1;	//kill anyka_ipc and watch dog flag
	//pthread_t m_pid = *((pthread_t *)monitor_pid);	//get monitor pid
	pthread_t send_hb_pid;	//send heart beat pid

	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());

	//heart beat, pc socket fd;
	Psocket_data pc_communicate = (Psocket_data)malloc(sizeof(socket_data));		
	char *receive_buf = (char *)malloc(TCP_BUF_SIZE);
	struct sockaddr_in my_addr, peer_addr; 

	//clear the struct
	memset(&my_addr, 0, sizeof(struct sockaddr));
	memset(&peer_addr, 0, sizeof(struct sockaddr));

	//init tcp struct
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(TCP_PORT); 
	my_addr.sin_addr.s_addr = INADDR_ANY;

	while(1)
	{
		/*** create socket ***/
		sock_fd = socket(AF_INET, SOCK_STREAM, 0);
		if(sock_fd == -1)
		{
			anyka_print("[%s:%d] fail to create TCP socket.\n", __func__, __LINE__);
			continue;
		}
		anyka_print("[%s:%d] Success to create TCP socket.\n", __func__, __LINE__);

		sinsize = 1;
		if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &sinsize, sizeof(int)) != 0)
		{
			anyka_print("[%s:%d] set socket option failed,%s\n", __func__, __LINE__, strerror(errno));
			sleep(3);
			goto close_sk;
		}
		if(fcntl(sock_fd, F_SETFD, FD_CLOEXEC) == -1)
		{
			anyka_print("[%s:%d] error:%s\n", __func__, __LINE__, strerror(errno));
			sleep(3);
			goto close_sk;
		}
		 	
		
		/*** bind ***/
		if(bind(sock_fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
		{
			anyka_print("[%s:%d] bind socket failed, %s.\n", __func__, __LINE__, strerror(errno));
			sleep(3);
			goto close_sk;
		}
		//anyka_print("[%s:%d] Success to bind socket.\n", __func__, __LINE__);

		/**** listen ****/
		if(listen(sock_fd, 1) == -1)
		{		
			anyka_print("[%s:%d] listen failed\n", __func__, __LINE__);		
			goto close_sk;
		}
		//anyka_print("[%s:%d] Set Listen success.\n", __func__, __LINE__);		
		anyka_print("[%s:%d] Waiting for connect......\n", __func__, __LINE__);
		
		/**** accept ****/	
		sinsize = sizeof(struct sockaddr);
		pc_communicate->sock_fd = accept(sock_fd, (struct sockaddr *)&peer_addr, (socklen_t *)&sinsize);
		if(pc_communicate->sock_fd == -1)
		{
			anyka_print("[%s:%d] accept failed\n", __func__, __LINE__); 	
			goto close_sk;
		}
		else
		{
			anyka_print("[%s:%d] This machine has been connnected.\n", __func__, __LINE__);
			test_running = 1;	/* the test is running, make the keys lose efficacy */

			/* create pthread to send heart beat */
			pc_communicate->pth_runflags = 1;
			anyka_pthread_create(&send_hb_pid, (anyka_thread_main *)daemon_send_hb_func, 
						(void *)pc_communicate, ANYKA_THREAD_MIN_STACK_SIZE, -1);
#if 0
			if(close_flags)
			{	
				monitor_runflags = 0;
				system("killall -12 daemon");	//close watch dog
				system("killall -9 net_manage.sh");				
				system("killall -9 anyka_ipc cmd discovery");  //close cmd discovery script
				anyka_print("[%s:%d] Start Test Mode.\n", __func__, __LINE__);
				close_flags = 0;
				//sleep(1);
			}
#endif	
			while(1)
			{
				anyka_print("[%s:%d] ready to receive data.\n", __func__, __LINE__);
				/* receive data */
				recv_size = recv(pc_communicate->sock_fd, receive_buf, TCP_BUF_SIZE, 0);
				if(recv_size == -1)
				{
					anyka_print("[%s:%d] receive data error\n", __func__, __LINE__); 	
					break;
				}
				if(recv_size == 0)
				{
					anyka_print("[%s:%d] client out of line\n", __func__, __LINE__); 	
					break;
				}	
				
				/* analyse command, than execute it. */
				cmd_ret = daemon_parse_exec(receive_buf, pc_communicate->sock_fd);
				if(cmd_ret == -1)//after execute the cmd, return  error
				{
					anyka_print("[%s:%d] execute command failed.\n", __func__, __LINE__);
					if(daemon_response(pc_communicate->sock_fd, (-1)) < 0)
					{
						anyka_print("[%s:%d] send response failed.\n", __func__, __LINE__);
					}
				}
				else if(cmd_ret == 0)//pc socket close
				{
					if(daemon_response(pc_communicate->sock_fd, (0)))
					{
						anyka_print("[%s:%d] send response success.\n", __func__, __LINE__);
					}
				}
				else if(cmd_ret == 2)//it has receive a finish signal
				{
					daemon_response(pc_communicate->sock_fd, (0));
					close(sock_fd);
					anyka_print("[%s:%d] Restart device!\n", __func__, __LINE__);
					system("reboot");
					exit(0);	//no use actually, cause the system has been reboot
				}
				else//manual test
				{
					anyka_print("[%s:%d] Manual test mode, it has send report to PC\n", __func__, __LINE__);
				}
				memset(receive_buf, 0, sizeof(receive_buf));
			}

			pc_communicate->pth_runflags = 0;	//close heart beat pthread	
			pthread_join(send_hb_pid, NULL);		
		}	
		
close_sk:
		close(sock_fd);
        if(cmd_ret == 2)
        {			
			daemon_response(pc_communicate->sock_fd, (0));
            anyka_print("[%s:%d] Receive Finish Signal, reboot the device.\n", __func__, __LINE__);
			free(pc_communicate);
			free(receive_buf);
            system("reboot");
            exit(1);
        }
	}
	free(pc_communicate);
	free(receive_buf);
	
	anyka_print("[%s:%d] Exit the socket thread, id : %ld\n", __func__, __LINE__, (long int)gettid());
	return NULL;
}

#if 0
static void daemon_get_net_gateway(char *send_buf)
{
	char *p = NULL;
	char cmdbuf[64] = {0};
	char result[128] = {0};

	sprintf(cmdbuf, "%s", "netstat -r | awk '/default/{print $2}'");
	do_syscmd(cmdbuf, result);

	if((p = strchr(result, '.')) != NULL){
		strncpy(send_buf, result, strlen(result));
		strcat(send_buf, "@");
	} else {
		sprintf(send_buf, "%s", "0.0.0.0");
	}
}
#endif


/**
 * *  @brief		daemon_get_machine_ip
 				获取机器当前使用的网卡的IP，子网掩码和网关
 * *  @author      	
 * *  @date       	 2014-10-28
 * *  @param[in]   char *send_buf， 存放ip等信息的bug 指针
 * *  @return        void
 * */
static void daemon_get_machine_ip (char *send_buf)
{
	char *p = NULL;
	char *q = NULL;
	char cmdbuf[64] = {0};
	char result[128] = {0};
	char gateway[32] = {0};

	/********* check current net status **********/
	sprintf(cmdbuf, "ifconfig eth0 | grep RUNNING");
	do_syscmd(cmdbuf, result);
	bzero(cmdbuf, sizeof(cmdbuf));

	if (strlen(result) > 0)
		sprintf(cmdbuf, "ifconfig eth0 | grep inet");
	else
		sprintf(cmdbuf, "ifconfig wlan0 | grep inet");
	bzero(result, sizeof(result));
	do_syscmd(cmdbuf, result);

	/********* get ip  ********/
	p = strchr(result, ':');
	if (p) {
		p += 1;
		q = strchr(p, ' ');
		if (q) {
			strncpy(send_buf, p, q - p);
			strncpy(gateway, p, q - p);
			strcat(send_buf, "@");
			//anyka_print("[%s:%d] get ip : [%s]\n", __func__, __LINE__, send_buf);
			send_buf += strlen(send_buf);
		}

		/******* get net mask *******/
		p = strrchr(p, ':');
		if (p) {		
			p += 1;
			q = strchr(p, '\n');
			if (q) {
				strncpy(send_buf, p, q - p);
				strcat(send_buf, "@");
				//anyka_print("[%s:%d] get net mask :[%s]\n", __func__, __LINE__, send_buf);
				send_buf += strlen(send_buf);
			}
		}

		/******* get net gateway *******/
		p = strrchr(gateway, '.');
		if (p) {
			bzero(result, sizeof(result));
			strncpy(result, gateway, p - gateway);

			bzero(gateway, sizeof(gateway));
			sprintf(gateway, "%s.%s", result, "1");
		}
		strncat(send_buf, gateway, strlen(gateway));
		strcat(send_buf, "@");
	} else {
		strcat(send_buf, "0.0.0.0@0.0.0.0@0.0.0.0@");	
		anyka_print("[%s:%d] no ip/netmask/gateway !!!", __func__, __LINE__);
	}
}



/**
 * *  @brief		daemon_get_net_dns
 				获取系统DNS  信息
 * *  @author      	
 * *  @date       	 2014-10-28
 * *  @param[in]   char *dns_buf， 存放dns信息的bug 指针
 * *  @return        NULL 
 * */

static void daemon_get_net_dns(char *dns_buf)
{
	char cmd[32] = {0};
	char result[128] = {0};
	char tmp_dns[32] = {0};
	char *p = NULL;
	int i, j;

	/*** to get dns message from system  ***/
	sprintf(cmd, "cat /etc/resolv.conf");

	/*** execute the command, the return message store in "result" ***/
	do_syscmd(cmd, result);		
	p = result;

	/** cause the dns message format is : e.g. nameserver 192.168.xx.xx 
	**   so we parse it as below, usually this msg has two tips, so we parse twice
	**/
	for(i = 0; i < 2; i++)
	{
		/** find the lable:nameserver at first **/
		p = strstr(p, "nameserver");
		
		/* If  the lable was found, do next */
		if(p)	
		{
			j = 0;
			while(!isdigit(p[j]))	//skip no-number
				j++;
			p += j;

			if(p)	//find the first number
			{
				j = 0;
				/** get the address by loop,  skip no-number and '.' **/
				while(1)	
				{
					if(isdigit(p[j]))
						j++;
					else if(p[j] == '.')
						j++;
					else
					{
						strncpy(dns_buf, p, j);		//copy the address to the buf
						strcat(dns_buf, "@");		//add division lable
						strncpy(tmp_dns, dns_buf, strlen(dns_buf));		//store the first dns
						dns_buf += strlen(dns_buf);
						p += j;
						break;
					}					
				}
			}
			else
				break;
		}
		else if(i == 1) //only one dns message, use the same
		{
			strncpy(dns_buf, tmp_dns, strlen(tmp_dns));
			dns_buf += strlen(dns_buf);			
			break;
		} else {	//no dns message
			sprintf(dns_buf, "0.0.0.0@0.0.0.0@");
			anyka_print("[%s:%d] It dose not get DNS.\n", __func__, __LINE__);
			break;
		}
	}
	return;
}
 

/**
 * *  @brief    daemon_get_local_info
				接收到特定广播包信息后，通过本函数获取一些系统上的信息。
* *  @author      	
* *  @date       	 2014-10-28
* *  @param[in]   char * send_buf，存放系统需要发送的系统信息的buf 指针
* *  @return        void 
* */
static void daemon_get_local_info(char * send_buf)
{
	char *p = send_buf;
	char ch_name[50] = {0};
	char dhcp[4] = {0};

 	/*** open the config file to get the handle ***/
	void * camera_info = anyka_config_init("/etc/jffs2/anyka_cfg.ini");
	/** get camera osd name message and ethernet dhcp status respectively **/
	anyka_config_get_title(camera_info, "camera", "osd_name", ch_name, "\xe5\x8a\x9e\xe5\x85\xac\xe5\xae\xa4");
	anyka_config_get_title(camera_info, "ethernet", "dhcp", dhcp, "0");

	/*************** get channel name *****************/
	strncpy(p, ch_name, strlen(ch_name));
	strcat(p, "@");	//add segmentation
	p += strlen(p);

	/** After store the message, we close the config file  **/
	anyka_config_destroy("anyka_cfg.ini", camera_info);
	
	/*********  get net ip & netmask & gateway ***********/
	daemon_get_machine_ip(p);
	p += strlen(p);

	/*********  get net gateway  ***********/	
	//daemon_get_net_gateway(p);
	//p += strlen(p);

	/*********  get net  dns ***********/
	daemon_get_net_dns(p);
	p += strlen(p);

	/*********  get net dhcp status ***********/	
	strncpy(p, dhcp, strlen(dhcp));

	return;
}


/**
 * *  @brief       	 daemon_broadcast_pth
 				接受广播包线程，对信息进行处理并返回给上位机
 * *  @author      	
 * *  @date       	 2014-10-28
 * *  @param[in]   void
 * *  @return        NULL 
 * */

static void *daemon_broadcast_pth(void)
{
	int udp_fd;	//sock fd
	int on = 1;
	char rec_buf[128] = {0};
	char snd_buf[256] = {0};
	char * flgs = "Anyka IPC ,Get IP Address!";

	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());
	
	socklen_t sockaddr_len;
	sockaddr_len = sizeof(struct sockaddr);
	
	struct sockaddr_in udp_addr, server_addr; 
	memset(&udp_addr, 0, sizeof(struct sockaddr));
	memset(&server_addr, 0, sizeof(struct sockaddr));

	/*** set socket attribute ***/
	udp_addr.sin_family = AF_INET;			/** IPV4 **/
	udp_addr.sin_port = htons(UPD_PORT); 	/** port 8192 **/
	udp_addr.sin_addr.s_addr = INADDR_ANY;	/** any ip  **/

	while (1) {
		/** create socket  **/
		if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
			perror("udp socket");
			anyka_print("[%s:%d] Create broadcast socket failed.\n", __func__, __LINE__);
			usleep(500);
			continue;
		} else {
			anyka_print("[%s:%d] create udp socket success, socket fd:%d.\n", __func__, __LINE__, udp_fd);
		}

		/** set socket options **/
		if (setsockopt(udp_fd ,SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &on, sizeof(on)) != 0) {
			perror("setsockopt");
			close(udp_fd);
			usleep(500);
			continue;
		}

		/*** set close-on-exec flag ***/
		if (fcntl(udp_fd, F_SETFD, FD_CLOEXEC) == -1) {
			anyka_print("[%s:%d] error:%s\n", __func__, __LINE__, strerror(errno));
			close(udp_fd);
			usleep(500);
			continue;
		}

		/** bind the socket  **/
		if (bind(udp_fd, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) {
			perror("bind udp");
			close(udp_fd);
			sleep(3);
			continue;			
		}
		anyka_print("[%s:%d] ready to receive broadcast\n", __func__, __LINE__);
		
		while (1) {
			/*** receive the broadcast package blocking ***/
			if (recvfrom(udp_fd, rec_buf, sizeof(rec_buf), 0, 
				(struct sockaddr *)&server_addr, &sockaddr_len) == -1) {
				anyka_print("[%s:%d] recv broadcast failed\n", __func__, __LINE__);				
				perror("receive date failed\n");
				continue;
			} else {
				rec_buf[127] = 0;			
				anyka_print("[%s:%d] recv PC broadcast: %s\n", __func__, __LINE__, rec_buf);

				/** compare the flag to make sure that the broadcast package was sent from our software **/
				if (strncmp(rec_buf, flgs, strlen(flgs)) != 0) {
					anyka_print("[%s:%d] recv data is not match\n", __func__, __LINE__);
					bzero(rec_buf, sizeof(rec_buf));
					continue;
				}
				/*** get the machine info fill into the snd_buf ***/
				daemon_get_local_info(snd_buf);		

				/** send the message back to the PC **/
				if(sendto(udp_fd, snd_buf, strlen(snd_buf), 0, 
					(struct sockaddr *)&server_addr, sockaddr_len) == -1) {
					anyka_print("[%s:%d] send broadcast failed, %s\n", __func__, __LINE__, strerror(errno));				
				}
				anyka_print("[%s:%d] Send Broadcast Success, info: [%s]\n", __func__, __LINE__, snd_buf);				
				bzero(snd_buf, sizeof(snd_buf));
				bzero(rec_buf, sizeof(rec_buf));
			}
		}

		/** finish, close the socket **/
		close(udp_fd);
		break;
	}
	
	anyka_print("[%s:%d] Exit thread, id : %ld\n", __func__, __LINE__, (long int)gettid());
	return NULL;
}



/**
 * *  @brief       	 signal process
 * *  @author      	
 * *  @date       	 2014-10-28
 * *  @param[in]   signal id
 * *  @return        void 
 * */
static void sigprocess(int sig)
{
	// Fix me ! 20140828 ! Bug Bug Bug Bug Bug
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

	if(sig == SIGINT || sig == SIGSEGV)
	{
		
	}

	if(sig == SIGTERM)
	{
	}
	exit(1);
}

/**
 * *  @brief       program's access point
 * *  @author      gao wangsheng
 * *  @date        2012-10-15
 * *  @param[in]   not use
 * *  @return      0 on success
 * */
/****	modify by chen yanhong ,2014-09-17****/
 /*	main program has became a daemon program before daemon_init operate	*/
int main (int argc, char **argv)
{	
	int ret = 0;
	int gpio_fd;
	pthread_t pth;
	pthread_t monitor_pth_id;
	pthread_t tcp_sock_id;
	pthread_t broadcast_sk_id;
	fd_set readfds, tempfds;

	daemon_init();
	
	signal(SIGUSR1, daemon_sig_fun);		//now no use
	signal(SIGALRM, daemon_alarm_print);	//for timer
	signal(SIGSEGV, sigprocess);	//for debug
	signal(SIGINT, sigprocess);		//for debug
	signal(SIGTERM, sigprocess);	//for debug
	signal(SIGPIPE, SIG_IGN);
	/*** register signal for close watch dog and close monitor anyka_ipc	***/
	signal(SIGUSR2, daemon_close_watch_dog);
	/************************ daemon pthread *************************/
	monitor_runflags = 1;
	anyka_pthread_create(&monitor_pth_id, (anyka_thread_main *)daemon_monitor,
				NULL, ANYKA_THREAD_MIN_STACK_SIZE, -1);
	anyka_print("***************************************\n");
	anyka_print("*****A monitor daemon has running!*****\n");
	anyka_print("***************************************\n");
	
	/*************************  TCP/IP work thread  **********************/
	anyka_pthread_create(&tcp_sock_id, (anyka_thread_main *)daemon_tcp_pth, 
				(void *)&monitor_pth_id, ANYKA_THREAD_NORMAL_STACK_SIZE, -1);
	
	/*************************  BROADCAST  thread  **********************/
	/*
	**接收PC端广播，返回当前机器的信息
	*/
	anyka_pthread_create(&broadcast_sk_id, (anyka_thread_main *)daemon_broadcast_pth,
				(void *)NULL, ANYKA_THREAD_MIN_STACK_SIZE, -1);
	
/**************** watch_dog *******************/
#ifdef WATCHDOG_ENABLE
	daemon_watchdog_enable();		//enable watchdog

	
#else
	signal(SIGUSR2, SIG_IGN);
#endif
	
	FD_ZERO(&readfds);		// clean the fd_set readfds

	/** open gpio key device **/
	if ((gpio_fd = open(KEY_GPIO_DEV, O_RDONLY)) < 0) {
		perror("Open gpio key dev fail");
		return -ENOENT;
	}
	FD_SET(gpio_fd, &readfds);		//add the gpio_fd to the readfds set

	fcntl(gpio_fd, F_SETFD, FD_CLOEXEC);	//set the gpio_fd's close-on-exec attribute

	/*** 检测热插拔状态并做相应处理的线程 ***/
	pthread_create(&pth, NULL, daemon_pth_func, NULL);

	/*** 检测按键循环 ***/
	while (1) {
		int rd = 0;
		struct input_event key_event[64];
		
		tempfds = readfds;
		/**** monitor the tempfds no-blocking ****/
		ret = select(FD_SETSIZE, &tempfds, (fd_set *)0, (fd_set *)0, (struct timeval*)0);	
		if (ret < 1) {
			perror("select");
			//exit(2);
		}

		/** To test whether the gpio_fd's status has changed **/
		if (FD_ISSET(gpio_fd, &tempfds)) {
			/** read the event to the buf **/
			rd = read(gpio_fd, key_event, sizeof(struct input_event) * sizeof(key_event));
			/*** parse the event ***/
			daemon_do_key(key_event, rd);
		}
	}
	
	/** 停止热插拔检测线程 **/
	pthread_cancel(pth);
	/*** close the key device  ***/
	close(gpio_fd);

	return ret;
}
