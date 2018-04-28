#ifndef _record_replay_h_
#define _record_replay_h_
#include "video_fs_list_manage.h"


typedef void P_RECORD_REPLAY_CALLBACK(T_VOID *param, int type, T_STM_STRUCT *pstream);

/**
 * NAME         record_replay_start
 * @BRIEF	 启动录像回放功能
 
 
 * @PARAM    start_time              回放的开始时间
                    end_time               回放的结束时间
                    clip_limit   限制的视频片段数量，0表示不限制, >0 是有效值
                    type                        要求回放的类型，支持警告与录像回放
                    mydata                     用户数据结构
                    psend_video_callback 送音视频数据的回调函数
 * @RETURN	int
 * @RETVAL	1-->fail; 0-->success
 */

int record_replay_start(unsigned long  start_time, unsigned long end_time, int clip_limit,int type, void *mydata, P_RECORD_REPLAY_CALLBACK psend_video_callback);

/**
 * NAME         record_replay_play_status
 * @BRIEF	 录像回放的播放与暂停
 
 
 * @PARAM    mydata        用户数据结构，是在启动回放传进来的
                    play            1-->play; 0-->pause     
 * @RETURN	void
 * @RETVAL	
 */

void record_replay_play_status(void *mydata, int play);

/**
 * NAME         record_replay_stop
 * @BRIEF	 录像回放的取消
 
 
 * @PARAM    mydata        用户数据结构，是在启动回放传进来的
 * @RETURN	void
 * @RETVAL	
 */
void record_replay_stop(void *mydata);
/**
 * NAME         record_replay_init
 * @BRIEF	初始化录像回放相关信息，这里只创建线程处理，然后直接返回
 
 
 * @PARAM    void
 * @RETURN	void
 * @RETVAL	
 */

void record_replay_init(void);


/**
 * NAME         record_replay_get_read
 * @BRIEF	得到符合记录的下一条记录信息
  
 * @PARAM    record_list       open时候 的句柄
                    find_end       符合记录的最后时间
                    result              记录信息储存
 * @RETURN	查找结果
 * @RETVAL	0-->FAIL, 1-->success
 */

int record_replay_get_read(void *record_list, unsigned long find_end, Precord_replay_info result);


/**
 * NAME         record_replay_get_close_file
 * @BRIEF	关闭打开文件
  
 * @PARAM    
 * @RETURN	NONE
 * @RETVAL	
 */


void record_replay_get_close_file(void *record_list);

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


void * record_replay_get_open_file(int type, unsigned long start_time, unsigned long find_end, Precord_replay_info result);






#endif


