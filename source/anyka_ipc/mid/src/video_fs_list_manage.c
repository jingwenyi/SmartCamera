#include "common.h"
#include "video_fs_manage.h"
#include "video_fs_list_manage.h"


static void *record_replay_queue = NULL;

  

//所有录像应用对应的录像列表文件,供回放使用
static char g_video_list_file_name[RECORD_APP_NUM][32]=
{
	{"/tmp/record.list"},
	{"/tmp/alarm.list"}
};

static void video_fs_list_free_file_queue(void *item)
{

    if(item)
    {
        free(item);
    }
}


static Precord_replay_info video_fs_list_malloc_file_queue(int file_len)
{    
    return (Precord_replay_info )malloc(sizeof(record_replay_info));
}

/**
 * NAME         video_fs_list_compare_record_name
 * @BRIEF	比较两个记录文件名大小
  
 * @PARAM    item1
                    item2
 * @RETURN	比较结果
 * @RETVAL	
                   <0 item1<item2,
                   =0 item1=item2
                   >0 item1>item2
                   这里的item指的是item最终指向的文件名
 */

static int video_fs_list_compare_record_name(void *item1, void *item2)
{
    Precord_replay_info precord1 = (Precord_replay_info)item1;
    Precord_replay_info precord2 = (Precord_replay_info)item2;

    return strcmp(precord1->file_name, precord2->file_name);
    
}

/**
 * NAME         video_fs_list_get_info
 * @BRIEF	得到相应文件的录像时长
  
 * @PARAM    file_name  
                   type          应用类型(计划录像、告警录像等)
 * @RETURN	文件的录像相关信息
 * @RETVAL	
 */

static Precord_replay_info video_fs_list_get_info(char *file_name , int type)
{
    int file_len;   
    Precord_replay_info pnew_record;
	struct tm time_st;
	char  s_time[6][10];
	const video_file_name_info * cfg;
	char file_name_fmt[100];
	int i,namepos;
	int record_time;
	
    file_len = strlen(file_name);
    record_time = video_demux_get_total_time(file_name);
    if (record_time <=0)
    	return NULL;
    pnew_record = video_fs_list_malloc_file_queue(file_len + 4);
    pnew_record->record_time =record_time;

	//下面从文件名获取视频起始时间
	cfg = video_fs_get_file_name_cfg();
	//构造取时间的格式化串 for CYC_DV_20150908-090511.mp4
	sprintf(file_name_fmt, "%s%%4c%%2c%%2c-%%2c%%2c%%2c%s", cfg[type].prefix,cfg[type].posfix);
	memset(s_time, 0, sizeof(s_time));	
	//去掉文件名路径
	for(i=0 ;i< file_len;i++)
	{
		if (file_name[file_len-1-i] =='/')
			break;
	}
	namepos = file_len -i;
	sscanf(file_name+namepos, file_name_fmt,
		s_time[0],s_time[1],s_time[2],s_time[3],s_time[4],s_time[5]);
	time_st.tm_year = atoi(s_time[0]) -1900;
	time_st.tm_mon  = atoi(s_time[1]) -1;
	time_st.tm_mday = atoi(s_time[2]);
	time_st.tm_hour = atoi(s_time[3]);
	time_st.tm_min  = atoi(s_time[4]);
	time_st.tm_sec  = atoi(s_time[5]);
	time_st.tm_isdst= 8;

	pnew_record->recrod_start_time = mktime(&time_st);
	
	
	
    strcpy(pnew_record->file_name, file_name);
    pnew_record->record_time = pnew_record->record_time / 1000;
    
    return pnew_record;
}

/**
 * NAME         video_fs_list_insert_item
 * @BRIEF	将指定的文件插入到队列
  
 * @PARAM    file_name   文件名
                    file_size    文件大小
                    type        应用类型(计划录像、告警录像等)
 * @RETURN	NONE
 * @RETVAL	
 */

static void video_fs_list_insert_item(char *file_name, int file_size,int type)
{
    Precord_replay_info pnew_record;
    pnew_record = video_fs_list_get_info(file_name,type);

    if (pnew_record ==NULL)
    	return ;
    if(pnew_record->recrod_start_time < 1418954000) //只取2014-12-19 后的文件
    {
        video_fs_list_free_file_queue(pnew_record);
        return ;
    }
    
    if(0 == anyka_queue_push(record_replay_queue, (void *)pnew_record))
    {
        video_fs_list_free_file_queue(pnew_record);
    }
}


/**
 * NAME         video_fs_list_init_list
 * @BRIEF	统计指定目录下的录像文件，并将住处保存在临时文件中
  
 * @PARAM    in_dir         目录信息         
                    save_file    保存文件名
                    type: 0-->video record, 1-->alarm record
 * @RETURN	NONE
 * @RETVAL	
 */
static void video_fs_list_init_list(char * in_dir, char *save_file, unsigned short type)
{
    char *file_buf;
    int file_record_list;
    Precord_replay_info pcur_record;
	
    file_record_list = open(save_file,  O_RDWR | O_CREAT | O_TRUNC );
    if(file_record_list < 0)
    {
        return ;
    }
    file_buf = (char *)malloc(1024);
    record_replay_queue = anyka_queue_init(1024*10);
	if(record_replay_queue == NULL)
	{
		anyka_print("[%s:%d] Fails to init the queue:%s\n", __func__, __LINE__, "record_replay_queue");
	}

	video_fs_init_record_list(in_dir, video_fs_list_insert_item, type);    
	
    anyka_queue_sort(record_replay_queue, video_fs_list_compare_record_name);

    while(anyka_queue_not_empty(record_replay_queue))
    {
        pcur_record = anyka_queue_pop(record_replay_queue);
        if(pcur_record)
        {
            sprintf(file_buf, "file:%s;start:%ld;time:%ld\n", pcur_record->file_name, pcur_record->recrod_start_time, pcur_record->record_time);
            write(file_record_list, file_buf, strlen(file_buf));
            video_fs_list_free_file_queue(pcur_record);
        }
    }
    anyka_queue_destroy(record_replay_queue, video_fs_list_free_file_queue);
    free(file_buf);
    close(file_record_list);
    record_replay_queue = NULL;
}

/**
 * NAME         video_fs_list_init
 * @BRIEF	应用程序启动时，创建录像文件列表
  
 * @PARAM    user                
 * @RETURN	NONE
 * @RETVAL	
 */

//void record_replay_test();
void video_fs_list_init(  )
{
	video_setting * pvideo_record_setting = anyka_get_sys_video_setting();    
    Psystem_alarm_set_info alarm_info = anyka_sys_alarm_info();

    video_fs_list_init_list(pvideo_record_setting->recpath, (char *)g_video_list_file_name[RECORD_APP_PLAN], RECORD_APP_PLAN); //video
    video_fs_list_init_list(alarm_info->alarm_default_record_dir, (char *)g_video_list_file_name[RECORD_APP_ALARM], RECORD_APP_ALARM);	//alarm
    /***************************************************
       *****************************************************/
}

/**
 * NAME         video_fs_list_insert_file
 * @BRIEF	添加一种类型的录像文件，一般在计划录像或报警录像调用
  
 * @PARAM    type                回放的类型
                    file_name        录像文件名
 * @RETURN	NONE
 * @RETVAL	
 */

void video_fs_list_insert_file(char *file_name, int type)
{
    char *file_buf;
    int record_list;
    Precord_replay_info pcur_record;
    char *save_file;
    
//    anyka_print("[%s,%d]it inserts record list(name:%s; type:%d)!\n", __FUNCTION__, __LINE__, file_name, type);
    save_file = (char *)&g_video_list_file_name[type];
    
    record_list = open(save_file,  O_RDWR | O_APPEND);
    if(record_list < 0)
    {
        anyka_print("[%s,%d]it fails to open record list(%s)!\n", __FUNCTION__, __LINE__, save_file);
        return ;
    }
    pcur_record = video_fs_list_get_info(file_name,type);
    if(pcur_record)
    {
	    if(pcur_record->recrod_start_time >= 1418954000) //只取2014-12-19 后的文件
	    {
        file_buf = (char *)malloc(1024);
        
        sprintf(file_buf, "file:%s;start:%ld;time:%ld\n", pcur_record->file_name, pcur_record->recrod_start_time, pcur_record->record_time);
        write(record_list, file_buf, strlen(file_buf));
        
        free(file_buf);
      }
        video_fs_list_free_file_queue(pcur_record);
    }
    close(record_list);
}

/**
 * NAME         video_fs_list_remove_file
 * @BRIEF	删除一种类型的录像文件，一般在计划录像或报警录像调用
  
 * @PARAM    type                回放的类型
                    file_name        录像文件名
 * @RETURN	NONE
 * @RETVAL	
 */

void video_fs_list_remove_file(char *file_name, int type)
{
    char *file_buf;
    FILE * record_list, *new_record;
    char *save_file;
    int file_len = strlen(file_name);

    save_file = (char *)&g_video_list_file_name[type];
    
    record_list = fopen(save_file,  "r");
    if(record_list == NULL)
    {
        return ;
    }
    new_record = fopen("/tmp/tmp.txt",  "w");
    if(new_record == NULL)
    {
        fclose(record_list);
        return ;
    }
    file_buf = (char *)malloc(1024);
    
    while(!feof(record_list))
    {
        if(fgets(file_buf, 1024, record_list) == NULL)
        {
            break;
        }
        if(strstr(file_buf,"file:"))
        {
            if(memcmp(file_buf + 5, file_name, file_len) == 0)
            {
                continue;
            }
        }
        fputs(file_buf, new_record);
    }        
    
    fclose(record_list);
    fclose(new_record);
    remove(save_file);
    rename("/tmp/tmp.txt", save_file);
    free(file_buf);
}


static void video_fs_list_format_info(char *buf, char *file_name, unsigned long *start, unsigned long *cnt)
{
    char *find;

    find = strstr(buf, "file:");
    if(find)
    {
        find += 5;
        while(*find && *find != ';')
        {
            *file_name = *find;
            file_name ++;
            find ++;
        }
        *file_name = 0;
    }
    find = strstr(buf, "start:");
    if(find)
    {
        find += 6;
        *start = atoi(find);
    }
    find = strstr(buf, "time:");
    if(find)
    {
        find += 5;
        *cnt = atoi(find);
    }
}

/**
 * NAME         video_fs_list_load_list
 * @BRIEF	 加载指定类型的所有记录，并按文件名进行排序
  
 * @PARAM    type                回放的类型
 * @RETURN	返回符合回放的记录指针
 * @RETVAL	
 */

static Precord_replay_info video_fs_list_load_list(int type)
{
    char *file_buf;
    FILE * record_list;
    Precord_replay_info cur, head = NULL, tail = NULL;
    char *save_file;
    unsigned long record_end_time;
    
    save_file = (char *)&g_video_list_file_name[type];

    record_list = fopen(save_file,  "r");
    if(record_list == NULL)
    {
        return NULL;
    }
    file_buf = (char *)malloc(1024);
    
    while(!feof(record_list))
    {
        if(fgets(file_buf, 1024, record_list) == NULL)
        {
            break;
        }
        if(strstr(file_buf,"file:"))
        {
        	//get one frame info store in cur
            cur = (Precord_replay_info)malloc(sizeof(record_replay_info));
            video_fs_list_format_info(file_buf, cur->file_name, &cur->recrod_start_time, &cur->record_time);
            cur->next = NULL;	//next frame
            if(head == NULL)
            {
                head = cur;
            }
            else
            {
            	//前后片段去重, 通过减少录制时间使结束时间提前
            	record_end_time = tail->recrod_start_time + tail->record_time;
            	
            	if (cur->recrod_start_time <= record_end_time)
            	{
            		tail->record_time -= cur->recrod_start_time - record_end_time + 1;
            	}
                tail->next = cur;
            }
            tail = cur;
        }
    }        
    
    free(file_buf);
    fclose(record_list);
    return head;	//return the frame info
}

/**
 * NAME         video_fs_list_check_record
 * @BRIEF	 检查当前记录是否在指定时间内
  
 * @PARAM    start_time        回放的起始时间
                    end_time         回放的结束时间
                    cur                  当前检查的记录
 * @RETURN	1-->是在区间内，0-->不是在区间内
 * @RETVAL	
 */

static int video_fs_list_check_record(unsigned long  start_time, unsigned long end_time, Precord_replay_info cur)
{
	/** 分别保存当前录像文件的起始时间和结束时间 **/
    int record_start, record_end;

    record_start = cur->recrod_start_time;
    record_end = cur->recrod_start_time + cur->record_time;

	/** 不在区间内 **/
    if(record_start >= end_time ||  record_end <= start_time)
    {
        return 0;
    }/** 在区间内的 **/
    else if(record_start < start_time)
    {
        cur->play_start = start_time - record_start;
        cur->play_time = record_end - start_time;
    }
    else if(record_start >= start_time && record_end <= end_time)
    {
        cur->play_start = 0;
        cur->play_time = cur->record_time;
    }
    else if(record_start < end_time && record_end > end_time)
    {
        cur->play_start = 0;
        cur->play_time = end_time - record_start;
    }
	
    return 1;
}

/**
 * NAME         video_fs_list_get_record
 * @BRIEF	 统计符合回放的记录信息
  
 * @PARAM    start_time        回放的起始时间
                    end_time         回放的结束时间
                    type                回放的类型
 * @RETURN	返回符合回放的记录指针
 * @RETVAL	
 */

Precord_replay_info video_fs_list_get_record(unsigned long  start_time, unsigned long end_time, int clip_limit , int type)
{
    Precord_replay_info head = NULL, cur, tail = NULL;
    Precord_replay_info all_list;
	int count = 0;

	/** 获取所有录像文件信息 **/
	all_list = video_fs_list_load_list(type);	//allocate mem no release
	

    while(all_list)
    {
        cur = all_list;
        all_list = all_list->next;
        if(video_fs_list_check_record(start_time, end_time, cur))
        {
        	if (clip_limit > 0) //需要限制片段个数
        	{
	        	if (count >= clip_limit)
	        	{
		            free(cur);
    	    		continue;
    	    	}
    	    }
        		
            cur->next = NULL;
            if(head == NULL)
            {
                head = cur; 
            }
            else
            {
                tail->next = cur;
            }
            tail = cur;
			count++;
        }
        else
        {
            free(cur);
        }
    }

    return head;
}


#if 0
/**
* @brief  video_fs_cmp_alarm_record_file
* 			比较移动侦测录像文件
* @date 	2015/3
* @param:	char * file_name， 待比较的文件名
* @return 	uint8 
* @retval 	是当前系统录制的文件-->1, 否则-->0
*/

uint8 video_fs_cmp_alarm_record_file(char *file_name)
{
	int file_len, cmp_len, record_file_len;

	cmp_len = strlen(RECORD_SAVE_SUF_NAME);
    file_len = strlen(file_name);
    record_file_len = 6 + strlen(OUR_VIDEO_ALARM_FILE_PREFIX) + cmp_len;
    if(file_len < record_file_len)
    {
        anyka_print("[%s:%d] %s isn't anyka video record!, (%d,%d)\n",
			   		 __func__, __LINE__, file_name, file_len, record_file_len);
        return 0;
    }

	/**** first compare prefix ****/
    if(memcmp(file_name, OUR_VIDEO_ALARM_FILE_PREFIX, strlen(OUR_VIDEO_ALARM_FILE_PREFIX)))
    {
        anyka_print("[%s:%d] %s isn't anyka video record!, %s\n",
			   		__func__, __LINE__, file_name, OUR_VIDEO_ALARM_FILE_PREFIX);
        return 0;
    }

	/**** then compare suffix ****/
    if(memcmp(file_name + file_len - cmp_len, RECORD_SAVE_SUF_NAME, cmp_len))
    {
        anyka_print("[%s:%d] %s isn't anyka video record!, %s\n", 
					__func__, __LINE__, file_name, RECORD_SAVE_SUF_NAME);
        return 0;
    }
    return 1;
}



/**
* @brief  video_fs_cmp_video_record_file
* 			比较计划录像文件
* @date 	2015/3
* @param:	char * file_name， 待比较的文件名
* @return 	uint8 
* @retval 	是当前系统录制的文件-->1, 否则-->0
*/

uint8 video_fs_cmp_video_record_file(char *file_name)
{
	int file_len, cmp_len, record_file_len;

	cmp_len = strlen(RECORD_SAVE_SUF_NAME);
    file_len = strlen(file_name);
    record_file_len = 6 + strlen(VIDEO_FILE_PREFIX) + cmp_len;
    if(file_len < record_file_len)
    {
        anyka_print("[%s:%d] %s isn't anyka video record!, (%d,%d)\n",
			   		 __func__, __LINE__, file_name, file_len, record_file_len);
        return 0;
    }
	
	/**** first compare prefix ****/
    if(memcmp(file_name, VIDEO_FILE_PREFIX, strlen(VIDEO_FILE_PREFIX)))
    {
        anyka_print("[%s:%d] %s isn't anyka video record!, %s\n",
			   		__func__, __LINE__, file_name, VIDEO_FILE_PREFIX);
        return 0;
    }

	/**** then compare suffix ****/
    if(memcmp(file_name + file_len - cmp_len, RECORD_SAVE_SUF_NAME, cmp_len))
    {
        anyka_print("[%s:%d] %s isn't anyka video record!, %s\n", 
					__func__, __LINE__, file_name, RECORD_SAVE_SUF_NAME);
        return 0;
    }
    return 1;
	
}

#endif

#if 0
/**
* @brief  video_fs_check_record_dir_name
* 			根据目录名和类型比较当前目录是否为存放录像目录
* @date 	2015/3
* @param:	char * dir_name， 待比较的目录
			unsigned short type，RECORD_REPLAY_TYPE 计划录像累型，否则为移动侦测类型
* @return 	uint8 
* @retval 	是当前系统录制的文件-->1, 否则-->0
*/

uint8 video_fs_check_record_dir_name(char * dir_name, unsigned short type)
{
	int ret = 0;
	if(type == RECORD_REPLAY_TYPE)	//video
	{
		if(0 == memcmp(dir_name, VIDEO_FILE_PREFIX, strlen(VIDEO_FILE_PREFIX)))
			ret = 1;
	}
	
	else if(type == ALARM_REPLAY_TYPE)	//alarm
	{
		if(0 == memcmp(dir_name, ALARM_DIR_PREFIX, strlen(ALARM_DIR_PREFIX)))
			ret = 1;
	}

	return ret;
}
#endif 

