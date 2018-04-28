#include "common.h"
#include "dvs.h"
#include "anyka_config.h"
#include "ak_onvif.h"

// extern int onvif_start_flage;
int frame_index_m = 0, frame_index_s = 0;

/**
 * NAME        onvif_get_720P_video
 * @BRIEF	get onvif 720P video
                  
 * @PARAM	T_VOID *param, 
 			T_STM_STRUCT *pstream, video data
 			
 * @RETURN	void
 * @RETVAL	
 */

void onvif_get_720P_video(T_VOID *param, T_STM_STRUCT *pstream)
{
    unsigned int tv_sec, tv_usec, pts;
    int iframe_m, res;
	buffers_t *send_bufs_m = &(g_dvs.chunnel[0].video_send_bufs);
    
	if (pstream->iFrame == 1)
	{
		iframe_m = 0x02;
		frame_index_m = 0;
	}
	else
	{
	   	iframe_m = 0;
		frame_index_m++;
	}
    tv_sec = pstream->timestamp / 1000;
    tv_usec = (pstream->timestamp % 1000) * 1000;
    pts = tv_sec * 90000 + tv_usec *9 / 100;
	res = buffers_put_data_with_pts (pstream->buf, pstream->size,
      		send_bufs_m, iframe_m, 0, frame_index_m, pstream->timestamp);
    
	if (res < 1)		
	{

        if (frame_index_m == 0)	//IFrame
    	{
    		anyka_print("onvif 720P buffers full, clear_data !\r\n");
    		buffers_clear_data (send_bufs_m);
    		buffers_put_data_with_pts (pstream->buf, pstream->size,
        		send_bufs_m, iframe_m, 0, frame_index_m, pstream->timestamp);
    	}
	}
}


/**
 * NAME        onvif_get_VGA_video
 * @BRIEF	get onvif VGA video
                  
 * @PARAM	T_VOID *param, 
 			T_STM_STRUCT *pstream, video data
 			
 * @RETURN	void
 * @RETVAL	
 */

void onvif_get_VGA_video(T_VOID *param, T_STM_STRUCT *pstream)
{
    int iframe_s, res;
    unsigned int tv_sec, tv_usec, pts;
	buffers_t *send_bufs_s = &(g_dvs.chunnel[0].video_send_bufs_small);

    
    tv_sec = pstream->timestamp / 1000;
    tv_usec = (pstream->timestamp % 1000) * 1000;
    pts = tv_sec * 90000 + tv_usec *9 / 100;
    if (pstream->iFrame == 1)
    {
    	iframe_s = 0x02;
    	frame_index_s = 0;
    }
    else
    {
       	iframe_s = 0;
    	frame_index_s++;
    }
    res = buffers_put_data_with_pts (pstream->buf, pstream->size,
      		send_bufs_s, iframe_s, 0, frame_index_s, pts);
    if (res < 1)		//buffer full
    {

    	if (frame_index_s == 0)	//IFrame
    	{
    		anyka_print("onvif VGA buffers full, clear_data ! \r\n");
    		buffers_clear_data (send_bufs_s);
    		buffers_put_data_with_pts (pstream->buf, pstream->size,
        		send_bufs_s, iframe_s, 0, frame_index_s, pts);
    	}

    }
}


/**
 * NAME        onvif_init
 * @BRIEF	onvif 平台初始化主接口
                  
 * @PARAM	void
 			
 * @RETURN	void
 * @RETVAL	
 */

void onvif_init(void)
{    
    Psystem_onvif_set_info onvif = anyka_get_sys_onvif_setting();
    onvif_start_flage = 1;
    init_onvif(1280, VIDEO_HEIGHT_720P);
    start_onvif();
    video_add(onvif_get_720P_video,&onvif_start_flage, FRAMES_ENCODE_RECORD, onvif->kbps1);
    video_set_encode_fps(FRAMES_ENCODE_RECORD, onvif->fps1);
    video_set_encode_bps(FRAMES_ENCODE_RECORD, onvif->kbps1);
    video_add(onvif_get_VGA_video,&onvif_start_flage, FRAMES_ENCODE_VGA_NET, onvif->kbps2);
	video_set_encode_fps(FRAMES_ENCODE_VGA_NET, onvif->fps2);
    video_set_encode_bps(FRAMES_ENCODE_VGA_NET, onvif->kbps2);
    onvif_start_flage = 0;
    anyka_print("*********onvif has runed************\n");
    
}


