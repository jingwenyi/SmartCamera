#ifndef _video_ctrl_h_
#define _video_ctrl_h_
#include "anyka_types.h"
#include "basetype.h"

#include "motion_detector_lib.h"

#define VIDEO_WIDTH_720P    1280
#define VIDEO_HEIGHT_720P	camera_get_ch1_height()
#define VIDEO_WIDTH_VGA     640
#define VIDEO_HEIGHT_VGA	camera_get_ch2_height()

enum FRAMES_ENCDOE_TYPE
{
    FRAMES_ENCODE_RECORD,  //30  FRAMES
    FRAMES_ENCODE_720P_NET, //10 FRAMES
    FRAMES_ENCODE_VGA_NET, // 15 FRAMES
    FRAMES_ENCODE_PICTURE, // PICTURE
    FRAMES_ENCODE_HANDLE_NUM // 
};

typedef struct {
	T_U32 timestamp;
	T_U32 iFrame;
	T_pDATA  buf;
	T_U32 size;
	T_U16 num;
	time_t calendar_time;
}T_STM_STRUCT;

typedef int PANYKA_FILTER_VIDEO_CHECK_FUNC();

typedef void P_VIDEO_DOWITH_CALLBACK(T_VOID *param, T_STM_STRUCT *pstream);
/**
 * NAME         video_add
 * @BRIEF	 启动视频编码线程，将编码后的数据通过回调传给应用,
 * @PARAM	pcallback           视频回调函数
                    mydata              用户数据
                    video_type
                    video_bps
 * @RETURN	void
 * @RETVAL	
 */

void video_init(void);
/**
 * NAME         video_add
 * @BRIEF	 启动视频编码线程，将编码后的数据通过回调传给应用,
 * @PARAM	pcallback           视频回调函数
                    mydata              用户数据
                    video_type
                    video_bps
 * @RETURN	void
 * @RETVAL	
 */

void video_add(P_VIDEO_DOWITH_CALLBACK pcallback, T_VOID *mydata, int video_type, int video_bps);

/**
 * NAME         video_add
 * @BRIEF	修改视频的相关参数
 * @PARAM	mydata              用户数据
                    video_type
                    video_bps
 * @RETURN	void
 * @RETVAL	
 */
void video_change_attr(T_VOID *mydata, int video_type, int video_bps);

/**
 * NAME         video_del
 * @BRIEF	关闭对应的视频句柄，这里可能会触发CAMERA的关闭动作
 * @PARAM	mydata              用户数据
 * @RETURN	void
 * @RETVAL	
 */

void video_del(T_VOID *mydata);

typedef void video_move_check_notice_callback(void *user, uint8 notice);

/**
 * NAME         video_start_move_check
 * @BRIEF	启动移动侦测主功能
 * @PARAM	height
                    width
                    detction
                    ratios
                    Palarm_func  如果发生侦测时的回调函数
                    user        用户数据
                    filter_check   当前是否过滤侦测的，目前设计腾讯的在视频观看的时候不做侦测
 * @RETURN	0-->fail; 1-->ok
 * @RETVAL	
 */



int video_start_move_check(int height, int width, T_MOTION_DETECTOR_DIMENSION *detction, uint16 *ratios, int time, video_move_check_notice_callback pcallback, void *user, PANYKA_FILTER_VIDEO_CHECK_FUNC filter_check);

/**
 * NAME         video_start_move_check
 * @BRIEF	停止移动侦测
 * @PARAM	
 * @RETURN	
 * @RETVAL	
 */

void video_stop_move_check(void);


/**
 * NAME         video_set_move_check_attr
 * @BRIEF	设置移动侦测参数
 * @PARAM	ratios
 * @RETURN	
 * @RETVAL	
 */
void video_set_move_check_attr(uint16 *ratios);

/**
 * NAME         video_set_encode_fps
 * @BRIEF	 设置编码的帧率，目前只有ONVIF有这个用法
 
 
 * @PARAM    video_type
                    fps
 * @RETURN	void
 * @RETVAL	
 */

void video_set_encode_fps(int video_type, int fps);
/**
 * NAME         video_set_encode_bps
 * @BRIEF	 设置编码的码率，目前只有ONVIF有这个用法
 * @PARAM	video_type
                    bitrate
 * @RETURN	void
 * @RETVAL	
 */


void video_set_encode_bps(int video_type, int bitrate);

/**
 * NAME         video_set_iframe
 * @BRIEF	强制将视频设置为I帧
 * @PARAM	mydata              用户数据
 * @RETURN	void
 * @RETVAL	
 */

void video_set_iframe(T_VOID *mydata);

#endif

