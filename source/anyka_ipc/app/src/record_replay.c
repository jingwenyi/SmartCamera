#include "common.h"
#include "video_fs_list_manage.h"




struct _record_play_link;
typedef struct _record_play_link *Precord_play_link;
typedef struct _record_play_link
{
    int run_flag;
    int play_flag;
    void *userdata;
    PA_HTHREAD  id;
    P_RECORD_REPLAY_CALLBACK *pcallback;
    Precord_replay_info list;
    Precord_play_link next;
}record_play_link;


typedef struct _record_play_handle_
{
    pthread_mutex_t     link_mutex;
    Precord_play_link user_link;
}record_play_handle, *Precord_play_handle;

static Precord_play_handle precord_play = NULL;


#define RECORD_REPLAY_DEBUG     //anyka_print
#if 0
int test_malloc_number = 0;
void *RECORD_REPLAY_MALLOC(size_t size)
{
	test_malloc_number ++;
	return malloc(size);
}
void RECORD_REPLAY_free(void *ptr)
{
	test_malloc_number --;
	free(ptr);
}
#else
#define RECORD_REPLAY_MALLOC malloc
#define RECORD_REPLAY_free free

#endif

/**
 * NAME         record_replay_play_thread
 * @BRIEF	 暂停回放
  
 * @PARAM    mydata        回放的用户结构信息
 * @RETURN	void
 * @RETVAL	
 */

void record_replay_pause_thread(void *mydata)
{
    if( NULL == precord_play)
    {
        return;
    }
    Precord_play_link head;
    
    if( NULL == precord_play)
    {
        return;
    }
    
    pthread_mutex_lock(&precord_play->link_mutex);

    head = precord_play->user_link;

    while(head)
    {
        if(head->userdata == mydata)
        {
            break;
        }
        head = head->next;
    }
    if(head)
    {
        head->play_flag = 0;
    }    
    pthread_mutex_unlock(&precord_play->link_mutex);
    
}

/**
 * NAME         record_replay_play_thread
 * @BRIEF	 继续回放
  
 * @PARAM    mydata        回放的用户结构信息
 * @RETURN	void
 * @RETVAL	
 */

void record_replay_play_thread(void *mydata)
{
    if( NULL == precord_play)
    {
        return;
    }
    Precord_play_link head;
    
    if( NULL == precord_play)
    {
        return;
    }
    
    pthread_mutex_lock(&precord_play->link_mutex);

    head = precord_play->user_link;

    while(head)
    {
        if(head->userdata == mydata)
        {
            break;
        }
        head = head->next;
    }
    if(head)
    {
        head->play_flag = 1;
    }    
    pthread_mutex_unlock(&precord_play->link_mutex);
    
}

/**
 * NAME         record_replay_stop_thread
 * @BRIEF	 停止回放
  
 * @PARAM    mydata        回放的用户结构信息
 * @RETURN	void
 * @RETVAL	
 */

void record_replay_stop_thread(void *mydata)
{
    if( NULL == precord_play)
    {
        return;
    }
    Precord_play_link head;
    
    if( NULL == precord_play)
    {
        return;
    }
    
    pthread_mutex_lock(&precord_play->link_mutex);

    head = precord_play->user_link;

    while(head)
    {
        if(head->userdata == mydata)
        {
            break;
        }
        head = head->next;
    }
    if(head)
    {
        head->play_flag = 1;
        head->run_flag = 0;
    }    
    pthread_mutex_unlock(&precord_play->link_mutex);
    
    if(head)
    {
        pthread_join(head->id, NULL);
    }
}

/**
 * NAME         record_replay_remove_thread
 * @BRIEF	 将相应的回放句柄增加到队列
  
 * @PARAM    cur        回放的句柄信息
 * @RETURN	void
 * @RETVAL	
 */

void record_replay_insert_thread(Precord_play_link cur)
{
    Precord_play_link head;
    
    if( NULL == precord_play)
    {
        return;
    }
    pthread_mutex_lock(&precord_play->link_mutex);
    head = precord_play->user_link;
    if(head == NULL)
    {
        precord_play->user_link = cur;
        pthread_mutex_unlock(&precord_play->link_mutex);
        return;
    }
    while(head->next)
    {
        head = head->next;
    }
    head->next = cur;
    pthread_mutex_unlock(&precord_play->link_mutex);
}

/**
 * NAME         record_replay_remove_thread
 * @BRIEF	 将相应的回放句柄移除队列
 
 
 * @PARAM    cur        回放的句柄信息
 * @RETURN	void
 * @RETVAL	
 */

void record_replay_remove_thread(Precord_play_link cur)
{
    if( NULL == precord_play)
    {
        return;
    }
    Precord_play_link head, pre = NULL;
    
    if( NULL == precord_play)
    {
        return;
    }
    
    pthread_mutex_lock(&precord_play->link_mutex);

    head = precord_play->user_link;

    while(head)
    {
        if(head == cur)
        {
            if(head == precord_play->user_link)
            {
                precord_play->user_link = head->next;
            }
            else
            {
                pre->next = head->next;
            }
            break;
        }
        pre = head;
        head = head->next;
    }
    if(head)
    {
        Precord_replay_info tmp, record = head->list;
        while(record)
        {
            tmp = record;
            record = record->next;
            RECORD_REPLAY_free(tmp);
        }
        RECORD_REPLAY_free(head);
    }    
    pthread_mutex_unlock(&precord_play->link_mutex);
}

/**
 * NAME         record_replay_send_video
 * @BRIEF	 将指定 文件进行回放处理
 
 
 * @PARAM    main_info        启动此次回放的句柄信息
                    record_info      当前要回放的文件信息
 * @RETURN	void
 * @RETVAL	
 */

void record_replay_send_video(Precord_play_link main_info, Precord_replay_info record_info)
{
    T_STM_STRUCT *record_data;
    int type;
    void *handle;
    unsigned long cur_time, next_time = 0, base_time, end_time ;
	struct timeval tvs;
	int differ;

    anyka_print("[%s,%d]:play file:%s, start:%ld, time:%ld\n", __FUNCTION__, __LINE__, record_info->file_name, record_info->play_start, record_info->play_time);
    handle = video_demux_open(record_info->file_name, record_info->play_start*1000);
    
	gettimeofday(&tvs, NULL);
    base_time = (tvs.tv_sec - record_info->play_start)*1000 + (tvs.tv_usec) /1000;
    end_time = base_time + (record_info->play_start + record_info->play_time)*1000;

    record_data = video_demux_get_data(handle, &type);
    while(next_time < end_time && record_data!=NULL)
    {
        while(main_info->play_flag == 0 && main_info->run_flag)
        {
            usleep(100*1000);
            base_time += 100;
        }
   
        if(main_info->run_flag == 0)
        {
            anyka_print("[%s,%d]:user cancel the replay record!\n", __FUNCTION__, __LINE__);
            break;
        }
        gettimeofday(&tvs, NULL);
        cur_time = (tvs.tv_sec)*1000 + (tvs.tv_usec) /1000;

		differ = next_time -cur_time;
        if(cur_time < next_time && type ==1)
        {
				usleep((differ) * 1000);
        }
        
        //把时间戳调整为相对于播放起始值
		record_data->timestamp =record_data->timestamp+(record_info->recrod_start_time - (unsigned long)main_info->userdata)*1000;
//        anyka_print("frame time_stamp=%u,type=%d,Iframe=%d,curtime=%u\n"
//			,(unsigned int)record_data->timestamp,(int)type, (int)record_data->iFrame,(unsigned int)cur_time);
        if(main_info->pcallback) 
        {
            main_info->pcallback(main_info->userdata, type, record_data);
        }
        video_demux_free_data(record_data);
		
		//下一帧
        record_data = video_demux_get_data(handle, &type);
        if(record_data == NULL)
        {
            break;
        }
        if(type == 1)//video
        {
            next_time = base_time + record_data->timestamp;
        }
		
    }
    if(record_data == NULL)
    {
        anyka_print("[%s:%d] fail to get data,end!\n", __func__, __LINE__);
    }else
    {
        video_demux_free_data(record_data);
    }

    video_demux_close(handle);
    anyka_print("[%s:%d] play file:%s, end!\n",__func__, __LINE__, record_info->file_name);
    
}

/**
 * NAME         record_replay_thread
 * @BRIEF	 录像回放的主线程，将顺序启动本地文件，按要求进行播放
 
 
 * @PARAM    user        当前用户句柄
 * @RETURN	void
 * @RETVAL	
 */

void* record_replay_thread(void *user)
{
	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());

    Precord_play_link cur = (Precord_play_link)user;
    Precord_replay_info head, next;

    head = cur->list;
    while(head&& cur->run_flag)
    {
        next = head->next;
        record_replay_send_video(cur, head);
        RECORD_REPLAY_free(head);
        head = next;
        cur->list = head;
        if((check_sdcard() < 0))
        {
            break;
        }
    }
    anyka_print("[%s:%d] record replay %s!\n", 
		__func__, __LINE__, (cur->run_flag == 0)? "stop":"finish");
    record_replay_remove_thread(cur);
    if(cur->run_flag)
    {
        pthread_exit(NULL);
    }
    anyka_print("[%s:%d] the current record replay over!\n", __func__, __LINE__);
	
    return NULL;
}



/**
 * NAME         record_replay_init
 * @BRIEF	初始化录像回放相关信息，这里只创建线程处理，然后直接返回
 
 
 * @PARAM    void
 * @RETURN	void
 * @RETVAL	
 */

void record_replay_init(void)
{
    
    video_fs_list_init();
    if( NULL == precord_play)
    {
        precord_play = (Precord_play_handle)RECORD_REPLAY_MALLOC(sizeof(record_play_handle));
        memset(precord_play, 0, sizeof(record_play_handle));
        pthread_mutex_init(&precord_play->link_mutex, NULL); 
        precord_play->user_link = NULL;
    }
}



/**
 * NAME         record_replay_start
 * @BRIEF	 启动录像回放功能
 
 
 * @PARAM    start_time              回放的开始时间
                    end_time               回放的结束时间
                    clip_limit   限制的视频片段数量，0表示不限制, >0 是有效值
                    type                        要求回放的类型，支持警告与录像回放
                    mydata                     用户数据结构
                    psend_video_callback 送音视频数据的回调函数
 * @RETURN	void
 * @RETVAL	
 */

int record_replay_start(unsigned long  start_time, unsigned long end_time, int clip_limit,int type, void *mydata, P_RECORD_REPLAY_CALLBACK psend_video_callback)
{
    Precord_play_link cur;
    
    if((check_sdcard() < 0))
    {
        anyka_print("[%s,%d]it fails to start beacuse of sd out!\n", __FUNCTION__, __LINE__);
        return 1;
    }
    
    if(NULL == precord_play)
    {
        return 1;
    }

    
    cur = (Precord_play_link)RECORD_REPLAY_MALLOC(sizeof(record_play_link));

    cur->list = video_fs_list_get_record(start_time, end_time, clip_limit,type);
    if(cur->list == NULL)
    {
        RECORD_REPLAY_free(cur);
        return 1;
    }
    cur->userdata = mydata;
    cur->pcallback = psend_video_callback;
    cur->next = NULL;
    cur->run_flag = 1;
    cur->play_flag = 1;
    record_replay_insert_thread(cur);
    anyka_print("[%s:%d] we will start record replay, time begin %ld end  %ld\n",__FUNCTION__, __LINE__, start_time, end_time);
    if(anyka_pthread_create( &(cur->id), record_replay_thread, (void *)cur, ANYKA_THREAD_NORMAL_STACK_SIZE, -1) != 0 )
    {
        anyka_print("[%s:%d] it fail to create thread!\n", __FUNCTION__, __LINE__);
        record_replay_remove_thread(cur);
    }
    return 0;
}

/**
 * NAME         record_replay_play_status
 * @BRIEF	 录像回放的播放与暂停
 
 
 * @PARAM    mydata        用户数据结构，是在启动回放传进来的
                    play            1-->play; 0-->pause     
 * @RETURN	void
 * @RETVAL	
 */

void record_replay_play_status(void *mydata, int play)
{
    if(play)
    {
        anyka_print("[%s:%d]we will PLAY the record replay!", __FUNCTION__, __LINE__);
        record_replay_play_thread(mydata);
    }
    else
    {
        anyka_print("[%s:%d]we will PAUSE the record replay!", __FUNCTION__, __LINE__);
        record_replay_pause_thread(mydata);
    }
}

/**
 * NAME         record_replay_stop
 * @BRIEF	 录像回放的取消
 
 
 * @PARAM    mydata        用户数据结构，是在启动回放传进来的
 * @RETURN	void
 * @RETVAL	
 */

void record_replay_stop(void *mydata)
{
    anyka_print("[%s:%d] we will stop the record replay!\n", __FUNCTION__, __LINE__);
    record_replay_stop_thread(mydata);
}

////////////////////////////tmp use
//下面的三个函数是大拿接口所用，后续可以改用腾讯接口的实现方式
#if 1
void * record_replay_get_open_file(int type, unsigned long start_time, unsigned long find_end, Precord_replay_info result)
{
	return NULL;
}

void record_replay_get_close_file(void *record_list)
{
}

int record_replay_get_read(void *record_list, unsigned long find_end, Precord_replay_info result)
{
	return 0;
}

#endif

#if 0

/**
 * NAME         record_replay_get_open_file
 * @BRIEF	打开录像文件记录表，并设置到指定位置
  
 * @PARAM    type                回放的类型
                    start_time       需要记录的开始时间
                    find_end       返回符合记录的最后时间
                    result              记录信息储存
 * @RETURN	查找文件句柄
 * @RETVAL	NULL-->FAIL, other-->success
 */


void * record_replay_get_open_file(int type, unsigned long start_time, unsigned long find_end, Precord_replay_info result)
{
    int find = 0;
    char *file_buf;
    FILE * record_list;
    char *save_file;
    

    if(type == RECORD_REPLAY_TYPE)
    {
        save_file = RECORD_REPLAY_LIST_NAEM;
    }
    else
    {
        save_file = ALARM_REPLAY_LIST_NAEM;
    }
    record_list = fopen(save_file,  "r");
    if(record_list < 0)
    {
        anyka_print("[%s,%d]we can't open the record.list!\n", __FUNCTION__, __LINE__);
        return NULL;
    }
    file_buf = (char *)RECORD_REPLAY_MALLOC(1024);
    
    while(!feof(record_list))
    {
        if(fgets(file_buf, 1024, record_list) == NULL)
        {
            break;
        }
        if(strstr(file_buf,"file:"))
        {
            video_fs_list_format_info(file_buf, result->file_name, &result->recrod_start_time, &result->record_time);
            if(result->recrod_start_time >= find_end )
            {
                find = 0;
                break;
            }            
            
            if(result->recrod_start_time >= start_time)
            {
                find = 1;
                break;
            }
        }
    }   
    RECORD_REPLAY_free(file_buf);
    if(find == 0)
    {
        fclose(record_list);
        return NULL;
    }
    return (void *)record_list;
    
}


/**
 * NAME         record_replay_get_close_file
 * @BRIEF	关闭打开文件
  
 * @PARAM    
 * @RETURN	NONE
 * @RETVAL	
 */


void record_replay_get_close_file(void *record_list)
{

    fclose((FILE *)record_list);
}


/**
 * NAME         record_replay_get_read
 * @BRIEF	得到符合记录的下一条记录住处
  
 * @PARAM    record_list       open时候 的句柄
                    find_end       符合记录的最后时间
                    result              记录信息储存
 * @RETURN	查找结果
 * @RETVAL	0-->FAIL, 1-->success
 */

int record_replay_get_read(void *record_list, unsigned long find_end, Precord_replay_info result)
{
    int find = 1;
    char *file_buf;

    file_buf = (char *)RECORD_REPLAY_MALLOC(1024);
    
    while(!feof((FILE *)record_list))
    {
        if(fgets(file_buf, 1024, (FILE *)record_list) == NULL)
        {
            find = 0;
            break;
        }
        if(strstr(file_buf,"file:"))
        {
            video_fs_list_format_info(file_buf, result->file_name, &result->recrod_start_time, &result->record_time);
            if(result->recrod_start_time >= find_end )
            {
                find = 0;
                break;
            }
            break;
        }
    }        
    
    RECORD_REPLAY_free(file_buf);
    return find;
    
}
#endif 

