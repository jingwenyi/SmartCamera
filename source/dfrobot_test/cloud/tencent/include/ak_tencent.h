#ifndef _ak_tencent_h_
#define _ak_tencent_h_


/**
 * NAME     qq_start_video
 * @BRIEF	开始启动视频传输
 * @RETURN	true
 * @RETVAL	
 */
bool qq_start_video();

/**
 * NAME     qq_stop_video
 * @BRIEF	关闭视频传输
 * @RETURN	true
 * @RETVAL	
 */
bool qq_stop_video(void);

/**
 * NAME     qq_reset_video
 * @BRIEF	已经发生丢帧了，下一帧将传输I帧
 * @PARAM	
 * @RETURN	true
 * @RETVAL	
 */
bool qq_reset_video(void);

/**
 * NAME     qq_set_video
 * @BRIEF	因网络质量发生变化，将修改码率
 * @PARAM	bit_rate 码率
 * @RETURN	true
 * @RETVAL	
 */
bool qq_set_video(int bit_rate);

/**
 * NAME     qq_start_mic
 * @BRIEF	启动向腾讯发送音频数据
 * @RETURN	true
 * @RETVAL	
 */
bool qq_start_mic(void);

/**
 * NAME     qq_stop_mic
 * @BRIEF	结束向腾讯发送音频数据
 * @RETURN	true
 * @RETVAL	
 */
bool qq_stop_mic(void);

/**
 * NAME     qq_start_video_replay 
 * @BRIEF	start qq video replay
 * @PARAM	play_time : begin play time
 * @RETURN	NONE
 * @RETVAL	
 */
void qq_start_video_replay(unsigned int play_time);

/**
 * NAME     ak_qq_get_audio_data
 * @BRIEF	腾讯向我们设备发送音频数据
 * @PARAM	pcEncData  数据缓冲区
            nEncDataLen 数据长度
 * @RETURN	NONE
 * @RETVAL	
 */
void ak_qq_get_audio_data(tx_audio_encode_param *pAudioEncodeParam, unsigned char *pcEncData, int nEncDataLen);

/**
 * NAME     ak_qq_init
 * @BRIEF	启动与腾讯对接的相关资源初始化
 * @PARAM	
 * @RETURN	NONE
 * @RETVAL	
 */
void ak_qq_init(void);

/**
 * NAME     ak_qq_control_rotate
 * @BRIEF	马达控制
 * @PARAM	rotate_direction: 马达转动的方向
                    rotate_degree   : 马达转动的角度，目前我们不支持。转动默认角度
 * @RETURN	NONE
 * @RETVAL	
 */
int ak_qq_control_rotate(int rotate_direction, int rotate_degree);

/**
 * NAME     ak_qq_send_msg
 * @BRIEF	向腾讯QQ发送设备消息
 * @PARAM	msg_id: 消息类型
            file_name   : 对应消息的文件住处
            title :     消息的标题
            content: 消息的内容
            alarm_time: 消息发出的时间。
 * @RETURN	NONE
 * @RETVAL	
 */
void ak_qq_send_msg(int msg_id, char * file_name, char *title, char *content, int alarm_time );

/**
 * NAME     ak_qq_set_video_para
 * @BRIEF	开始启动视频传输
 * @PARAM	type    视频的类型
 * @RETURN	NONE
 * @RETVAL	
 */
bool ak_qq_set_video_para(int type);

/**
 * NAME     ak_qq_check_video_filter
 * @BRIEF	检测是否在网络视频 
 * @PARAM	NONE
 * @RETURN	0:没有在预览状态；1 :在预览状态
 * @RETVAL	
 */
int ak_qq_check_video_filter(void);

/**
* NAME     ak_qq_OnUpdateConfirm
* @BRIEF   on update confirm, when user tick the confirm button then we start update   
* @PARAM   NONE
* @RETURN  NONE
* @RETVAL  
*/
void ak_qq_OnUpdateConfirm();

/**
* NAME     ak_qq_get_global_setting
* @BRIEF   use to get the device name and check whether this machine is first start after update
* @PARAM   dev_name: store the device name buf
*          first_start: store the value; 
*          1=> the device is first start; 0=> the device is not first start after update
* @RETURN  int
* @RETVAL  on success, return 0; else return -1;
*/
int ak_qq_get_global_setting(char *dev_name, unsigned short *first_start);

/**
* NAME     ak_qq_change_firmware_update_status 
* @BRIEF   change the firmware update status
* @PARAM   flags: flags 0 or 1
* @RETURN  NONE
* @RETVAL  
*/
void ak_qq_change_firmware_update_status(int flags);

/**
* NAME     ak_qq_start_video_replay 
* @BRIEF   start video replay
* @PARAM   play_time : begin time 
* @RETURN  NONE
* @RETVAL  
*/
void ak_qq_start_video_replay(unsigned int play_time);

/**
* NAME     ak_qq_stop_video_replay 
* @BRIEF   stop video replay
* @PARAM   NONE
* @RETURN  NONE
* @RETVAL  
*/
void ak_qq_stop_video_replay(void);


/**
 * NAME ak_qq_start_voicelink
 * @BRIEF start voicelink to get wifi config info
 * @RETURN NONE
 */
void ak_qq_start_voicelink();

/**
 * NAME ak_qq_stop_voicelink
 * @BRIEF stop voicelink function
 * @RETURN NONE
 */
void ak_qq_stop_voicelink();

#endif


