#ifndef _DAEMON_INC_H_
#define _DAEMON_INC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <pthread.h>
#include <linux/netlink.h>
#include <linux/version.h>
#include <linux/input.h>
#include <syslog.h>
#include <stdarg.h>
#include <execinfo.h>
#include <errno.h>
#include <sys/vfs.h>
/************** for tcp ********************/
#include <sys/types.h>			/* See NOTES */ 	 
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
/********** modify 2014-9-16 ************/

/***************************************/
#include "daemon_init.h"
#include "anyka_ini.h"
#include "rtc.h"


#define ANYKA_THREAD_PRIORITY   50
#define ANYKA_THREAD_NORMAL_STACK_SIZE (200*1024)
#define ANYKA_THREAD_MIN_STACK_SIZE (100*1024)


/*************  TCP/IP TEST  DATA STRUCT **************/
enum
{
	COMMAND = 1,
	RESPONSE,
	HEARTBEAT,
	FINISH,
};

/***** tcp communite data struct  *****/
typedef struct data_struct
{
	unsigned short len;
	unsigned short arg_len;
	unsigned short file_len;
	unsigned short check_sum;
	unsigned char type;
	unsigned char auto_test;
	char *file_name;
	char *arg_data;
}data_struct, *Pdata_struct;

typedef struct socket_data
{
	int sock_fd;
	int pth_runflags;
}socket_data, *Psocket_data;


typedef    char                    T_CHR;        /* char */
typedef    void                    T_VOID;        /* void */

typedef void* anyka_thread_main(void* param);


/**
 * *  @brief       	anyka_pthread_create
 * *  @author      	aj
 * *  @date       	2014-10-28
 * *  @param[in]  pthread_t *thread_id, thread id
 				anyka_thread_main *func, thread function
 				void *arg , argument for thread 
 				int stack_size, thread stack size
 				int priority, priority default -1
 * *  @return        0, success; -1 failed
 * */

int anyka_pthread_create(pthread_t *thread_id, anyka_thread_main *func, void *arg, int stack_size, int priority);


/**
 * *  @brief       	anyka_print
 				声明的同时使用attribute 校验参数使其参数错误时编译给出警告
 * *  @author      	aj
 * *  @date       	2014-10-28
 * *  @param[in] 	char * fmt, 可变参数
 * *  @return       void
 * */

extern T_VOID anyka_print(char* fmt, ...)__attribute__((format(printf,1,2)));

/**
 * *  @brief       	 gettid
 * *  @author      	 chen yanhong
 * *  @date       	 2014-10-28
 * *  @param[in]   void 
 * *  @return        pid_t, return current thread id
 * */

pid_t gettid();

#endif
