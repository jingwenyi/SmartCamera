/** @file
* @brief drow Font to Display
*
* Copyright (C) 2006 Anyka (GuangZhou) Software Technology Co., Ltd.
* @author
* @date 2006-01-16
* @version 1.0
*/

#ifndef __AKFONT_DISPLAY_H__
#define __AKFONT_DISPLAY_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "font_lib.h"


typedef struct _osd_camera_info
{
    int start_x;
    int dot_x;
    void *font_buf;
}osd_camera_info, *Posd_camera_info;


typedef T_VOID (*FONT_DISP_SET_PIXEL_CB)(T_pVOID param, T_U16 pos_x, T_U16 pos_y);
typedef T_S8 (*FONT_DISP_SET_PIXEL_TO_HW_OSD_CB)(T_pVOID param, T_U16 pos_x, T_U16 pos_y, T_S8 pixel_value);

T_U32 FontDisp_DrawStr(FONT_DISP_SET_PIXEL_CB setPixelCbf, T_pVOID param,
        T_POS x, T_POS y, T_pWSTR disp_string, T_U16 strLen, int font_dot);

T_U32 FontDisp_SetFontMatrix(FONT_DISP_SET_PIXEL_TO_HW_OSD_CB setPixelCbf,
		T_pVOID param, T_U16 code,int font_dot);

int FontDisp_init_osd_info(T_pWSTR disp_string, T_U16 strLen, Posd_camera_info osd_info);


#ifdef __cplusplus
}
#endif
#endif
