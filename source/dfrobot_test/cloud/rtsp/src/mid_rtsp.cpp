#include "rtsp_main.hh"

/**
* * @brief 	start_rtsp_service
* * 			call RTSP_INIT to start rtsp service
* * @date 	2015/4
* * @param:	void
* * @return	int 
* * @retval 0 -> success, -1 -> failed	
* */

#ifdef __cplusplus
extern "C" {
#endif

void start_rtsp_service(void)
{
	RTSP_init();	
}

#ifdef __cplusplus
}
#endif

