#ifndef _anyka_dana_h_
#define _anyka_dana_h_
#include "stdio.h"
#include "anyka_types.h"
#include "video_ctrl.h"
#include "danavideo.h"


/**
* @brief 	本文件为anyka 和大拿对接中的一些头文件
* @date 	2015/3
* @param:	
* @return 	
* @retval 	
*/

typedef struct _MyData {
    pdana_video_conn_t *danavideoconn;
    int device_type;
    uint8_t chan_no;
    char appname[32]; 
} MYDATA;

#endif

