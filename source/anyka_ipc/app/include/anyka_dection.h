#ifndef _anyka_dection_h_
#define _anyka_dection_h_
//目前系统只做了移动与声音侦测功能
enum
{
    SYS_CHECK_VOICE_ALARM = 1,
    SYS_CHECK_VIDEO_ALARM = 2,
    SYS_CHECK_OTHER_ALARM = 4,
};
typedef void PANYKA_SEND_ALARM_FUNC(int alarm_type, int save_flag, char *save_path, int start_time, int time_len);

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


void anyka_dection_init(PANYKA_SEND_ALARM_FUNC Palarm_func, PANYKA_FILTER_VIDEO_CHECK_FUNC filter_check);

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

void anyka_dection_start(int move_level, int check_type, PANYKA_SEND_ALARM_FUNC Palarm_func, PANYKA_FILTER_VIDEO_CHECK_FUNC filter_check);

/**
 * NAME         anyka_dection_stop
 * @BRIEF	关闭相应的侦测类型,如果所有侦测全被关闭，我们将释放所有资源
 * @PARAM	check_type
 * @RETURN	void
 * @RETVAL	
 */

void anyka_dection_stop(int check_type);

/**
 * NAME         anyka_dection_save_record
 * @BRIEF	侦测录像是否开始
 * @PARAM	
 * @RETURN	1-->正在侦测录像; 0-->侦测录像未开始
 * @RETVAL	
 */

int anyka_dection_save_record(void);


/**
 * NAME         anyka_dection_pause_dection
 * @BRIEF	停止移动与声音侦测
 * @PARAM	
 * @RETURN	
 * @RETVAL	
 */

void anyka_dection_pause_dection(void);

/**
 * NAME         anyka_dection_resume_dection
 * @BRIEF	恢复移动与声音侦测
 * @PARAM	
 * @RETURN	
 * @RETVAL	
 */

void anyka_dection_resume_dection(void);




#endif

