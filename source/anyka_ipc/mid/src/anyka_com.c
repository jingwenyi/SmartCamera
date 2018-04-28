#include <syslog.h>
#include <stdarg.h>
#include "common.h"
#include "anyka_com.h"
#include <sys/syscall.h>

#define ANYKA_IPC_START "/tmp/anyka_ipc_run"

/*
*  color define 
*/
#define NONE          "\033[m"			//close
#define NONE_N        "\033[m\n"		//close and new_line
#define RED           "\033[0;32;31m"	//red
#define LIGHT_RED     "\033[1;31m"		//red high light
#define GREEN         "\033[0;32;32m"	//green
#define LIGHT_GREEN   "\033[1;32m"		//green high light


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
 * *  @brief       	anyka_pthread_mutex_destroy
 				销毁线程锁
 * *  @author      	aj
 * *  @date       	2014-10-28
 * *  @param[in]  pthread_mutex_t *pmutex
				要销毁的线程锁
 * *  @return        void
 * */

void anyka_pthread_mutex_destroy(pthread_mutex_t *pmutex)
{
    
	int rc = pthread_mutex_destroy(pmutex); 
    if (rc == EBUSY) {                      
        pthread_mutex_unlock(pmutex);             
        pthread_mutex_destroy(pmutex);    
    } 
    
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
	/** init sched param attribute **/
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
 * NAME         anyka_malloc_stream_ram
 * @BRIEF	 分配流媒体内存
 
 * @PARAM	
                    
                   
 
 * @RETURN	void
 * @RETVAL	
 */

T_STM_STRUCT * anyka_malloc_stream_ram(int size)
{
    T_STM_STRUCT *pStream;
    
    pStream = malloc(sizeof(T_STM_STRUCT));
    if(pStream)
    {
        pStream->buf = malloc(size);
        if(pStream->buf == NULL)
        {
            free(pStream);
            return NULL;
        }
    }

    return pStream;
}


/**
 * NAME         anyka_free_stream_ram
 * @BRIEF	释放流媒体内存
 
 * @PARAM	
                    
                   
 
 * @RETURN	void
 * @RETVAL	
 */

void anyka_free_stream_ram(void *pStream)
{
    T_STM_STRUCT *tmp = (T_STM_STRUCT*)pStream;

    if(pStream)
    {
        free(tmp->buf);
        free(tmp);
    }
}


/**
 * *  @brief       	anyka_print
 * *  @author      	aj
 * *  @date       	2014-10-28
 * *  @param[in] 	char * fmt, 可变参数
 * *  @return       void
 * */

#if 1
#define LOG_BUF_MAX 512

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

T_VOID anyka_debug(char* fmt, ...)
{
    T_CHR buf[LOG_BUF_MAX];
    va_list ap;
	system_user_info * set_info = anyka_get_system_user_info();

    if (set_info->debuglog == 0)
        return;
        
    memset(buf, 0, sizeof(buf));
    va_start(ap, fmt);
    vsnprintf(buf, LOG_BUF_MAX, fmt, ap);
    if(strlen(buf) >= LOG_BUF_MAX)
    {
        printf("*****************buf overflow**********************\n");
    }
    buf[LOG_BUF_MAX - 1] = 0;
    va_end(ap);
    syslog(LOG_INFO, "%s", buf); 
    printf(LIGHT_GREEN"%s"NONE, buf);		//green color print
	fflush(stdout);
    fflush(stderr);
}

void anyka_err(char* fmt, ...)
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
    syslog(LOG_ERR, "%s", buf); 
    printf(LIGHT_RED"%s"NONE, buf);		//red color print
    fflush(stdout);
    fflush(stderr);
}



/**
 * *  @brief       	do_syscmd
 				执行系统命令
 * *  @author      	aj
 * *  @date       	2014-10-28
 * *  @param[in] 	char * cmd, 要执行的命令
 				char *result， 执行结果
 * *  @return       int
 * *  @retval		lengh of result buf --> success, less than zero --> failed
 * */

int do_syscmd(char *cmd, char *result)
{
	char buf[4096];	
	FILE *filp;
	
	filp = popen(cmd, "r");
	if (NULL == filp){
		anyka_print("[%s:%d] popen %s, cmd:%s!\n", 
			__func__, __LINE__, strerror(errno), cmd);
		return -2;
	}

	//fgets(buf, sizeof(buf)-1, filp);
	memset(buf, '\0', sizeof(buf));
	fread(buf, sizeof(char), sizeof(buf)-1, filp);
	
	sprintf(result, "%s", buf);
	
	pclose(filp);
	return strlen(result);	
}

static int ipc_runflag = 0;
/**
 * *  @brief    anyka_check_first_run
 				check  whether the anyka_ipc started
 * *  @author      	
 * *  @date       	
 * *  @param[in] 	void
 * *  @return       int
 * *  @retval		if anyka_ipc has run at once, return 0, others return 1
 * */

int anyka_check_first_run(void)
{
	char cmd[100] = {0};

	if(ipc_runflag == 1)
		return 1;

	if(access(ANYKA_IPC_START, F_OK) == 0)
		return 0;

	sprintf(cmd, "touch %s", ANYKA_IPC_START);
	system(cmd);
	
	ipc_runflag = 1;

	return 1;
}

#endif

