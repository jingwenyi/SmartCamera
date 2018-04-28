#ifndef _AK_RTSP_H_
#define _AK_RTSP_H_

/**
* * @brief  anyka_rtsp_init
* *         create a thread to check net, only if the device get a ip address we would start the rtsp service
* * @date   2015/4
* * @param: void
* * @return int 
* * @retval 0 -> success, -1 -> failed  
* */

int anyka_rtsp_init(void);

#endif
