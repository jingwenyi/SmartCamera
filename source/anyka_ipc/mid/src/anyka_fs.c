/************************************************************
目前发现，我们在合成录像文件的时候，会有读的操作，
读是要所有数据写完之后再能正常工作，目前还是设计为
将所有的写的数据放到一个队列中，专门用一个线程来写数据
*************************************************************/


#include "common.h"
#include "anyka_fs.h"


typedef struct _anyka_fs_file_point_info
{
    int file_id;
    int file_size;
    struct _anyka_fs_file_point_info *next;
}anyka_fs_file_point_info, *Panyka_fs_file_point_info;

typedef struct _anyka_fs_asyn_info
{
    uint8 run_flag;
    uint8 run_times;
    uint8 write_err;
    PA_HTHREAD  id;
    sem_t   sem_write;
    pthread_mutex_t fs_lock;
    void *asyn_write_queue;
    Panyka_fs_file_point_info file_list;
}anyka_fs_asyn_info, *Panyka_fs_asyn_info;

static Panyka_fs_asyn_info panyka_asyn = NULL;


typedef struct _anyka_fs_asyn_file_info
{
    int file_id;
    int size;
    void *buf;
}anyka_fs_asyn_file_info, *Panyka_fs_asyn_file_info;
#if 0
struct aiocb* anyka_fs_asyn_create_ram(int size)
{
    struct aiocb* paio_info;

    paio_info = (struct aiocb*)malloc(sizeof(struct aiocb));
    if(paio_info == NULL)
    {
        anyka_print("[%s:%d]:it fails to malloc ram", __func__, __LINE__);
        return NULL;
    }
    memset(paio_info, 0, sizeof(struct aiocb));
    paio_info->aio_buf = malloc(size + 4);
    if(paio_info->aio_buf == NULL)
    {
        anyka_print("[%s:%d]:it fails to malloc ram", __func__,__LINE__);
        free(paio_info);
        return NULL;
    }
    return paio_info;
}
void anyka_fs_asyn_free(void *item)
{
    
    struct aiocb* paio_info = (struct aiocb*)item;

    if(paio_info)
    {
        free(paio_info->aio_buf);
        free(paio_info);
    }
}
#else
Panyka_fs_asyn_file_info anyka_fs_asyn_create_ram(int size)
{
    Panyka_fs_asyn_file_info paio_info;

    paio_info = (Panyka_fs_asyn_file_info)malloc(sizeof(anyka_fs_asyn_file_info));
    if(paio_info == NULL)
    {
        anyka_print("[%s:%d] it fails to malloc ram", __func__, __LINE__);
        return NULL;
    }
    memset(paio_info, 0, sizeof(anyka_fs_asyn_file_info));
    paio_info->buf= malloc(size + 4);
    if(paio_info->buf== NULL)
    {
        anyka_print("[%s:%d] it fails to malloc ram", __func__,__LINE__);
        free(paio_info);
        return NULL;
    }
    return paio_info;
}
void anyka_fs_asyn_free(void *item)
{
    
    Panyka_fs_asyn_file_info paio_info = (Panyka_fs_asyn_file_info)item;

    if(paio_info)
    {
        free(paio_info->buf);
        free(paio_info);
    }
}
#endif


/**
 * NAME         anyka_fs_asyn_write_main
 * @BRIEF	系统写文件的主线程
 * @PARAM	arg 用户数据
                   
 
 * @RETURN	void *
 * @RETVAL	
 */


void* anyka_fs_asyn_write_main(void *arg)
{
	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());

	int write_size, cur_size;
    unsigned long tick = 0;
    //system_user_info * set_info = anyka_get_system_user_info();
    
    while(panyka_asyn->run_flag)
    {
        sem_wait(&panyka_asyn->sem_write);
        while(anyka_queue_not_empty(panyka_asyn->asyn_write_queue))
        {
            Panyka_fs_asyn_file_info paio_info;
            
            pthread_mutex_lock(&panyka_asyn->fs_lock);
            paio_info = (Panyka_fs_asyn_file_info) anyka_queue_pop(panyka_asyn->asyn_write_queue);
            pthread_mutex_unlock(&panyka_asyn->fs_lock);
            //aio_suspend( paio_info, 1, NULL );     
            write_size = 0;
            cur_size = paio_info->size;
            while(cur_size)
            {
                if(sd_get_status())
                {
                    struct timeval tvs;
                	//if(set_info->debuglog)
                    {
                        gettimeofday(&tvs, NULL);
                        tick = (tvs.tv_sec * 1000) + (tvs.tv_usec / 1000);        
                    }
                    
                    write_size = write(paio_info->file_id, paio_info->buf, cur_size);

                	//if(set_info->debuglog)
                    {
                        gettimeofday(&tvs, NULL);
                        if ((tvs.tv_sec * 1000) + (tvs.tv_usec / 1000) > tick)
                            tick = (tvs.tv_sec * 1000) + (tvs.tv_usec / 1000) - tick;
                        else
                            tick = 0; //no use
                        if (tick > 300)
                            anyka_print("fs_async_write too long:%ldms, queue size=%d\n", 
                            	tick, anyka_queue_items(panyka_asyn->asyn_write_queue));
                    }
                }
                else
                {
                    write_size = cur_size;
                }
                if(write_size <= 0)
                {
                    panyka_asyn->write_err++;
                    anyka_print("fs write error:ret=%d, errno=%d, tick=%ld, write_err=%d\n", 
						write_size, errno, tick, panyka_asyn->write_err);
                }
                cur_size -= write_size;
            }
            anyka_fs_asyn_free(paio_info);            
        }
    }

	return NULL;
}

/**
 * NAME         anyka_fs_init
 * @BRIEF	创建异步写的句柄
 * @PARAM	void
**/                   
void anyka_fs_init(void)
{
    int ret;
    
    panyka_asyn = (Panyka_fs_asyn_info)malloc(sizeof(anyka_fs_asyn_info));
    if(panyka_asyn == NULL)
    {
        anyka_print("[%s:%d] it fail to malloc ram!\n", __func__, __LINE__);
        return;
    }
    memset(panyka_asyn, 0, sizeof(anyka_fs_asyn_info));
    panyka_asyn->run_flag = 1;
    panyka_asyn->write_err = 0;
    if(sem_init(&panyka_asyn->sem_write, 0, 0))
    {
        anyka_print("[%s:%d] it fail to create sem!\n", __func__, __LINE__);
		free(panyka_asyn);
	    panyka_asyn = NULL;
        return;		
    }
    
    pthread_mutex_init(&panyka_asyn->fs_lock, NULL); 
    panyka_asyn->asyn_write_queue = anyka_queue_init(500); //该队列如果过小的话容易出现异步写缓冲溢出
    if(panyka_asyn->asyn_write_queue == NULL)
    {
        anyka_print("[%s:%d] it fail to create queue!\n", __func__, __LINE__);
        goto ERR_anyka_fs_crete;
    }
    ret = anyka_pthread_create(&(panyka_asyn->id), anyka_fs_asyn_write_main,
		   	NULL, ANYKA_THREAD_NORMAL_STACK_SIZE, -1);
    if((ret) != 0 ) 
    {
        anyka_print( "[%s:%d] create thread anyka_fs_asyn_write_main failed, ret=%d!\n", 
				__func__, __LINE__, ret);
        goto ERR_anyka_fs_crete;
    }

    panyka_asyn->file_list = NULL;
    return;

 ERR_anyka_fs_crete:
    sem_destroy(&panyka_asyn->sem_write);
	pthread_mutex_destroy(&panyka_asyn->fs_lock);
    if(panyka_asyn->asyn_write_queue)
    {
        anyka_queue_destroy(panyka_asyn, anyka_fs_asyn_free);
    }
    free(panyka_asyn);
    panyka_asyn = NULL;
}


/**
 * NAME         anyka_fs_reset
 * @BRIEF	创建异步写的句柄
 * @PARAM	void
**/                   

void anyka_fs_reset(void)
{
    if(panyka_asyn)
    {
        panyka_asyn->write_err = 0;
    }
}


/**
 * NAME         anyka_fs_insert_file
 * @BRIEF	将一个文件插入到异步写队列中
 * @PARAM	file_id   文件ID
                   
 
 * @RETURN	void 
 * @RETVAL	
 */

void anyka_fs_insert_file(int file_id)
{
    Panyka_fs_file_point_info head, file_list;

    file_list = malloc(sizeof(anyka_fs_file_point_info));
    file_list->file_id = file_id;
    file_list->file_size = lseek64( file_id , 0, SEEK_CUR );
    file_list->next = NULL;
    
    pthread_mutex_lock(&panyka_asyn->fs_lock);
    head = panyka_asyn->file_list;
    if(head == NULL)
    {
        panyka_asyn->file_list = file_list;        
        pthread_mutex_unlock(&panyka_asyn->fs_lock);
        return;
    }
    while(head->next)
    {
        head = head->next;
    }
    head->next = file_list;
    
    pthread_mutex_unlock(&panyka_asyn->fs_lock);
}



/**
 * NAME         anyka_fs_set_file_size
 * @BRIEF	设置异步写文件的大小
 * @PARAM	file_id   文件ID
 * @RETURN	void 
 * @RETVAL	
 */

int anyka_fs_set_file_size(int file_id, int file_size)
{
    Panyka_fs_file_point_info head;

    if(panyka_asyn == NULL)
    {
        return -1;
    }
    head = panyka_asyn->file_list;
    while(head)
    {
        if(head->file_id == file_id)
        {
            head->file_size += file_size;
            return head->file_size;
        }
        head = head->next;
    }
    return -1;
}




/**
 * NAME         anyka_fs_remove_file
 * @BRIEF	将文件移出异步写队列中
 * @PARAM	file_id   文件ID
                   
 
 * @RETURN	void 
 * @RETVAL	
 */

void anyka_fs_remove_file(int file_id)
{
    Panyka_fs_file_point_info pre,head;
    
    pthread_mutex_lock(&panyka_asyn->fs_lock);

    head = panyka_asyn->file_list;
    pre = head;
    while(head)
    {
        if(head->file_id == file_id)
        {
            break;
        }
        pre = head;
        head = head->next;
    }
    if(head)
    {
        if(pre == head)
        {
            panyka_asyn->file_list = head->next;
        }
        else
        {
            pre->next = head->next;
        }
        free(head);
    }
    pthread_mutex_unlock(&panyka_asyn->fs_lock);
}


/**
 * NAME         anyka_fs_check_file_over
 * @BRIEF	检查当前这个文件是否异步写结束
 * @PARAM	file_id   文件ID
                   
 
 * @RETURN	void 
 * @RETVAL	
 */

uint8 anyka_fs_check_file_over(int file_id)
{
    uint8 ret = 1;
    int i, total;
    
    pthread_mutex_lock(&panyka_asyn->fs_lock);
    total = anyka_queue_items(panyka_asyn->asyn_write_queue);
    for(i = 0; i < total; i ++)
    {
        Panyka_fs_asyn_file_info paio_info;
        paio_info = (Panyka_fs_asyn_file_info) anyka_queue_get_index_item(panyka_asyn->asyn_write_queue, i);

        if(paio_info == NULL)
        {
            break;
        }
        if(paio_info->file_id == file_id)
        {
            ret = 0;
            break;
        }
    }
    pthread_mutex_unlock(&panyka_asyn->fs_lock);
    return ret;
}


/**
 * NAME         anyka_fs_flush
 * @BRIEF	结束异步写,并将对应的文件ID的异步写数据全部写完
 * @PARAM	file_id   文件ID
                   
 
 * @RETURN	void 
 * @RETVAL	
 */

void anyka_fs_flush(int file_id)
{
    if(panyka_asyn == NULL)
    {
        return;
    }
    {
        //int i, run_flag = 1;
        while(1)
        {
            if(anyka_fs_check_file_over(file_id))
            {
                break;
            }
            usleep(10*1000);
        }
    }
}

/**
 * NAME         anyka_fs_read
 * @BRIEF	读数据，目前没有需要进行异步方式 ，所以直接读
 * @PARAM	hFile
                    buf
                    size                   
 
 * @RETURN	void 
 * @RETVAL	
 */

T_S32 anyka_fs_read(T_S32 hFile, T_pVOID buf, T_S32 size)
{
    return read(hFile, buf, size);
}


/**
 * NAME         anyka_fs_write
 * @BRIEF	异步写数据，如果不在异步写队列里，直接写
 * @PARAM	hFile
                    buf
                    size                   
 
 * @RETURN	void 
 * @RETVAL	
 */


T_S32 anyka_fs_write(T_S32 hFile, T_pVOID buf, T_S32 size)
{
    Panyka_fs_asyn_file_info paio_write_info;
    int retry_time = 0, sz;


    if((panyka_asyn == NULL))
    {
        return write(hFile, buf, size);
    }

    if(panyka_asyn->write_err > 10)
    {
        return 0;
    }

    //如果队列满，将等待空间释放再写数据
    while(anyka_queue_is_full(panyka_asyn->asyn_write_queue) && retry_time < 10)
    {
        usleep(10*1000);
        retry_time ++;
    }
    if (retry_time == 10)
        anyka_print("[%s:%d] this queue is full!\n", __func__, __LINE__);

    retry_time = 0;
	while(1)
	{
	    paio_write_info = anyka_fs_asyn_create_ram(size);

	    if(paio_write_info == NULL)
	    {        
			if(retry_time > 100)
			{
		        anyka_print("[[%s:%d] it fails to malloc!\n", __func__, __LINE__);
		        return 0;
			}
			retry_time ++;
			usleep(10*1000);
	    }
		else
		{
			break;
		}
	}
	
    pthread_mutex_lock(&panyka_asyn->fs_lock);
    sz = anyka_fs_set_file_size(hFile, size);
    if(sz == -1)
    {
        pthread_mutex_unlock(&panyka_asyn->fs_lock);
        anyka_fs_asyn_free((void *)paio_write_info);		
        return write(hFile, buf, size);
    }
	
    paio_write_info->file_id = hFile;
    memcpy(paio_write_info->buf, buf, size);
    paio_write_info->size = size;
    if(0 == anyka_queue_push(panyka_asyn->asyn_write_queue, (void *)paio_write_info))
    {
        anyka_print("[%s:%d] it fail to write, write_err=%d!\n", __func__, __LINE__, panyka_asyn->write_err);
        anyka_fs_asyn_free((void *)paio_write_info);
        panyka_asyn->write_err++;
    }
    pthread_mutex_unlock(&panyka_asyn->fs_lock);
    sem_post(&panyka_asyn->sem_write);
    return size;
    #if 0
    paio_write_info->aio_offset = 0;
    paio_write_info->aio_sigevent.sigev_notify = SIGEV_THREAD; 
    paio_write_info->aio_sigevent.sigev_notify_attributes = aio_completion_handler;  
    paio_write_info->aio_sigevent.sigev_notify_attributes = NULL; 
    paio_write_info->aio_sigevent.sigev_value.sival_ptr = paio_write_info; 
    anyka_print("write asyn11:%d, %d\n", hFile, size);
    #if 1
    if (aio_write(paio_write_info) != 0)
    {
      anyka_print("[anyka_fs:anyka_fs_write]:it fails to asyn wirte!/n");
      return 0;
    }   
    #endif
   //anyka_queue_push(panyka_asyn->asyn_write_queue, (void *)paio_write_info); 
   //sem_post(&panyka_asyn->sem_write);
    return size;
    #endif

}


/**
 * NAME         anyka_fs_seek
 * @BRIEF	SEEK
 * @PARAM	hFile
                    offset
                    whence                   
 
 * @RETURN	void 
 * @RETVAL	
 */


T_S32 anyka_fs_seek(T_S32 hFile, T_S32 offset, T_S32 whence)
{
    return lseek64(hFile, offset, whence);

}

/**
 * NAME         anyka_fs_tell
 * @BRIEF	得到当前文件的大小
 * @PARAM	hFile                
 
 * @RETURN	void 
 * @RETVAL	
 */

T_S32 anyka_fs_tell(T_S32 hFile)
{
    T_S32 ret;


    ret = anyka_fs_set_file_size(hFile, 0);
    if(ret <= 0)
    {
        ret =  lseek64( hFile , 0, SEEK_CUR );
    }
    return ret;
}


/**
 * NAME         anyka_fs_isexist
 * @BRIEF	文件是否存在
 * @PARAM	hFile                
 
 * @RETURN	void 
 * @RETVAL	
 */

T_S32 anyka_fs_isexist(T_S32 hFile)
{
    if(lseek64( hFile , 0, SEEK_CUR ) < 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}







