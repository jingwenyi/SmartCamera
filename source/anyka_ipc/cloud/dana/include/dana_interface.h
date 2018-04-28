#ifndef _dana_interface_h_
#define _dana_interface_h_


/**
* @brief 	dana_init
			大拿云平台初始化入口函数
* @date 	2015/3
* @param:	char *danale_path， 大拿序列号文件路径
* @return 	void
* @retval 	
*/

void dana_init(char *danale_path);

/**
* @brief 	anyka_send_net_audio
* 			发送网络音频回调
* @date 	2015/3
* @param:	T_VOID *param，数据
			T_STM_STRUCT *pstream，视频数据参数
* @return 	void
* @retval 	
*/

void anyka_send_net_audio(T_VOID *param, T_STM_STRUCT *pstream);

/**
* @brief 	anyka_set_pzt_control
* 			云台控制接口
* @date 	2015/3
* @param:	int code ，控制类型
			int para1，参数
* @return 	void
* @retval 	
*/

void anyka_send_video_move_alarm(int alarm_type, int save_flag, char *save_path, int start_time, int time_len);

/**
* @brief 	anyka_get_talk_data
* 			获取语音数据回调
* @date 	2015/3
* @param:	T_VOID *param，数据
* @return 	void *
* @retval 	
*/

void *anyka_get_talk_data(void *param );

/**
* @brief 	anyka_send_dana_pictrue
* 			发送网络照片回调
* @date 	2015/3
* @param:	T_VOID *param，数据
			T_STM_STRUCT *pstream，视频数据参数
* @return 	void 
* @retval 
*/

void anyka_send_dana_pictrue(void *parm, T_STM_STRUCT *pstream);

/**
* @brief 	anyka_send_net_video
* 			发送网络视频回调
* @date 	2015/3
* @param:	T_VOID *param，数据
			T_STM_STRUCT *pstream，视频数据参数
* @return 	void
* @retval 	
*/

void anyka_send_net_video(T_VOID *param, T_STM_STRUCT *pstream);


/**
* @brief 	anyka_set_pzt_control
* 			云台控制接口
* @date 	2015/3
* @param:	int code ，控制类型
			int para1，参数
* @return 	int
* @retval 	成功0
*/

int anyka_set_pzt_control( int code , int para1);

/**
* @brief 	anyka_dana_record_play
* 			启动录像回放
* @date 	2015/3
* @param:	unsigned long start_time， 起始时间
			void *mydata， 数据
* @return 	int
* @retval 	
*/

int anyka_dana_record_play(unsigned long start_time, void *mydata);

/**
* @brief 	anyka_dana_record_play
* 			暂停录像回放
* @date 	2015/3
* @param:	void *mydata， 数据
			int play，播放标志， 0  暂停
* @return 	int
* @retval 	
*/
void anyka_dana_record_play_pause(void *mydata, int play);

/**
* @brief 	anyka_dana_record_play
* 			停止录像回放
* @date 	2015/3
* @param:	
			void *mydata， 数据
* @return 	int
* @retval 	
*/
void anyka_dana_record_stop(void *mydata);
#endif

