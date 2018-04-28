#include "common.h"


typedef struct _anyka_phone_info_
{
    unsigned long end_time;
    int time_per_phone;
    char *path;
    anyka_photo_callabck *pcallback;
    void *callback_para;
    PA_HTHREAD photo_id;
    pthread_mutex_t photo_mutex;
    sem_t   photo_sem;
    sem_t   photo_ok_sem;
}anyka_phone_info, *Panyka_phone_info;



static Panyka_phone_info pphoto_handle = NULL;


/**
 * NAME         anyka_phone_save
 * @BRIEF	拍照数据获取通知函数
 * @PARAM	arg
 * @RETURN	
 * @RETVAL	
 */
void anyka_phone_save(void *parm, T_STM_STRUCT *pstream)
{
    char file_name[100];

    pthread_mutex_lock(&pphoto_handle->photo_mutex);
    video_fs_get_photo_name(pphoto_handle->path, file_name); 
    pthread_mutex_unlock(&pphoto_handle->photo_mutex);
    anyka_debug("[%s:%d] it save pic to file(%s)\n", __func__, __LINE__, file_name);
    long fid = open(file_name, O_RDWR | O_CREAT | O_TRUNC);
    if(fid <= 0)
    {
        anyka_print("[%s:%d] fails to create the pic file\n", __func__, __LINE__);
        sem_post(&pphoto_handle->photo_ok_sem);
        return;
    }
    write( fid, pstream->buf, pstream->size);
    close(fid);
    sync();
    if(pphoto_handle->pcallback)
    {
        pphoto_handle->pcallback(pphoto_handle->callback_para, file_name);
    }
    sem_post(&pphoto_handle->photo_ok_sem);
    
}


/**
 * NAME         anyka_phone_manage
 * @BRIEF	拍照线程
 * @PARAM	arg
 * @RETURN	
 * @RETVAL	
 */

void* anyka_phone_manage(void *arg)
{	
	unsigned long ts, pic_time;
	struct timeval tvs;

	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());

    while(1)
    {
        sem_wait(&pphoto_handle->photo_sem);
        gettimeofday(&tvs, NULL);
        pic_time = tvs.tv_sec;        
        while(1)
        {
            gettimeofday(&tvs, NULL);
            ts = tvs.tv_sec;
            if(ts >= pphoto_handle->end_time)
            {
                break;
            }
            if(ts >= pic_time)
            {
                usleep(500);
                video_add(anyka_phone_save, pphoto_handle, FRAMES_ENCODE_PICTURE, 2048);            
                sem_wait(&pphoto_handle->photo_ok_sem);
                video_del(pphoto_handle);
                sync();
                pic_time = pphoto_handle->time_per_phone + pic_time;
            }
            sleep(1);
        }
    }
}


/**
 * NAME         anyka_photo_init
 * @BRIEF	启动拍照线程，等待用户拍照指令
 * @PARAM	photo_path  拍照的路径
 * @RETURN	
 * @RETVAL	
 */

int anyka_photo_init(char *photo_path)
{
    int ret;
    if(pphoto_handle)
    {
        anyka_print("[%s:%d] photo thread is working \n", __func__, __LINE__);
        return -1;
    }		
    pphoto_handle = (Panyka_phone_info)malloc(sizeof(anyka_phone_info));
    if(pphoto_handle == NULL)
    {
        anyka_print("[%s:%d] fails to malloc\n", __func__, __LINE__);
        return -1;
    }
    pphoto_handle->path = malloc(strlen(photo_path) + 1);
    if(pphoto_handle->path == NULL)
    {
        free(pphoto_handle);
        pphoto_handle = NULL;
        anyka_print("[%s:%d] fails to malloc\n", __func__, __LINE__);
        return -1;
    }
    strcpy(pphoto_handle->path, photo_path);
    pphoto_handle->end_time = 0;
    pphoto_handle->time_per_phone = 1;
        
    pthread_mutex_init(&pphoto_handle->photo_mutex, NULL);
    sem_init(&pphoto_handle->photo_sem, 0, 0);
    sem_init(&pphoto_handle->photo_ok_sem, 0, 0);
    if ( ( ret = anyka_pthread_create( &(pphoto_handle->photo_id), anyka_phone_manage, pphoto_handle, ANYKA_THREAD_MIN_STACK_SIZE, -1) ) != 0 ) 
    {
        anyka_print("[%s:%d] fails to create thread!\n", __func__, __LINE__);
        sem_destroy(&pphoto_handle->photo_sem);
        anyka_pthread_mutex_destroy(&pphoto_handle->photo_mutex);
        free(pphoto_handle->path);
        free(pphoto_handle);
        pphoto_handle = NULL;
        return -1;
    }
    return 0;
}

/**
 * NAME         anyka_photo_start
 * @BRIEF	开始启动拍照
 * @PARAM	total_time   拍照时间，拍照将会持续的时间,如果为-1,将持续拍照
                    time_per_phone     拍照的间隔时间
                    photo_path  拍照的路径，如果为NULL,将使用上次拍照的路径
                    ppic_tell     如果拍照结束，将通知用户
                    para           回调函数的参数
 * @RETURN	
 * @RETVAL	
 */

int  anyka_photo_start(int total_time, int time_per_phone, char *photo_path, anyka_photo_callabck *ppic_tell, void *para)
{
    int ret;
	unsigned long ts;
	struct timeval tvs;
	gettimeofday(&tvs, NULL);
	ts = (tvs.tv_sec);
    
    if(total_time == -1)
    {
        pphoto_handle->end_time = (unsigned long)total_time;
    }
    else
    {        
        pphoto_handle->end_time = total_time + ts;
    }
    pphoto_handle->pcallback = ppic_tell;
    pphoto_handle->callback_para = para;
    pphoto_handle->time_per_phone = time_per_phone;
    if(photo_path)
    {
        pthread_mutex_lock(&pphoto_handle->photo_mutex);
        free(pphoto_handle->path);
        pphoto_handle->path = malloc(strlen(photo_path) + 1);
        strcpy(pphoto_handle->path, photo_path);
        pthread_mutex_unlock(&pphoto_handle->photo_mutex);
        
    }
    ret = video_fs_create_dir(pphoto_handle->path);
    if(ret == 0)
    {
        sem_post(&pphoto_handle->photo_sem);
    }
    return ret == 0;
}

/**
 * NAME         anyka_photo_stop
 * @BRIEF	提前结束之前启动的拍照
 * @PARAM	void
 * @RETURN	void
 * @RETVAL	
 */

void anyka_photo_stop()
{
    pphoto_handle->end_time = 0;
}


