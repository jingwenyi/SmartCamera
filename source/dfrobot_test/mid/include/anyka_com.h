#ifndef _anyka_com_h_
#define _anyka_com_h_

#include "video_ctrl.h"

#ifdef __cplusplus  //jwy add 2016.5.31
extern "C" {
#endif


#define ANYKA_THREAD_PRIORITY   50
#define ANYKA_THREAD_NORMAL_STACK_SIZE (200*1024)
#define ANYKA_THREAD_MIN_STACK_SIZE (100*1024)
void anyka_pthread_mutex_destroy(pthread_mutex_t *pmutex);

typedef void* anyka_thread_main(void* param);
int anyka_pthread_create(pthread_t *thread_id, anyka_thread_main *func, void *arg, int stack_size, int priority);

T_VOID anyka_print(char* fmt, ...)__attribute__((format(printf,1,2)));

T_VOID anyka_debug(char* fmt, ...)__attribute__((format(printf,1,2)));

T_VOID anyka_err(char* fmt, ...)__attribute__((format(printf,1,2)));

int do_syscmd(char *cmd, char *result);

pid_t gettid();	//get pthread id function, should include the <sys/syscall.h> head file

/*
**  check whether the ipc run at once
*/
int anyka_check_first_run(void);


/**
 * NAME         anyka_free_stream_ram
 * @BRIEF	释放流媒体内存
 
 * @PARAM	
                    
                   
 
 * @RETURN	void
 * @RETVAL	
 */

void anyka_free_stream_ram(void *pStream);

/**
 * NAME         anyka_malloc_stream_ram
 * @BRIEF	 分配流媒体内存
 
 * @PARAM	
                    
                   
 
 * @RETURN	void
 * @RETVAL	
 */

T_STM_STRUCT * anyka_malloc_stream_ram(int size);

#ifdef __cplusplus  //jwy add 2016.05.31
} /* end extern "C" */
#endif


#endif

