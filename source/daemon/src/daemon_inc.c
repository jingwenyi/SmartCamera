#include "daemon_inc.h"
#include <sys/syscall.h>

#define LOG_BUF_MAX 512

/**
 * *  @brief       	 gettid
 * *  @author      	 chen yanhong
 * *  @date       	 2014-10-28
 * *  @param[in]   void 
 * *  @return        pid_t, return current thread id
 * */
pid_t gettid()
{
	return syscall(SYS_gettid);
}

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

int anyka_pthread_create(pthread_t *thread_id, anyka_thread_main *func, void *arg, int stack_size, int priority)
{
    
    int ret;
    pthread_attr_t SchedAttr;
    struct sched_param	SchedParam;

#ifdef _POSIX_THREAD_PRIORITY_SCHEDULING
    //anyka_print("pthread priority(SCHED_RR)=(min=%d,max=%d)\n", sched_get_priority_min(SCHED_RR), sched_get_priority_max(SCHED_RR));
    //anyka_print("pthread priority(SCHED_FIFO)=(min=%d,max=%d)\n", sched_get_priority_min(SCHED_FIFO), sched_get_priority_max(SCHED_FIFO));
#endif
    /** init sched param attribute  **/
    pthread_attr_init(&SchedAttr);		
        
    if(priority != -1)
    {
        ret = pthread_attr_setinheritsched(&SchedAttr, PTHREAD_EXPLICIT_SCHED);
        if (ret != 0)
             anyka_print("pthread_attr_setschedpolicy, %s\n", strerror(ret));       
        ret = pthread_attr_setschedpolicy(&SchedAttr, SCHED_RR);
        if (ret != 0)
            anyka_print("pthread_attr_setschedpolicy, %s\n", strerror(ret));
        SchedParam.sched_priority = priority;	
        ret = pthread_attr_setschedparam(&SchedAttr, &SchedParam);
        if (ret != 0)
            anyka_print("pthread_attr_setschedparam, %s\n", strerror(ret));
    }
    
    ret = pthread_attr_setstacksize(&SchedAttr, stack_size);
    if (ret != 0)
        anyka_print("pthread_attr_setstacksize, %s\n", strerror(ret));
    ret = pthread_create(thread_id, &SchedAttr, func, arg);
	if(ret != 0)
		anyka_print("pthread_create [ %x ] failed, %s\n", (unsigned int)*func, strerror(ret));

    pthread_attr_destroy(&SchedAttr);

    return ret;
    
}


/**
 * *  @brief       	anyka_print
 * *  @author      	aj
 * *  @date       	2014-10-28
 * *  @param[in] 	char * fmt, 可变参数
 * *  @return       void
 * */

T_VOID anyka_print(char* fmt, ...)
{
	T_CHR buf[LOG_BUF_MAX];
	va_list ap;

	memset(buf, 0, sizeof(buf));
	va_start(ap, fmt);

	vsnprintf(buf, LOG_BUF_MAX, fmt, ap);
	if(strlen(buf) >= LOG_BUF_MAX)
	{
		printf("*****************buf overflow**********************\n");
	}

	buf[LOG_BUF_MAX - 1] = 0;
  	va_end(ap);

	syslog(LOG_DEBUG,"%s", buf);
	printf("%s", buf);
											    
}












































