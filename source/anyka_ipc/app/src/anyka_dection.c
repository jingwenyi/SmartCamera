/*******************************************************************
此文件完成侦测相关功能，目前只支持移动与声音侦测。
移动侦测会启动侦测录像功能，持续移动会进行录像文件分割。在无移动一分钟后会结束侦测录像。
启动录像保存文件的规则是从发现前VIDEO_CHECK_MOVE_IFRAMES个I帧开始，到没有侦测到后的VIDEO_CHECK_MOVE_OVER_TIME
时间的整个区间。
*******************************************************************/

#include "common.h"
#include "anyka_alarm.h"
#include "record_replay.h"

#define VIDEO_CHECK_MOVE_IFRAMES 		2
#define MAX_ALARM_FILE_NAME_LEN        	100         //最大的录像文件名


#define ALARM_TMP_FILE_NAME 		"/mnt/tmp/"
#define AKGPIO_DEV 					"/dev/akgpio"
#define ALARM_TMP_PHOTO_DIR     	"/tmp/alarm/"
#define MIN_DISK_SIZE_FOR_ALARM		(200*1024)  //磁盘的保留空间，单位是KB


/** 两组录像文件信息，一组临时文件，一组最终保存的文件**/
const char * alarm_record_tmp_file_name[] = 
{
	"/mnt/tmp/alarm_record_file_1",
	"/mnt/tmp/alarm_record_file_2"
};

const char * alarm_rec_tmp_index[] = 
{
	"/mnt/tmp/alarm_record_index_1",
	"/mnt/tmp/alarm_record_index_2"
};

/** 录像信息结构体 **/
typedef struct _alarm_record_info
{
	uint32  file_size;	
	char    *file_name;
}alarm_record_info;

/** 录像文件索引结构体**/
typedef struct alarm_record_file
{
	int index_fd;
	int record_fd;
}alarm_record_file, *Palarm_record_file;

typedef struct _video_check_move_ctrl
{
    uint8   run_flag;	//侦测录像运行标志
    uint8   check_flag;	//侦测后的处理标志，0表示不做处理
    uint8   check_type;	//侦测类型
    uint8   save_record_flag;	//是否保存文件标志
    uint8   iframe_num;	// key 帧( I 帧)数量
    uint8   move_ok;	//是否处在移动状态，0表示未移动
    uint8   alarm_type;	//报警类型
	uint8   rec_used_index;	//录像使用的节点索引
	uint8   rec_save_index;	//保存录像使用的节点索引
    uint32  move_time;	//检测到异常的起始时间
    uint32  cur_time;	//当前时间
	uint32  alarm_record_total_size;	//总的移动侦测录像文件占用的空间大小	
	uint32  used_alarm_record_size;		//当前系统已占用的移动侦测录像文件大小
    uint16  video_level;		//视频等级
    uint16  audio_level;		//音频等级
    void    *video_queue;		//视频数据队列
    void    *audio_queue;		//音频数据队列
	void 	*record_queue;		//录像文件队列
	void 	*mux_handle;		//合成库合成句柄
	void 	*save_mux_handle;	//保存录像用的合成库句柄
    PA_HTHREAD  id;			
    PA_HTHREAD  save_file_id;
    pthread_mutex_t     move_mutex;	//本模块线程锁
    sem_t   save_sem;		//侦测到移动时抛的信号
    sem_t   data_sem;		//获取到音视频数据后抛的信号
	sem_t   manage_sem;		//单个文件录制时间到的信号
	char 	alarm_record_file_name[2][MAX_ALARM_FILE_NAME_LEN];		//录像文件名数组
	alarm_record_file 	record_file[2];		//录像节点信息数组
    PANYKA_SEND_ALARM_FUNC *Palarm_func;	//用户回调
    PANYKA_FILTER_VIDEO_CHECK_FUNC *filter_check;
}video_check_move_ctrl, *Pvideo_check_move_ctrl;

static Pvideo_check_move_ctrl pvideo_move_ctrl = NULL;
int anyka_dection_pass_video_check();

/**
 * NAME         anyka_detection_wait_irq
 * @BRIEF	等待GPIO中断到来
 * @PARAM	int fd，打开的设备句柄
 			struct gpio_info gpio，gpio结构信息
 * @RETURN	void
 * @RETVAL	
 */
static int anyka_detection_wait_irq(int fd, struct gpio_info gpio)
{
	if(ioctl(fd, SET_GPIO_IRQ, &gpio) < 0)
	{
		anyka_print("[%s:%d]set irq error.\n", __func__, __LINE__);
		return -1;

	}
	anyka_print("[%s:%d]set irq success.\n", __func__, __LINE__);

	if(ioctl(fd, LISTEN_GPIO_IRQ, (void *)gpio.pin) < 0)
	{
		anyka_print("[%s:%d]listen irq error.\n", __func__, __LINE__);
		return -2;
	}
	anyka_print("[%s:%d]get irq.\n", __func__, __LINE__);

	if(ioctl(fd, DELETE_GPIO_IRQ, (void *)gpio.pin) < 0)
	{
		anyka_print("[%s:%d]clean irq error.\n", __func__, __LINE__);
		return -3;
	}

	return 0;
}



/**
 * NAME         anyka_dection_send_alarm_info
 * @BRIEF	发送警告信息的callback
 * @PARAM	void *para，要发送的参数
 			char *file_name，发送侦测图片的文件名
 * @RETURN	void
 * @RETVAL	
 */

void anyka_dection_send_alarm_info(void *para, char *file_name)
{
    if(pvideo_move_ctrl->Palarm_func)
    {   
        pvideo_move_ctrl->Palarm_func(pvideo_move_ctrl->alarm_type, 0, file_name, time(0), 1);           
    }
}


/**
 * NAME         anyka_dection_alarm
 * @BRIEF	其他类型侦测的报警，目前是GPIO侦测
 * @PARAM	void
 * @RETURN	NULL
 * @RETVAL	
 */

static void *anyka_dection_alarm(void)
{
	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());

	int alarm_status = -1;
	int ak_alarm_fd = -1;
	struct gpio_info gpio_alarm_in;
	struct gpio_info gpio_dly_dr;

	memset(&gpio_alarm_in, 0, sizeof(struct gpio_info));
	memset(&gpio_dly_dr, 0, sizeof(struct gpio_info));
	
	gpio_alarm_in.pin		= 5,
	gpio_alarm_in.pulldown	= -1,
	gpio_alarm_in.pullup 	= AK_PULLUP_ENABLE,
	gpio_alarm_in.value		= AK_GPIO_OUT_HIGH,
	gpio_alarm_in.dir		= AK_GPIO_DIR_INPUT,
	gpio_alarm_in.int_pol	= AK_GPIO_INT_LOWLEVEL,

	gpio_dly_dr.pin        = 61,
	gpio_dly_dr.pulldown   = AK_PULLDOWN_ENABLE,
	gpio_dly_dr.pullup     = -1,
	gpio_dly_dr.value      = AK_GPIO_OUT_HIGH,
	gpio_dly_dr.dir        = AK_GPIO_DIR_OUTPUT,
	gpio_dly_dr.int_pol    = -1,


	ak_alarm_fd = open(AKGPIO_DEV, O_RDWR);
	if(ak_alarm_fd < 0)
	{
		anyka_print("[%s:%d] open ak customs device failed.\n", __func__, __LINE__);
		return NULL;
	}

	while(1){
		anyka_detection_wait_irq(ak_alarm_fd, gpio_alarm_in);     

		while(1)
		{
			ioctl(ak_alarm_fd, GET_GPIO_VALUE, &gpio_alarm_in);
			alarm_status = gpio_alarm_in.value;
			
			if(alarm_status > 0)
			{	
				/*********debounce**********/
				usleep(200);
				ioctl(ak_alarm_fd, GET_GPIO_VALUE, &gpio_alarm_in);
				alarm_status = gpio_alarm_in.value;
				
				if(alarm_status > 0)
				{
					anyka_print("[%s:%d]we check the alarm!\n", __func__, __LINE__);

					gpio_dly_dr.value = AK_GPIO_OUT_HIGH;
					ioctl(ak_alarm_fd, SET_GPIO_FUNC, &gpio_dly_dr);  
					
			        pvideo_move_ctrl->move_ok = 1;
			        pvideo_move_ctrl->alarm_type = SYS_CHECK_OTHER_ALARM;
			        pvideo_move_ctrl->move_time = time(0);
			        sem_post(&pvideo_move_ctrl->save_sem);

					sleep(2);
					gpio_dly_dr.value = AK_GPIO_OUT_LOW;
					ioctl(ak_alarm_fd, SET_GPIO_FUNC, &gpio_dly_dr);  
					
					break;
				}
			}
			else
			{
				anyka_print("[%s:%d] no alarm :%d\n", __func__, __LINE__, alarm_status);	
				break;
			}
		}
	}
	close(ak_alarm_fd);

	return NULL;
}


/**
 * NAME         anyka_dection_video_move_queue_free
 * @BRIEF	释放指定节点的资源
 * @PARAM	void *item ，指定要释放资源的节点
                   
 * @RETURN	void
 * @RETVAL	
 */

void anyka_dection_video_move_queue_free(void *item)
{
    T_STM_STRUCT *pstream = (T_STM_STRUCT *)item;

    if(pstream)
    {
        free(pstream->buf);
        free(pstream);
    }
}


/**
 * NAME         anyka_dection_video_move_queue_copy
 * @BRIEF	将参数所指的节点数据copy到一个新的节点
 * @PARAM	T_STM_STRUCT *pstream， 待copy的数据
                   
 * @RETURN	T_STM_STRUCT *
 * @RETVAL	成功返回新的数据节点指针，失败返回空
 */

T_STM_STRUCT * anyka_dection_video_move_queue_copy(T_STM_STRUCT *pstream)
{
    T_STM_STRUCT *new_stream;

    new_stream = (T_STM_STRUCT *)malloc(sizeof(T_STM_STRUCT));
    if(new_stream == NULL)
    {
        return NULL;
    }
    memcpy(new_stream, pstream, sizeof(T_STM_STRUCT));
    new_stream->buf = (T_pDATA)malloc(pstream->size);
    if(new_stream->buf == NULL)
    {
        free(new_stream);
        return NULL;
    }
    memcpy(new_stream->buf, pstream->buf, pstream->size);
    return new_stream;
}

/**
 * NAME         anyka_dection_move_video_data
 * @BRIEF	视频回调函数,目前系统只保存两个I帧的数据，所以超过
                    将之前的视频数据删除。
 * @PARAM	param 用户数据
                    pstream 音频相关数据
 * @RETURN	void *
 * @RETVAL	
 */

void anyka_dection_move_video_data(T_VOID *param, T_STM_STRUCT *pstream)
{
    T_STM_STRUCT *new_stream;
    uint32 last_time_stamp;
    int try_time = 0;
	
	pvideo_move_ctrl->cur_time = pstream->timestamp / 1000;
    
    if(pvideo_move_ctrl->run_flag == 0)
    {
        return;
    }
    
    pthread_mutex_lock(&pvideo_move_ctrl->move_mutex);	

    if((pstream->iFrame == 1) && (pvideo_move_ctrl->move_ok == 0))
    {
        pvideo_move_ctrl->iframe_num ++;
        if(pvideo_move_ctrl->move_ok == 0)
        {
            //我们将清除部分音频与视频数据，视频数据只保存两个
            //I,音频将与视频最后一个帧的时间进行同步。
            last_time_stamp = 0xFFFFFFFF;
            while(pvideo_move_ctrl->iframe_num > VIDEO_CHECK_MOVE_IFRAMES)
            {

                new_stream = (T_STM_STRUCT *)anyka_queue_pop(pvideo_move_ctrl->video_queue);

                if(new_stream == NULL)
                {
                    pvideo_move_ctrl->iframe_num = 0;
                    break;
                }
                
                last_time_stamp = new_stream->timestamp;
                if(new_stream->iFrame)
                {
                    T_STM_STRUCT * piframe_stream;
                    while(1)
                    {
                        piframe_stream = (T_STM_STRUCT *)anyka_queue_get_index_item(pvideo_move_ctrl->video_queue, 0);
                        if(piframe_stream == NULL)
                        {
                            break;
                        }
                        if(piframe_stream->iFrame)
                        {
                            break;
                        }
                        piframe_stream = (T_STM_STRUCT *)anyka_queue_pop(pvideo_move_ctrl->video_queue);
                        last_time_stamp = piframe_stream->timestamp;
                        anyka_dection_video_move_queue_free((void *)piframe_stream);
                    }
                    pvideo_move_ctrl->iframe_num --;
                }
                anyka_dection_video_move_queue_free((void *)new_stream);
            }
            //清除音频数据，与最后一个被清除的视频数据的时间为准
            if(last_time_stamp != 0xFFFFFFFF)
            {
                while(anyka_queue_not_empty(pvideo_move_ctrl->audio_queue))
                {
                    new_stream = anyka_queue_get_index_item(pvideo_move_ctrl->audio_queue, 0);
                    if(new_stream->timestamp >= last_time_stamp)
                    {
                        break;
                    }
                    new_stream = (T_STM_STRUCT *)anyka_queue_pop(pvideo_move_ctrl->audio_queue);
                    anyka_dection_video_move_queue_free((void *)new_stream);
                }
            }
        }
    }
    while(((new_stream = anyka_dection_video_move_queue_copy(pstream)) == NULL) && try_time < 10)
    {
        usleep(10*1000);
        anyka_print("we will wait for the video queue ram , it fail to  malloc!\n");
        try_time ++;
    }
    if(new_stream)
    {
        //如果队列满，将等待空间释放再写数据
        try_time = 0;
        while(anyka_queue_is_full(pvideo_move_ctrl->video_queue) && try_time < 10)
        {
            pthread_mutex_unlock(&pvideo_move_ctrl->move_mutex);
            usleep(10*1000);
            pthread_mutex_lock(&pvideo_move_ctrl->move_mutex);
            anyka_print("we will wait for the video queue free!\n");
            try_time ++;
        }
        
        if(anyka_queue_push(pvideo_move_ctrl->video_queue, new_stream) == 0)
        {
            anyka_print("we fails to add video data!\n");
            anyka_dection_video_move_queue_free((void *)new_stream);
        }
    }
	
    pthread_mutex_unlock(&pvideo_move_ctrl->move_mutex);
    if(pvideo_move_ctrl->move_ok)
    {
        sem_post(&pvideo_move_ctrl->data_sem);
    }
}

/**
 * NAME         anyka_dection_video_move_audio_data
 * @BRIEF	音频回调函数
 * @PARAM	param 用户数据
                    pstream 音频相关数据
 * @RETURN	void *
 * @RETVAL	
 */

void anyka_dection_video_move_audio_data(T_VOID *param, T_STM_STRUCT *pstream)
{
    T_STM_STRUCT *new_stream;
    
    if(pvideo_move_ctrl->run_flag == 0)
    {
        return;
    }

    pthread_mutex_lock(&pvideo_move_ctrl->move_mutex);
	/** copy the node data **/
    new_stream = anyka_dection_video_move_queue_copy(pstream);
    if(new_stream)
    {
		/** push the data to the queue **/
        if(anyka_queue_push(pvideo_move_ctrl->audio_queue, new_stream) == 0)
        {
            anyka_dection_video_move_queue_free((void *)new_stream);
        }
    }
    pthread_mutex_unlock(&pvideo_move_ctrl->move_mutex);
    if(pvideo_move_ctrl->move_ok)
    {
        sem_post(&pvideo_move_ctrl->data_sem);
    }
}

/**
 * NAME         anyka_dection_video_notice_dowith
 * @BRIEF	检查到移动时的回调
 * @PARAM	user 用户数据
                    notice
 
 * @RETURN	void *
 * @RETVAL	
 */

void anyka_dection_video_notice_dowith(void *user, uint8 notice)
{
    //如果首次移动，将清除部分以前的数据。
    Psystem_alarm_set_info alarm_info = anyka_sys_alarm_info();
    static int send_alarm_time;

    if(pvideo_move_ctrl->check_flag == 0)
    {
        //anyka_print("[%s:%d]we will pass the moving!\n", __func__, __LINE__);
        return ;
    }
    
    if(alarm_info->alarm_send_type == 0)
    {
        
        pthread_mutex_lock(&pvideo_move_ctrl->move_mutex);  
        pvideo_move_ctrl->move_ok = 1;		//set check move flag
        //pvideo_move_ctrl->move_time = time(0);
        if(pvideo_move_ctrl->save_record_flag == 0)	//no save record , get system time
        {
            pvideo_move_ctrl->cur_time = time(0);
        }
		pvideo_move_ctrl->move_time = pvideo_move_ctrl->cur_time;	//store current time
	
        sem_post(&pvideo_move_ctrl->save_sem);		//send message to get video data thread
        pthread_mutex_unlock(&pvideo_move_ctrl->move_mutex);  
        pvideo_move_ctrl->alarm_type = SYS_CHECK_VIDEO_ALARM;	//set alarm type
        /*if not preview and trigger move detection, move_ok is set 1, goto preview and this function is trigger and take move detection. fixed it*/
		if(pvideo_move_ctrl->filter_check && (1 == pvideo_move_ctrl->filter_check()))
        {
        	return;
        }
        if((send_alarm_time + alarm_info->alarm_send_msg_time < pvideo_move_ctrl->cur_time) && (pvideo_move_ctrl->Palarm_func))
        {
            send_alarm_time = pvideo_move_ctrl->cur_time;
			/** take photos **/
            if(0 == anyka_photo_start(1, 1, ALARM_TMP_PHOTO_DIR, anyka_dection_send_alarm_info, NULL))
            {
				/** call the user callback to send message to app **/
                if(pvideo_move_ctrl->Palarm_func)
                {
                    pvideo_move_ctrl->Palarm_func(pvideo_move_ctrl->alarm_type, 0, NULL, pvideo_move_ctrl->cur_time, 1);
                }
            }
        }   
    }
   
}


/**
 * NAME         anyka_dection_remove_tmp_file
 * @BRIEF	删除录像使用的临时文件
 * @PARAM	char *rec_path，查找文件路径
 			video_find_record_callback *pcallback， 找到文件之后用于排序的callback
 * @RETURN	uint64，返回符合类型的文件的总的size
 * @RETVAL	成功返回符合文件类型的字节大小，失败返回-1.
 */

void anyka_dection_remove_tmp_file()
{
    remove(alarm_rec_tmp_index[0]);
    remove(alarm_rec_tmp_index[1]);
    remove(alarm_record_tmp_file_name[0]);
    remove(alarm_record_tmp_file_name[1]);
    sync();
}


/**
 * NAME         anyka_dection_flush_record_file
 * @BRIEF	录像完成后关闭临时文件，关闭录像文件
 * @PARAM	int index，0 或1，两组文件切换使用
 * @RETURN	void
 * @RETVAL	
 */

void anyka_dection_flush_record_file(int index)
{
    struct stat     statbuf; 
    unsigned long cur_file_size;  
    
	/******  close last index file  *******/

	if(pvideo_move_ctrl->record_file[index].index_fd > 0)
	{
		/****** close record file ******/
		close(pvideo_move_ctrl->record_file[index].index_fd);	//close index fd 
		/****** remove tmp file ******/		
		remove(alarm_rec_tmp_index[index]);
	}

	/**** close last record file , rename the tmp file name for next time record ****/
	if(pvideo_move_ctrl->record_file[index].record_fd > 0)
	{
		fsync(pvideo_move_ctrl->record_file[index].record_fd);	//sync file
		close(pvideo_move_ctrl->record_file[index].record_fd);	//close record fd
		//anyka_print("[%s:%d] alarm record file name: %s\n", __func__, __LINE__, pvideo_move_ctrl->alarm_record_file_name[index]);
		/** rename the record file **/
		rename(alarm_record_tmp_file_name[index], pvideo_move_ctrl->alarm_record_file_name[index]);		

		/** get record file information **/
        stat(pvideo_move_ctrl->alarm_record_file_name[index], &statbuf );
        cur_file_size = (statbuf.st_size >> 10) + ((statbuf.st_size & 1023)?1:0);            //更新文件大小系统到队列.
        /** file size less than 100k, delete it **/
        if(cur_file_size < 100)
        {
            remove(pvideo_move_ctrl->alarm_record_file_name[index]);
        }
        else
        {	
			/** update the current file message into the record queue **/
            video_record_update_use_size(cur_file_size);
			video_fs_list_insert_file(pvideo_move_ctrl->alarm_record_file_name[index], RECORD_APP_ALARM);
        }
	}

	pvideo_move_ctrl->record_file[index].index_fd = 0;
	pvideo_move_ctrl->record_file[index].record_fd = 0;

}


/*
**	create record file and index file 
**	return val :
**	0 success, -1, failed
*/

int anyka_dection_creat_new_record_file(int index)
{
	char res[1000] = {0};

	pvideo_move_ctrl->record_file[index].record_fd = 
			open(alarm_record_tmp_file_name[index], O_RDWR| O_CREAT | O_TRUNC);
	if(pvideo_move_ctrl->record_file[index].record_fd < 0)
	{

		/** read-only file system **/
		if(EROFS == errno)
		{
			anyka_print("[%s:%d] %s, repair it ...\n", __func__, __LINE__, strerror(errno));
		
			do_syscmd("mount | grep '/dev/mmcblk0p1 on /mnt'", res);
			if(strlen(res) > 0) {
				anyka_print("[%s:%d] remount /dev/mmcblk0p1\n", __func__, __LINE__);
				system("mount -o remount,rw /dev/mmcblk0p1");
				return -1;
			}
		
			bzero(res, 1000);
	
			do_syscmd("mount | grep '/dev/mmcblk0 on /mnt'", res);
			if(strlen(res) > 0) {
				anyka_print("[%s:%d] remount /dev/mmcblk0\n", __func__, __LINE__);
				system("mount -o remount,rw /dev/mmcblk0");
				return -1;
			}
		
		} 
		else
			anyka_print("[%s:%d] %s\n", __func__, __LINE__, strerror(errno));
		return -1;	
		
	}

	pvideo_move_ctrl->record_file[index].index_fd = open(alarm_rec_tmp_index[index], O_RDWR| O_CREAT | O_TRUNC);
	if(pvideo_move_ctrl->record_file[index].index_fd < 0)
	{
		anyka_print("[%s:%d] open file %s failed, :%s\n", __func__, __LINE__, 
						alarm_rec_tmp_index[index], strerror(errno));
		close(pvideo_move_ctrl->record_file[index].record_fd);
		pvideo_move_ctrl->record_file[index].record_fd = 0;
		remove(alarm_record_tmp_file_name[index]);
		return -1;
	}
	
	return 0;
}

/**
 * NAME         anyka_dection_free_file_queue
 * @BRIEF	释放队列成员资源
 * @PARAM	void *item，要释放的资源节点
 * @RETURN	void
 * @RETVAL	
 */

void anyka_dection_free_file_queue(void *item)
{
	alarm_record_info *file_info = (alarm_record_info*)item;

	if(file_info)
	{
		free(file_info->file_name);
		free(file_info);
	}
}

/**
 * NAME         anyka_dection_malloc_file_queue
 * @BRIEF	分配队列节点的数据内存
 * @PARAM	int file_size， 要分配的文件长度
 * @RETURN	alarm_record_info *
 * @RETVAL	成功: 返回指向队列数据节点的指针
 			失败: 返回空
 */

alarm_record_info *anyka_dection_malloc_file_queue(int file_len)
{    
	alarm_record_info *file_info = NULL;

	file_info = (alarm_record_info *)malloc(sizeof(alarm_record_info));
	if(file_info)
	{
		file_info->file_name = (char *)malloc(file_len);
		if(file_info->file_name == NULL)
		{
			free(file_info);
			return NULL;
		}
	}
	return  file_info;
}

/**
 * NAME         anyka_dection_insert_file
 * @BRIEF	将文件插入到指定的队列中
 * @PARAM	char *file_name， 要插入的文件名
 			int file_size， 要要入的文件的大小
 * @RETURN	void
 * @RETVAL	
 */

void anyka_dection_insert_file(char *file_name, int file_size,int type)
{
	int file_len;    
	alarm_record_info * pnew_record;

	file_len = strlen(file_name);
	pnew_record = anyka_dection_malloc_file_queue(file_len + 4);
	pnew_record->file_size = file_size;
	strcpy(pnew_record->file_name, file_name);
	if(0 == anyka_queue_push(pvideo_move_ctrl->record_queue, (void *)pnew_record))
	{
		anyka_dection_free_file_queue(pnew_record);
	}
}

/**
 ** NAME    anyka_dection_compare_record_name
 ** @BRIEF	compare two record file name
 ** @PARAM	void
 ** @RETURN	void
 ** @RETVAL	
 **/

int anyka_dection_compare_record_name(void *item1, void *item2)
{
	alarm_record_info *precord1 = (alarm_record_info *)item1;
	alarm_record_info * precord2 = (alarm_record_info *)item2;
	int len1, len2;

	len1 = strlen(precord1->file_name);
	len2 = strlen(precord2->file_name);

	return memcmp(precord1->file_name, precord2->file_name, len1>len2?len1:len2); 
}

/**
 * NAME     anyka_dection_manage_file
 * @BRIEF	save record file
 * @PARAM	void *, now no use
 * @RETURN	void *
 * @RETVAL	
 */

void *anyka_dection_manage_file(void * arg)
{	
	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());

	char record_file_name[100];
	int create_file_ret;
	Psystem_alarm_set_info alarm_info = anyka_sys_alarm_info();
	alarm_record_info *alarm_remove_file;
	
	while(pvideo_move_ctrl->run_flag)
	{
		sem_wait(&pvideo_move_ctrl->manage_sem);

		uint32 cur_file_size;
		struct stat  statbuf;
		/**** send alarm message to the phone or computer ****/
		if(pvideo_move_ctrl->save_mux_handle)
		{
			anyka_print("[%s:%d] Close last handle, save file!\n", __func__, __LINE__);

			//uint32 cur_file_size;
			//struct stat  statbuf;

			/************ close mux handle *************/
			mux_close(pvideo_move_ctrl->save_mux_handle);

			/***** get record file name *****/
	        video_fs_get_alarm_record_name(pvideo_move_ctrl->alarm_type, 
					alarm_info->alarm_default_record_dir, record_file_name, ".mp4");

			/** copy the file name to the array **/
			strcpy(pvideo_move_ctrl->alarm_record_file_name[pvideo_move_ctrl->rec_save_index], record_file_name);

			/************* get_cur_file_index ***************/
            anyka_print("[%s:%d] Now save file[%d]: %s\n", __func__, __LINE__,
            		pvideo_move_ctrl->rec_save_index, pvideo_move_ctrl->alarm_record_file_name[pvideo_move_ctrl->rec_save_index]);

			/********* close the last file , open a new file  *********/
			anyka_dection_flush_record_file(pvideo_move_ctrl->rec_save_index);

			create_file_ret = anyka_dection_creat_new_record_file(pvideo_move_ctrl->rec_save_index);
            if(create_file_ret < 0)
            {
				remove(record_file_name);
                anyka_print("[%s:%d] It fails to create record file!\n", __func__, __LINE__);
            }
			
			sync();
			pvideo_move_ctrl->save_mux_handle = NULL;
		}

		stat(pvideo_move_ctrl->alarm_record_file_name[pvideo_move_ctrl->rec_save_index], &statbuf);
		cur_file_size = (statbuf.st_size >> 10) + ((statbuf.st_size & 1023)?1:0);
		anyka_dection_insert_file(record_file_name, cur_file_size,RECORD_APP_ALARM);
		pvideo_move_ctrl->used_alarm_record_size += cur_file_size;

		/***** if the room is not enough, delete the earliest record file *****/
		while(pvideo_move_ctrl->used_alarm_record_size >= pvideo_move_ctrl->alarm_record_total_size)
		{
			alarm_remove_file = (alarm_record_info *)anyka_queue_pop(pvideo_move_ctrl->record_queue);
			if(alarm_remove_file == NULL)
			{
				break;
			}
			anyka_print("[%s] room isn't enough, remove: %s, size:%d\n",
					__func__, alarm_remove_file->file_name, 
					pvideo_move_ctrl->used_alarm_record_size);				
			remove(alarm_remove_file->file_name);   
			pvideo_move_ctrl->used_alarm_record_size -= alarm_remove_file->file_size;           
			anyka_dection_free_file_queue(alarm_remove_file);
		}

	}
	anyka_print("[%s:%d] save the lasted alarm record finish.\n", __func__, __LINE__);

	return NULL;
}

/**
 ** NAME    anyka_dection_free_record_info
 ** @BRIEF	when check the SD card is out, we clean the record info
 ** @PARAM	void
 ** @RETURN	void 
 ** @RETVAL	
 **/
void anyka_dection_free_record_info(void)
{
	pvideo_move_ctrl->alarm_record_total_size = 0;
	pvideo_move_ctrl->used_alarm_record_size = 0;
	
	void *tmp = pvideo_move_ctrl->record_queue;
	pvideo_move_ctrl->record_queue = NULL;
	anyka_queue_destroy(tmp, anyka_dection_free_file_queue);
}


/**
 ** NAME    anyka_dection_get_record_info
 ** @BRIEF	create record queue for manage record file, then get the SD card record info
 ** @PARAM	void 
 ** @RETURN	int
 ** @RETVAL	-1, faild; 0 success
 **/
int anyka_dection_get_record_info(void)
{
    Psystem_alarm_set_info alarm_info = anyka_sys_alarm_info();

    if(NULL == pvideo_move_ctrl || (pvideo_move_ctrl->record_queue))
    {
		anyka_print("[%s:%d] Please initialize it at first!\n", __func__, __LINE__);	
        return 0;
    }
    
	pvideo_move_ctrl->record_queue = anyka_queue_init(2500);
	if(pvideo_move_ctrl->record_queue == NULL)
	{
		anyka_print("[%s:%d] it fails to init record queue\n", __func__, __LINE__);	
		anyka_queue_destroy(pvideo_move_ctrl->record_queue, anyka_dection_free_file_queue);
		return -1;
	}

	/*** initialize two wariable to record the space which can using for this model ***/
	pvideo_move_ctrl->alarm_record_total_size = video_fs_get_free_size(alarm_info->alarm_default_record_dir);
	pvideo_move_ctrl->used_alarm_record_size = video_fs_init_record_list(alarm_info->alarm_default_record_dir, 
															anyka_dection_insert_file, RECORD_APP_ALARM);
		
	/*** sort the record list ***/
	anyka_queue_sort(pvideo_move_ctrl->record_queue, anyka_dection_compare_record_name);

	/*** total record size is equal the remainder space plus ago record file occupy space ***/
	pvideo_move_ctrl->alarm_record_total_size += pvideo_move_ctrl->used_alarm_record_size;
	if(pvideo_move_ctrl->alarm_record_total_size < MIN_DISK_SIZE_FOR_ALARM)
	{
		anyka_print("[%s:%d] the SD card room is not enough, please clean it.\n", __func__, __LINE__);
		return -1;
	}
	pvideo_move_ctrl->alarm_record_total_size -= MIN_DISK_SIZE_FOR_ALARM; //reverve 100M size for others
	
	if(pvideo_move_ctrl->alarm_record_total_size <= pvideo_move_ctrl->used_alarm_record_size)
		sem_post(&pvideo_move_ctrl->manage_sem);

	anyka_print("[%s:%d] get record info, total size : %u, used size : %u\n", __func__, __LINE__,
				pvideo_move_ctrl->alarm_record_total_size, pvideo_move_ctrl->used_alarm_record_size);

	return 0;
}


/**
 ** NAME 	anyka_dection_start_save_file  
 ** @BRIEF	when check sd_card insert to system, we create file and get the record info from sd_card
 ** @PARAM	void  
 ** @RETURN	void 
 ** @RETVAL	
 **/

int anyka_dection_start_save_file(void)
{
    int first_time_stamp;
    T_STM_STRUCT *pvideo_stream, *paudio_stream;
    Psystem_alarm_set_info alarm_info = anyka_sys_alarm_info();

    anyka_print("[%s:%d] we will start save the alarm file!\n", __func__, __LINE__);
    video_fs_create_dir(ALARM_TMP_FILE_NAME);
    video_fs_create_dir(alarm_info->alarm_default_record_dir);

	if(anyka_dection_get_record_info() < 0)
	{
		anyka_print("[%s:%d] The SD_card room no enough, we don't start record\n",
					 __func__, __LINE__);
		return -1;
	}

    /************ sync video and audio *********************/
    first_time_stamp = -1;

    /************ we will lost p frame at first ***********/
    while(anyka_queue_not_empty(pvideo_move_ctrl->video_queue))
    {
        pvideo_stream = (T_STM_STRUCT *)anyka_queue_get_index_item(pvideo_move_ctrl->video_queue, 0);
        if(pvideo_stream == NULL)
        {
            break;
        }
        first_time_stamp = pvideo_stream->timestamp;
        if(pvideo_stream->iFrame)
        {
            break;
        }
        pvideo_stream = (T_STM_STRUCT *)anyka_queue_pop(pvideo_move_ctrl->video_queue);
        anyka_dection_video_move_queue_free((void *)pvideo_stream);
    }
    /******* lost the audio frame, before the first I frame ******/
    if(first_time_stamp != -1)
    {
        while(anyka_queue_not_empty(pvideo_move_ctrl->audio_queue))
        {
            paudio_stream = (T_STM_STRUCT *)anyka_queue_get_index_item(pvideo_move_ctrl->audio_queue, 0);
            if(paudio_stream == NULL)
            {
                break;
            }
            if(first_time_stamp <= paudio_stream->timestamp)
            {
                break;
            }
            paudio_stream = (T_STM_STRUCT *)anyka_queue_pop(pvideo_move_ctrl->audio_queue);
            anyka_dection_video_move_queue_free((void *)paudio_stream);
        }
    }
    pvideo_move_ctrl->rec_save_index = 0;
    pvideo_move_ctrl->rec_used_index = 0;
	/***** open the first record and index file *****/
	if(anyka_dection_creat_new_record_file(0) < 0)
	{
        anyka_print("[%s:%d] It fail to create the first record file!\n", __func__, __LINE__);
	}
	if(anyka_dection_creat_new_record_file(1) < 0)
	{
        anyka_print("[%s:%d] It fail to create the first record file!\n", __func__, __LINE__);
	}

	return 0;
	
}


/**
 ** NAME    anyka_dection_move_get_video_data
 ** @BRIEF	when check moving or other alarm, start get video data, open mux, add data to mux
 ** @PARAM	void * user, now no use
 ** @RETURN	void *
 ** @RETVAL	NULL
 **/

void *anyka_dection_record_handler(void *user)
{
	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());
    T_STM_STRUCT *pvideo_stream, *paudio_stream;
    uint32 cur_time, send_msg_time = 0, send_alarm_time = 0;
    int sd_ok = 1; //SD card status
    int sd_room = 1;
	Psystem_alarm_set_info alarm_info = anyka_sys_alarm_info();

	T_MUX_INPUT mux_input;    
	mux_input.m_MediaRecType = MEDIALIB_REC_3GP;	//MEDIALIB_REC_AVI_NORMAL;
	mux_input.m_eVideoType = MEDIALIB_VIDEO_H264;
	mux_input.m_nWidth = VIDEO_WIDTH_720P;
	mux_input.m_nHeight = VIDEO_HEIGHT_720P;
	//mux audio
	mux_input.m_bCaptureAudio = 1;
	mux_input.m_eAudioType = MEDIALIB_AUDIO_AMR;	//MEDIALIB_AUDIO_AAC;
	mux_input.m_nSampleRate = 8000;

    sd_ok = sd_get_status(); //get card status
    if(1 == sd_ok) //card is in
    {
        if (anyka_dection_start_save_file() < 0) {
			sd_room = 0;	//room not enough to start record
		}
    }
        
    while(pvideo_move_ctrl->run_flag)
    {
        sem_wait(&pvideo_move_ctrl->save_sem); //等待侦测信号才启动录像
        
        cur_time = pvideo_move_ctrl->cur_time;
        send_alarm_time = 0;
		if((pvideo_move_ctrl->move_ok == 0) || 
			(pvideo_move_ctrl->move_time + (alarm_info->alarm_move_over_time) < cur_time))
        {
            //anyka_print("[%s:%d] it pass the moving signal!\n", __func__, __LINE__);
            continue;
        }

#ifdef STOP_DECT_RECORD   //支持暂停侦测录像
        if (pvideo_move_ctrl->check_flag == 0)
            continue;
#endif        
		anyka_print("[%s:%d] move start time: %u, record time: %d, current time: %d\n", 
				 __func__, __LINE__, pvideo_move_ctrl->move_time , alarm_info->alarm_record_time, cur_time);
		
#ifdef STOP_DECT_RECORD   //支持暂停侦测录像
		while(pvideo_move_ctrl->run_flag && pvideo_move_ctrl->check_flag)
#else		
		while(pvideo_move_ctrl->run_flag)
#endif
		{
	        sem_wait(&pvideo_move_ctrl->data_sem); //等待音视频码流数据到来完成muxer合成

			/*
			** The distance from the most recent to detect anomaly time, 
			** plus the timeout still no exception, then stop the current mobile detection. 
			*/
	        cur_time = pvideo_move_ctrl->cur_time;
			if(pvideo_move_ctrl->move_time + (alarm_info->alarm_move_over_time) < cur_time)
	        {
	            anyka_print("[%s:%d] system detected there is no move. Current time: [%d], start time: [%d]!\n", 
							__func__, __LINE__, cur_time, pvideo_move_ctrl->move_time);
	            break;
	        }
            
	        while(pvideo_move_ctrl->run_flag &&
				(anyka_queue_not_empty(pvideo_move_ctrl->video_queue) || 
				 anyka_queue_not_empty(pvideo_move_ctrl->audio_queue))) {
				/*
				 *  mux main loop
				 */

	            //cur_time = time(0);  
	            cur_time = pvideo_move_ctrl->cur_time;
                #if 0
                if((send_alarm_time + alarm_info->alarm_send_msg_time < cur_time) 
						&& (pvideo_move_ctrl->Palarm_func)) {
                    send_alarm_time = cur_time;
                    if(0 == anyka_photo_start(1, 1, alarm_info->alarm_default_record_dir,
							   	anyka_dection_send_alarm_info, NULL)) {
                        if(pvideo_move_ctrl->Palarm_func) {
                            pvideo_move_ctrl->Palarm_func(pvideo_move_ctrl->alarm_type, 0, NULL, cur_time, 1);
                        }
                    }
                }   
                #endif
	            if (sd_get_status() == 0) { //T卡拔出 
                    if (sd_ok == 1) {
	                    sd_ok = 0;
						anyka_dection_free_record_info();
                        pvideo_move_ctrl->save_mux_handle = pvideo_move_ctrl->mux_handle;
                        pvideo_move_ctrl->mux_handle =  NULL;
                        sem_post(&pvideo_move_ctrl->manage_sem);
                    }
	            } else {		//T卡插入 
                    if (sd_ok == 0)
                    {
                        if(anyka_dection_start_save_file() < 0){
							sd_room = 0;	//room not enough to start record
                        }
                        sd_ok = 1;
                    }
				}

	            pvideo_stream = (T_STM_STRUCT *)anyka_queue_pop(pvideo_move_ctrl->video_queue);
	            if(pvideo_stream) {
    				/**** 侦测录像时长根据I帧的时间戳来进行分割，达到alarm_record_time就进行分割 ****/
                    if(pvideo_stream->iFrame)
                    {
                        cur_time = pvideo_stream->timestamp / 1000;
						/*
						 * according to tf card status and record time to decide start new mux add work
						 */
                        if((sd_ok == 1) && (pvideo_move_ctrl->mux_handle != NULL) && (sd_room == 1) &&
							(cur_time >= send_msg_time + (alarm_info->alarm_record_time))) {
                             
                            /***** change mux handle and index handle  *****/
                            pvideo_move_ctrl->rec_save_index = pvideo_move_ctrl->rec_used_index;
                            pvideo_move_ctrl->save_mux_handle = pvideo_move_ctrl->mux_handle;
                            pvideo_move_ctrl->rec_used_index = pvideo_move_ctrl->rec_used_index ? 0 : 1;
                            anyka_print("[%s:%d] using index exchange, now using handle :%d\n",
								   		 __func__, __LINE__, pvideo_move_ctrl->rec_used_index);
                        
                            pvideo_move_ctrl->mux_handle =  NULL;
                            sem_post(&pvideo_move_ctrl->manage_sem);
                        }
						/*
						 * open mux, start mux add
						 */
    					if ((sd_ok == 1) && (sd_room == 1) && (pvideo_move_ctrl->mux_handle == NULL)) {
     			            pvideo_move_ctrl->mux_handle = mux_open(&mux_input, 
    							pvideo_move_ctrl->record_file[pvideo_move_ctrl->rec_used_index].record_fd, 
    							pvideo_move_ctrl->record_file[pvideo_move_ctrl->rec_used_index].index_fd);
                      
                            send_msg_time = pvideo_stream->timestamp / 1000;
    					}
                    }

	                if (pvideo_stream->iFrame) {
	                    pthread_mutex_lock(&pvideo_move_ctrl->move_mutex);
                        if(pvideo_move_ctrl->iframe_num) {
	                        pvideo_move_ctrl->iframe_num --;
                        }
	                    pthread_mutex_unlock(&pvideo_move_ctrl->move_mutex);
	                }
                    if (mux_addVideo(pvideo_move_ctrl->mux_handle, pvideo_stream->buf, pvideo_stream->size, 
						pvideo_stream->timestamp, pvideo_stream->iFrame) <  0) {
                        video_set_iframe(pvideo_move_ctrl);
                        //now do nothing
                    }
	                anyka_dection_video_move_queue_free((void *)pvideo_stream);
	            }
	            
	            paudio_stream = (T_STM_STRUCT *)anyka_queue_pop(pvideo_move_ctrl->audio_queue);
	            if (paudio_stream) {
                    if (mux_addAudio(pvideo_move_ctrl->mux_handle, paudio_stream->buf, paudio_stream->size, 
									paudio_stream->timestamp) < 0) {
						//now do nothing
                    }
                	anyka_dection_video_move_queue_free((void *)paudio_stream);
            	}
        	}
        }

        /*
            此处必须判断当前有没有在做前一次分割文件的保存动作，
            需要等到前一次分割文件保存完成(即save_mux_handle变为空)，再做当前录像的保存动作。
        */
        while (pvideo_move_ctrl->save_mux_handle != NULL)
            usleep(100*1000);

        //侦测过程结束，保存当前录制的文件
        pvideo_move_ctrl->move_ok = 0;
        pvideo_move_ctrl->rec_save_index = pvideo_move_ctrl->rec_used_index;
        pvideo_move_ctrl->save_mux_handle = pvideo_move_ctrl->mux_handle;
        pvideo_move_ctrl->rec_used_index = pvideo_move_ctrl->rec_used_index ? 0 : 1; //新录像文件index切换
        pvideo_move_ctrl->mux_handle =  NULL;
        sem_post(&pvideo_move_ctrl->manage_sem);
        
        anyka_print("\n[%s:%d]### The alarming is over, Please check the record!\n", __func__, __LINE__);
    }
    anyka_print("[%s:%d] It exit, id : %ld\n", __func__, __LINE__, (long int)gettid());

	return NULL;	
}



/**
 * NAME         anyka_dection_audio_voice_data
 * @BRIEF	声音侦测逻辑
 * @PARAM	
 
 * @RETURN	void
 * @RETVAL	
 */

int anyka_dection_audio_voice_data(void *pbuf, int len, int valve)
{
    sint16 *pdata = (T_S16 *)pbuf;
    int count = len / 32;
    int i;
    int average = 0;
    sint16 temp;
    sint32 DCavr = 0;
    static sint32 DCValue = 0;
    static uint32 checkCnt = 0;

    
    //caculte direct_current value
    if (checkCnt < 2)  
    {
        //防止开始录音打压不稳定，开始两帧不参与直流偏移的计算
        checkCnt ++;
    }
    else if (checkCnt <= 20)
    {
    //考虑到硬件速度，只让3到20帧的数据参与直流偏移的计算
        checkCnt ++;
        for (i=0; i<len/2; i++)
        {
            DCavr += pdata[i];
        }
        DCavr /= (signed)(len/2);
        DCavr += DCValue;
        DCValue = (T_S16)(DCavr/2);
    }
    // spot check data value
    for (i = 0; i < count; i++)
    {
        temp = pdata[i*16];
        temp = (T_S16)(temp - DCValue);
        if (temp < 0)
        {
            average += (-temp);
        }
        else
        {
            average += temp;
        }
    }
    average /= count;
    if (average < valve)
    {
        return AK_FALSE;
    }
    else
    {
        return AK_TRUE;
    }
}



/**
 * NAME         anyka_dection_add_voice_check
 * @BRIEF	声音侦测音频数据回调
 * @PARAM	
 * @RETURN	void
 * @RETVAL	
 */

void anyka_dection_audio_get_pcm_data(void *param, T_STM_STRUCT *pstream)
{
    static int send_alarm_time = 0;
    
    if(pvideo_move_ctrl->check_flag == 0)
    {
        anyka_print("[%s:%d] we will pass the voice checking!\n", __func__, __LINE__);
        return ;
    }

    if(anyka_dection_audio_voice_data(pstream->buf, pstream->size, pvideo_move_ctrl->audio_level))
    {
        Psystem_alarm_set_info alarm_info = anyka_sys_alarm_info();	//get system alamr info
        //如果首次移动，将清除部分以前的数据。
        anyka_print("[%s:%d] we check the voice!\n", __func__, __LINE__);
        if(alarm_info->alarm_send_type == 0)
        {
            pvideo_move_ctrl->move_ok = 1;
            pvideo_move_ctrl->alarm_type = SYS_CHECK_VOICE_ALARM;
            pvideo_move_ctrl->move_time = time(0);
            sem_post(&pvideo_move_ctrl->save_sem);      //发出信号通知启动录像线程
            if((send_alarm_time + alarm_info->alarm_send_msg_time < pvideo_move_ctrl->cur_time) && (pvideo_move_ctrl->Palarm_func))
            {
                send_alarm_time = pvideo_move_ctrl->cur_time;
				/** take photos, send alarm to phone app **/
                if(0 == anyka_photo_start(1, 1, ALARM_TMP_PHOTO_DIR, anyka_dection_send_alarm_info, NULL))
                {
                    if(pvideo_move_ctrl->Palarm_func)	//call this callback send message
                    {
                        pvideo_move_ctrl->Palarm_func(pvideo_move_ctrl->alarm_type, 0, NULL, pvideo_move_ctrl->cur_time, 1);
                    }
                }
            }
        }
    }
}


/**
 * NAME         anyka_dection_add_voice_check
 * @BRIEF	打开移动侦测功能
 * @PARAM	move_level 侦测的标准
 * @RETURN	void
 * @RETVAL	
 */

void check_move_add_video_check(int move_level)
{
    uint16 i, ratios=100, Sensitivity[65];
    T_MOTION_DETECTOR_DIMENSION detection_pos;
    Psystem_alarm_set_info alarm = anyka_sys_alarm_info();	//get system alamr info

    move_level --;    
    switch(move_level)
    {
        case 0:
        {
            ratios = alarm->motion_detection_1;
            break;
        }
        case 1:
        {
            ratios = alarm->motion_detection_2;
            break;
        }
        case 2:
        {
            ratios = alarm->motion_detection_3;
            break;
        }       
    }
    
    for(i = 0; i < 65; i++)
    {
        Sensitivity[i] = ratios;
    }
	//分割图像为motion_size_x * motion_size_y 块
    detection_pos.m_uHoriNum = alarm->motion_size_x;
    detection_pos.m_uVeriNum = alarm->motion_size_y;
	/** start video move check func **/
	if(video_start_move_check(VIDEO_HEIGHT_VGA, VIDEO_WIDTH_VGA, &detection_pos, Sensitivity, alarm->alarm_interval_time, anyka_dection_video_notice_dowith, NULL, anyka_dection_pass_video_check) == 0)
    {
        pvideo_move_ctrl->check_type &= ~SYS_CHECK_VIDEO_ALARM;
    }
    else
    {
        pvideo_move_ctrl->check_type |= SYS_CHECK_VIDEO_ALARM;
    }
    pvideo_move_ctrl->video_level = ratios;
}

/**
 * NAME         anyka_dection_add_voice_check
 * @BRIEF	打开声音侦测功能
 * @PARAM	move_level 侦测的标准
 * @RETURN	void
 * @RETVAL	
 */

void anyka_dection_add_voice_check(int move_level)
{
    Psystem_alarm_set_info alarm = anyka_sys_alarm_info(); //get system alamr info
    uint16 ratios=10;
        
    move_level --;
    switch(move_level)
    {
        case 0:
        {
            ratios = alarm->opensound_detection_1;
            break;
        }
        case 1:
        {
            ratios = alarm->opensound_detection_2;
            break;
        }
        case 2:
        {
            ratios = alarm->opensound_detection_3;
            break;
        }       
    }
    pvideo_move_ctrl->audio_level = ratios;     

	/** add audio **/
    if(-1 == audio_add(SYS_AUDIO_RAW_PCM, anyka_dection_audio_get_pcm_data, (void *)pvideo_move_ctrl))
    {
        pvideo_move_ctrl->check_type &= ~SYS_CHECK_VOICE_ALARM;
    }
    else
    {
        pvideo_move_ctrl->check_type |= SYS_CHECK_VOICE_ALARM;
    }
}





/**
 * NAME         anyka_dection_start
 * @BRIEF	打开侦测功能
 * @PARAM	move_level 侦测的标准
                    check_type 侦测类型
                    Palarm_func 侦测回调函数
                    filter_check   当前是否过滤侦测的，目前设计腾讯的在视频观看的时候不做侦测
 * @RETURN	void
 * @RETVAL	
 */

void anyka_dection_start(int move_level, int check_type, PANYKA_SEND_ALARM_FUNC Palarm_func, PANYKA_FILTER_VIDEO_CHECK_FUNC filter_check)
{	
	pthread_t alarm_pid;
    Psystem_alarm_set_info alarm_info = anyka_sys_alarm_info();	//get system alamr info
	video_setting * pvideo_record_setting = anyka_get_sys_video_setting();

    if(move_level > 2)
    {
        move_level = 2;
    }
    
    if(check_type == 0) //unsupported type
    {
        return;
    }
    
    if(pvideo_move_ctrl)
    {
        if(pvideo_move_ctrl->check_type & check_type) //current check type has been opened.
        {
            anyka_print("[%s:%d] it fails to start because it has runned!\n", __func__, __LINE__);
            return ;
        }
        if(check_type & SYS_CHECK_VOICE_ALARM) //add voice check
        {
            anyka_print("[%s:%d] it will start voice check!\n", __func__, __LINE__);
            anyka_dection_add_voice_check(move_level); //voice check entry
        }
        if(check_type & SYS_CHECK_VIDEO_ALARM) //add video check
        {
            anyka_print("[%s:%d] it will start video check!\n", __func__, __LINE__);
            check_move_add_video_check(move_level);  //video check entry
        }
		if(check_type & SYS_CHECK_OTHER_ALARM) //add other check, now use gpio check
		{
		    anyka_print("[%s:%d] it will start other check!\n", __func__, __LINE__);
			/** gpio alarm check entry **/
			anyka_pthread_create(&alarm_pid, (anyka_thread_main *)anyka_dection_alarm, (void *)NULL,
									ANYKA_THREAD_MIN_STACK_SIZE, -1);
		}
        
        return;
    }
    
	/*** create temp directory, now is : /mnt/tmp/ ***/
    video_fs_create_dir(ALARM_TMP_FILE_NAME); 

	/*** create save record file directory according to the ini file setting ***/
    video_fs_create_dir(alarm_info->alarm_default_record_dir); 
	
	/*** initialize the main handle ***/
    pvideo_move_ctrl = (Pvideo_check_move_ctrl)malloc(sizeof(video_check_move_ctrl));
    if(pvideo_move_ctrl == NULL)
    {
        anyka_print("[%s:%d] it fail to malloc!\n", __func__, __LINE__);
        return;
    }
    memset(pvideo_move_ctrl, 0, sizeof(video_check_move_ctrl));

    pvideo_move_ctrl->filter_check = filter_check;
    pvideo_move_ctrl->Palarm_func = Palarm_func; //alarm call back, use to send message to the phone or PC
	pvideo_move_ctrl->mux_handle = NULL; //mux handle, use to open mux lib, add video and audio data to the mux lib
	pvideo_move_ctrl->save_mux_handle = NULL; //save muxed file handle
	pvideo_move_ctrl->check_type = 0; //clear check type
	pvideo_move_ctrl->rec_used_index = 0; //current record used index
	pvideo_move_ctrl->save_record_flag = alarm_info->alarm_save_record;
	
	/*** initialize three semaphore and one thread mutex locker ***/
	/*** according to different type to start respectively check handle ***/
    if(check_type & SYS_CHECK_VIDEO_ALARM) //video check 
    {
        check_move_add_video_check(move_level);//video check entry
    }
    
    if(check_type & SYS_CHECK_VOICE_ALARM) //voice check
    {
        anyka_dection_add_voice_check(move_level);//add voice check
    }
	if(check_type & SYS_CHECK_OTHER_ALARM) //other type check
	{
	    anyka_print("[%s:%d] it will start other check!\n", __func__, __LINE__);
		anyka_pthread_create(&alarm_pid, (anyka_thread_main *)anyka_dection_alarm, (void *)NULL,
								ANYKA_THREAD_MIN_STACK_SIZE, -1);
	}

    pvideo_move_ctrl->run_flag = 1; //set this module run flag 
    pvideo_move_ctrl->check_flag = 1;
    pvideo_move_ctrl->iframe_num = 0; //clear key frame total number
    pvideo_move_ctrl->move_ok = 0; //clear moving or voice or other abnormal phenomena has been checked flag
	pvideo_move_ctrl->cur_time = 0; //clear current time, used to control the record time
    pthread_mutex_init(&pvideo_move_ctrl->move_mutex, NULL); 

	/** Save record flag is true, then init some val **/
    if(pvideo_move_ctrl->save_record_flag)	
    {
        sem_init(&pvideo_move_ctrl->data_sem, 0, 0);
        sem_init(&pvideo_move_ctrl->save_sem, 0, 0);
        sem_init(&pvideo_move_ctrl->manage_sem, 0, 0);
        
    	/**** initialize three queue ****/
        pvideo_move_ctrl->video_queue = anyka_queue_init(100);
        pvideo_move_ctrl->audio_queue = anyka_queue_init(200);
        if((pvideo_move_ctrl->video_queue == NULL) || ( pvideo_move_ctrl->audio_queue == NULL))
        {
            goto ERR_video_check_move_start;
        }

    	/*** alarm_send_type = 0, record model, alarm_send_type = 1, photograph model ***/
        if(alarm_info->alarm_send_type != 1) //record model 
        {
			/** add video get data **/
            video_add(anyka_dection_move_video_data, (void *)pvideo_move_ctrl, FRAMES_ENCODE_RECORD, 
            	pvideo_record_setting->savefilekbps);
			/** add audio get data **/
            audio_add(SYS_AUDIO_ENCODE_AMR, anyka_dection_video_move_audio_data, (void *)pvideo_move_ctrl);
        }
    	/**** create get video data thread, the thread wait for semaphore, than open mux and add data ****/
    	if(anyka_pthread_create(&(pvideo_move_ctrl->id), anyka_dection_record_handler,
    							(void *)pvideo_move_ctrl, ANYKA_THREAD_NORMAL_STACK_SIZE, -1) != 0){
    		anyka_print("[%s:%d] it fail to create thread!\n", __func__, __LINE__);
    		goto ERR_video_check_move_start;
    	}
    	/****** create save alarm record file thread *****/
    	if(anyka_pthread_create(&(pvideo_move_ctrl->save_file_id), anyka_dection_manage_file,
    							(void *)pvideo_move_ctrl, ANYKA_THREAD_NORMAL_STACK_SIZE, -1) != 0){
    		anyka_print("[%s:%d] it fail to create thread!\n", __func__, __LINE__);
    		goto ERR_video_check_move_start;
    	}
    }

	return ;
	
ERR_video_check_move_start:

	/** release the resource  **/
    if(pvideo_move_ctrl->save_record_flag)
    {
        video_del(pvideo_move_ctrl);
        audio_del(SYS_AUDIO_ENCODE_AMR, pvideo_move_ctrl);
        anyka_queue_destroy(pvideo_move_ctrl->video_queue, anyka_dection_video_move_queue_free);
        anyka_queue_destroy(pvideo_move_ctrl->audio_queue, anyka_dection_video_move_queue_free);
        sem_destroy(&pvideo_move_ctrl->data_sem);
        sem_destroy(&pvideo_move_ctrl->save_sem);
        sem_destroy(&pvideo_move_ctrl->manage_sem);
    }
    
    anyka_pthread_mutex_destroy(&pvideo_move_ctrl->move_mutex); 
    free(pvideo_move_ctrl);
    pvideo_move_ctrl = NULL;
    return ;
    
}

/**
 * NAME         anyka_dection_stop
 * @BRIEF	关闭相应的侦测类型,如果所有侦测全被关闭，我们将释放所有资源
 * @PARAM	check_type
 * @RETURN	void
 * @RETVAL	
 */

void anyka_dection_stop(int check_type)
{
    if(pvideo_move_ctrl == NULL)
    {
        return;
    }
    if(check_type & SYS_CHECK_VIDEO_ALARM)
    {
        //关闭移动侦测
        if(pvideo_move_ctrl->check_type & SYS_CHECK_VIDEO_ALARM)
        {
            video_stop_move_check();
            pvideo_move_ctrl->check_type &= ~SYS_CHECK_VIDEO_ALARM;
            anyka_print("[%s:%d] It will stop video check!\n", __func__, __LINE__);
        }
    }
    if(check_type & SYS_CHECK_VOICE_ALARM)
    {
        //关闭声音侦测
        if(pvideo_move_ctrl->check_type & SYS_CHECK_VOICE_ALARM)
        {
            audio_del(SYS_AUDIO_RAW_PCM, (void *)pvideo_move_ctrl);
            pvideo_move_ctrl->check_type &= ~SYS_CHECK_VOICE_ALARM;
            anyka_print("[%s:%d] It will stop voice check!\n", __func__, __LINE__);
        }
    }
	if(check_type & SYS_CHECK_OTHER_ALARM)
	{
	    if(pvideo_move_ctrl->check_type & SYS_CHECK_OTHER_ALARM)
        {
            //audio_del(SYS_AUDIO_RAW_PCM, (void *)pvideo_move_ctrl);
            pvideo_move_ctrl->check_type &= ~SYS_CHECK_OTHER_ALARM;
            anyka_print("[%s:%d] It will stop voice check!\n", __func__, __LINE__);
        }
	}
    if(pvideo_move_ctrl->check_type == 0)
    {
        //如果所有侦测全部关闭后，将释放资源
        pvideo_move_ctrl->run_flag = 0;
        if(pvideo_move_ctrl->save_record_flag)
        {
            video_del(pvideo_move_ctrl);
            audio_del(SYS_AUDIO_ENCODE_AMR, pvideo_move_ctrl);

            sem_post(&pvideo_move_ctrl->data_sem);
            sem_post(&pvideo_move_ctrl->save_sem);
            pthread_join(pvideo_move_ctrl->id, NULL);	
            sem_post(&pvideo_move_ctrl->manage_sem);	
            pthread_join(pvideo_move_ctrl->save_file_id, NULL);

            pthread_mutex_lock(&pvideo_move_ctrl->move_mutex);

            anyka_queue_destroy(pvideo_move_ctrl->video_queue, anyka_dection_video_move_queue_free);
            anyka_queue_destroy(pvideo_move_ctrl->audio_queue, anyka_dection_video_move_queue_free);
            sem_destroy(&pvideo_move_ctrl->data_sem);
            sem_destroy(&pvideo_move_ctrl->save_sem);
    		sem_destroy(&pvideo_move_ctrl->manage_sem);
            anyka_dection_remove_tmp_file();
        }

        anyka_pthread_mutex_destroy(&pvideo_move_ctrl->move_mutex); 
		
		anyka_dection_free_record_info();	

        free(pvideo_move_ctrl);
        pvideo_move_ctrl = NULL;
        anyka_print("[%s:%d] It will release check!\n", __func__, __LINE__);
    }
}



/**
 * NAME         anyka_dection_pass_video_check
 * @BRIEF	是否忽略侦测的回调，目前只有腾讯平台使用，在观看视频时，不做侦测
 * @PARAM	
 * @RETURN	0 :不做逻辑修改，1:不检测，但是默认发送侦测消息 2: 不检测，也不发送侦测信息
 * @RETVAL	
 */
int anyka_dection_pass_video_check(void)
{
    int flag;

    if (pvideo_move_ctrl == NULL)
        return 2;
        
    if(pvideo_move_ctrl->filter_check)
    {
        flag = pvideo_move_ctrl->filter_check();

        if(flag)
        {
            if(pvideo_move_ctrl->move_ok == 0)
            {
                return 2;
            }
            else
            {
                return 1;
            }
        }
    }
    return 0;
}

/**
 * NAME         anyka_dection_init
 * @BRIEF	系统启动时，读取配置文件，看关机前是否设置侦测，如果设置
                    启动相应功能,因侦测到了要发消息出去，目前只支持大拿，如果其它
                    平台，修改回调函数就可以
 * @PARAM	Palarm_func  如果发生侦测时的回调函数
                    filter_check   当前是否过滤侦测的，目前设计腾讯的在视频观看的时候不做侦测
 * @RETURN	void
 * @RETVAL	
 */


void anyka_dection_init(PANYKA_SEND_ALARM_FUNC Palarm_func, PANYKA_FILTER_VIDEO_CHECK_FUNC filter_check)
{	
    Psystem_alarm_set_info alarm = anyka_sys_alarm_info();
	
	anyka_print("[%s] ####\n", __func__);

    if(alarm->motion_detection)
    {
        //启动移动侦测
        anyka_dection_start(alarm->motion_detection - 1, SYS_CHECK_VIDEO_ALARM, Palarm_func, filter_check);
    }
    if(alarm->opensound_detection)
    { 
        //启动声音侦测
        anyka_dection_start(alarm->opensound_detection - 1, SYS_CHECK_VOICE_ALARM, Palarm_func, filter_check);
    }
	if(alarm->other_detection){ 
		anyka_print("[%s:%d] other detection \n", __func__, __LINE__);
		anyka_dection_start(alarm->other_detection - 1, SYS_CHECK_OTHER_ALARM, Palarm_func, filter_check);   
    }
	
}

/**
 * NAME         anyka_dection_save_record
 * @BRIEF	侦测录像是否开始
 * @PARAM	
 * @RETURN	1-->正在侦测录像; 0-->侦测录像未开始
 * @RETVAL	
 */

int anyka_dection_save_record(void)
{
    return pvideo_move_ctrl != NULL;
}


/**
 * NAME         anyka_dection_pause_dection
 * @BRIEF	停止移动与声音侦测
 * @PARAM	
 * @RETURN	
 * @RETVAL	
 */

void anyka_dection_pause_dection(void)
{
    if(pvideo_move_ctrl && pvideo_move_ctrl->check_flag)
    {
#ifdef STOP_DECT_RECORD   //支持暂停侦测录像
        //stop audio&video codec
        video_del(pvideo_move_ctrl);
        audio_del(SYS_AUDIO_ENCODE_AMR, pvideo_move_ctrl);
        sem_post(&pvideo_move_ctrl->data_sem);
#endif
        pvideo_move_ctrl->check_flag = 0;
    }
}

/**
 * NAME         anyka_dection_resume_dection
 * @BRIEF	恢复移动与声音侦测
 * @PARAM	
 * @RETURN	
 * @RETVAL	
 */

void anyka_dection_resume_dection(void)
{
    if(pvideo_move_ctrl && pvideo_move_ctrl->check_flag == 0)
    {
        pvideo_move_ctrl->check_flag = 1;

#ifdef STOP_DECT_RECORD   //支持暂停侦测录像
		/** add video get data **/
        video_add(anyka_dection_move_video_data, (void *)pvideo_move_ctrl, FRAMES_ENCODE_RECORD, 15);
        /** add audio get data **/
        audio_add(SYS_AUDIO_ENCODE_AMR,anyka_dection_video_move_audio_data,(void *)pvideo_move_ctrl);
#endif
    }
}


