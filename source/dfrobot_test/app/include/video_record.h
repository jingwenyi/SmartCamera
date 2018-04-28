#ifndef _video_record_h_
#define _video_record_h_

/**
 * NAME         video_record_main_loop
 * @BRIEF	计划录像的启动与关闭，通过读配置文件，以时间为依据启动或停止
 * @PARAM	void
 * @RETURN	void
 * @RETVAL	
 */

void video_record_main_loop(void);

/**
 * NAME         video_record_check_run
 * @BRIEF	检查是否正在录像
 * @PARAM	none 
 * @RETURN	0返回不在录像，1返回正在录像
 * @RETVAL	
 */

int video_record_check_run(void);

/**
 * NAME         video_record_start
 * @BRIEF	录像启动
 * @PARAM	void
 * @RETURN	-1返回失败，1返回成功
 * @RETVAL	
 */

int video_record_start(void);

/**
 * NAME         video_record_stop
 * @BRIEF	录像停止，将释放所有的录像相关的资源
 * @PARAM	void
 * @RETURN	void
 * @RETVAL	
 */

void video_record_stop(void);
/**
 * NAME         video_record_err_stop
 * @BRIEF	如果录像过程中失败，将停止录像，设置标志，等待录像结束
 * @PARAM	void
 * @RETURN	void
 * @RETVAL	
 */
void video_record_err_stop(void);

/**
 * NAME         video_record_update_use_size
 * @BRIEF	当报警录像的使用了空间后，要更新这个录像大小
 * @PARAM	int file_size 报警文件大小
 * @RETURN	void*
 * @RETVAL	
 */


void video_record_update_use_size(int file_size);

#endif


