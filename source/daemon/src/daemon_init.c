
/****   daemon_init   ****/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>



/**
* @brief 	daemon_init
* 			将当前进程设置为守护进程接口
* @date 	2015/3
* @param:	void
* @return 	int
* @retval 	0
*/

int daemon_init(void)
{
	int a, max_fd, i;

	/** 1 **/
	signal(SIGHUP, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);	

	/*** 2 ***/
	a = fork();
	if(a > 0)
		exit(0);

	/*** 3 ***/
	setsid();

	/*** 4 ***/
	a = fork();
	if(a > 0)
		exit(0);

	/*** 5 ***/
	setpgrp();

	/*** 6 ***/
	max_fd = sysconf(_SC_OPEN_MAX);
	for(i = 3; i < max_fd; i++) 
		close(i);

	/*** 7 ***/
	umask(0);

	/*** 8 ***/
	chdir("/");
	
	return 0;
	
}
