/*******************************************************************
此文件完成网络视频的启动与停止，和图片的发送
*******************************************************************/

#include "common.h"
#include "dana_interface.h"

#define HIGH_BPS 100
#define NORMAL_BPS 60
#define BPS_DIVIDE 25	//码率分割值



/**
 * NAME         anyka_stop_video
 * @BRIEF	打开网络视频，或者修改网络视频参数
 * @PARAM	mydata           用户数据，此结构的指针要与anyka_start_video传的一致
 * @RETURN	void
 * @RETVAL	
 */

void anyka_stop_video(void *mydata)
{
    video_del(mydata);
}



/**
 * NAME         anyka_start_picture
 * @BRIEF	传送一个图片到网络
 * @PARAM	mydata           用户数据
                    psend_video_callback    网络发送图片的回调
 * @RETURN	void
 * @RETVAL	
 */
 

void anyka_start_picture(void *mydata, P_VIDEO_DOWITH_CALLBACK psend_video_callback)
{
    video_add(psend_video_callback,mydata, FRAMES_ENCODE_PICTURE, 500);
}


/**
 * NAME         anyka_set_video_iframe
 * @BRIEF	强行将编码设置为帧
 * @PARAM	mydata           用户数据
                   
 * @RETURN	NONE
 * @RETVAL	
 */
void anyka_set_video_iframe(void *mydata)

{
    video_set_iframe(mydata);
}

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
int anyka_start_video(int video_quality, void *mydata, P_VIDEO_DOWITH_CALLBACK psend_video_callback)
{

    int video_bps, video_type, frames;    
    Psystem_cloud_set_info sys_setting;    
    video_setting *pvideo_default_config = anyka_get_sys_video_setting();
    
    Psystem_onvif_set_info onvif_setting = anyka_get_sys_onvif_setting();
    
    sys_setting = anyka_get_sys_cloud_setting();
    if(video_quality >= HIGH_BPS)
    {
        video_quality = HIGH_BPS;
    }
    
    if(video_quality >= NORMAL_BPS)
    {
        frames = onvif_setting->fps1;
        video_type = FRAMES_ENCODE_RECORD;
        video_bps = onvif_setting->kbps1;
    }
    else if(video_quality >= BPS_DIVIDE)
    {
        
        #ifdef  CONFIG_ONVIF_SUPPORT
        if(sys_setting->onvif)
        {
            frames = pvideo_default_config->VGAPfps;
            video_type = FRAMES_ENCODE_VGA_NET;
            video_bps = pvideo_default_config->VGAmaxkbps;
        }
        else
        #endif
        {
            frames = pvideo_default_config->V720Pfps;
            video_type = FRAMES_ENCODE_720P_NET;
            video_bps = pvideo_default_config->V720Pminkbps + (pvideo_default_config->V720Pmaxkbps - pvideo_default_config->V720Pminkbps) * (video_quality - BPS_DIVIDE) / (NORMAL_BPS - BPS_DIVIDE);
        }
    }
    else 
    {
        frames = pvideo_default_config->VGAPfps;
        video_type = FRAMES_ENCODE_VGA_NET;
        video_bps = pvideo_default_config->VGAminkbps + (pvideo_default_config->VGAmaxkbps - pvideo_default_config->VGAminkbps) * video_quality  / BPS_DIVIDE;
    }
    
    if(psend_video_callback)
    {
        video_add(psend_video_callback,mydata, video_type, video_bps);
    }
    else
    {
        video_change_attr(mydata, video_type, video_bps);
    }
    return frames;
}


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
int anyka_start_video_withbps(int video_type, int video_bps, void *mydata, P_VIDEO_DOWITH_CALLBACK psend_video_callback)
{
    int frames;    
    video_setting *pvideo_default_config = anyka_get_sys_video_setting();
        
    if(video_type == FRAMES_ENCODE_720P_NET)
    {        
        frames = pvideo_default_config->V720Pfps;
    }
    else 
    {
        frames = pvideo_default_config->VGAPfps;
    }
    
    if(psend_video_callback)
    {
        video_add(psend_video_callback,mydata, video_type, video_bps);
    }
    else
    {
        video_change_attr(mydata, video_type, video_bps);
    }
    return frames;
}

/**
 * NAME         anyka_set_video_para
 * @BRIEF	修改视频方面的参数，
 * @PARAM	video_type   编码类型，目前只支持(1:720) OR (0:VGA)
            bps          视频的码率
            mydata       用户数据
 * @RETURN	NONE
 * @RETVAL	
 */
void anyka_set_video_para(int video_type, int bps, void *mydata)
{
    video_change_attr(mydata, video_type, bps);
}


