#include "common.h"
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>


#define FIFO_LOCATION "/tmp/daemon_fifo"

static int monitor_runflag = 0;	//run flag



/**
* @brief 	 monitor
* 			monitor主线程，和守护进程通信，建立心跳包等
* @date 	2015/3
* @param:	void
* @return 	NULL
* @retval 	
*/

void * monitor(void)
{
	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());
	int snd_fd, wt_ret;

	char i = 0;
	char *msg = "being alives";
	char info_msg[20];
	
	signal(SIGPIPE, SIG_IGN);	//register signal handler
	monitor_runflag = 1;
	/********** make fifo **********/	
	if(access(FIFO_LOCATION, F_OK) < 0){		//check the fifo 
		mkfifo(FIFO_LOCATION, 0777);			//not exist, create it 
	}else{
		anyka_print("the daemon fifo has created\n");
	}
	
	snd_fd = open(FIFO_LOCATION, O_WRONLY);		//open the fifo
	
	fcntl(snd_fd, F_SETFD, FD_CLOEXEC);			//set attribute 

	/** send heart beat package loop **/
	while(monitor_runflag)
	{	
		sprintf(info_msg, "%s%d", msg, i++);		//write specific val 	
											
		wt_ret = write(snd_fd, info_msg, strlen(info_msg));	//do write 
		if(wt_ret < 0)
		{
			anyka_print("write data failed,%s\n", strerror(errno));	//error handler
			break;
		}
		if(i > 128)
			i = 0;
																		
		sleep(2);		//make a interval
	}

	close(snd_fd);	//close the fifo when exit

	return NULL;
}








































