#ifndef _video_fs_list_manage_h_
#define _video_fs_list_manage_h_


typedef struct _record_replay_info *Precord_replay_info;

typedef struct _record_replay_info
{
    unsigned long  recrod_start_time;      //录像文件的起始时间
    unsigned long  record_time;      //录像文件的总时长
    unsigned long  play_start;      //录像文件的开始播放位置
    unsigned long  play_time;      //这个录像需要播放的时间
    char    file_name[100];     //录像文件名
    Precord_replay_info next;
}record_replay_info;

/**
 * NAME         video_fs_list_insert_file
 * @BRIEF	添加一种类型的录像文件，一般在计划录像或报警录像调用
  
 * @PARAM    type                回放的类型
                    file_name        录像文件名
 * @RETURN	NONE
 * @RETVAL	
 */
void video_fs_list_insert_file(char *file_name, int type);

/**
 * NAME         video_fs_list_remove_file
 * @BRIEF	删除一种类型的录像文件，一般在计划录像或报警录像调用
  
 * @PARAM    type                回放的类型
                    file_name        录像文件名
 * @RETURN	NONE
 * @RETVAL	
 */
void video_fs_list_remove_file(char *file_name, int type);

/**
 * NAME         video_fs_list_get_record
 * @BRIEF	 统计符合回放的记录信息
  
 * @PARAM    start_time        回放的起始时间
                    end_time         回放的结束时间
                    type                回放的类型
 * @RETURN	返回符合回放的记录指针
 * @RETVAL	
 */

Precord_replay_info video_fs_list_get_record(unsigned long  start_time, unsigned long end_time, int clip_limit , int type);

/**
 * NAME         video_fs_list_init
 * @BRIEF	应用程序启动时，创建录像文件列表
  
 * @PARAM    user                
 * @RETURN	NONE
 * @RETVAL	
 */

//void record_replay_test();
void video_fs_list_init(  );


#endif
