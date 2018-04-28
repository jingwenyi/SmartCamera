#ifndef _video_net_h_
#define _video_net_h_

/**
 * NAME         anyka_stop_video
 * @BRIEF	关闭网络视频
 * @PARAM	mydata           用户数据，此结构的指针要与anyka_start_video传的一致
 * @RETURN	void
 * @RETVAL	
 */


void anyka_stop_video(void *mydata);


/**
 * NAME         anyka_start_video
 * @BRIEF	打开网络视频，或者修改网络视频参数
 * @PARAM	video_quality   视频的质量参数
                    mydata           用户数据
                    psend_video_callback    用户回调函数，用于发送网络数据,如果为空，将是修改参数，
                                                     不为空将是新开启功能
 * @RETURN	返回视频的帧率
 * @RETVAL	
 */

int anyka_start_video(int video_quality, void *mydata, P_VIDEO_DOWITH_CALLBACK psend_video_callback);



/**
 * NAME         anyka_start_picture
 * @BRIEF	传送一个图片到网络
 * @PARAM	mydata           用户数据
                    psend_video_callback    网络发送图片的回调
 * @RETURN	void
 * @RETVAL	
 */
 

void anyka_start_picture(void *mydata, P_VIDEO_DOWITH_CALLBACK psend_video_callback);


/**
 * NAME         anyka_set_video_iframe
 * @BRIEF	强行将编码设置为帧
 * @PARAM	mydata           用户数据
                   
 * @RETURN	NONE
 * @RETVAL	
 */
void anyka_set_video_iframe(void *mydata);

/**
 * NAME         anyka_start_video_withbps
 * @BRIEF	打开网络视频，或者修改网络视频参数
 * @PARAM
            video_type 720P或者VGA
            bps   视频的码率
            mydata           用户数据
            psend_video_callback    用户回调函数，用于发送网络数据,如果为空，将是修改参数，
                                                     不为空将是新开启功能
 * @RETURN	返回视频的帧率
 * @RETVAL	
 */
int anyka_start_video_withbps(int video_type, int bps, void *mydata, P_VIDEO_DOWITH_CALLBACK psend_video_callback);


/**
 * NAME         anyka_set_video_para
 * @BRIEF	修改视频方面的参数，
 * @PARAM	video_type   编码类型，目前只支持(1:720) OR (0:VGA)
            bps          视频的码率
            mydata       用户数据
 * @RETURN	NONE
 * @RETVAL	
 */
 
void anyka_set_video_para(int video_type, int bps, void *mydata);

#endif


