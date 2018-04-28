/** @file
* @brief Define the ISP server api
*
* Copyright (C) 2015 Anyka (GuangZhou) Software Technology Co., Ltd.
* @author
* @date 2015-07-24
* @version 1.0
*/

#ifndef __ISP_MODULE_H__
#define __ISP_MODULE_H__

#include "anyka_types.h"
#include "ak_isp_char.h"

#define ISP_MODULE_MAX_SIZE		(1024*12)


typedef enum {
	ISP_BB = 0,   			//ºÚÆ½ºâ
	ISP_LSC,				//¾µÍ·Ð£Õý
	ISP_RAW_LUT,			//raw gamma
	ISP_NR,					//NR
	ISP_3DNR,				//3DNR

	ISP_GB,					//ÂÌÆ½ºâ
	ISP_DEMO,				//DEMOSAIC
	ISP_GAMMA,				//GAMMA
	ISP_CCM,				//ÑÕÉ«Ð£Õý
	ISP_FCS,				//FCS

	ISP_WDR,				//WDR
	ISP_EDGE,				//EDGE
	ISP_SHARP,				//SHARP
	ISP_SATURATION,			//±¥ºÍ¶È
	ISP_CONSTRAST,			//¶Ô±È¶È

	ISP_RGB2YUV,			//rgb to yuv
	ISP_YUVEFFECT,			//YUVÐ§¹û
	ISP_DPC,				//»µµãÐ£Õý
	ISP_WEIGHT,				//È¨ÖØÏµÊý
	ISP_AF,					//AF

	ISP_WB,					//WB	
	ISP_EXP,				//Expsoure
	ISP_MISC,				//ÔÓÏî

	ISP_3DSTAT,				//3D½µÔëÍ³¼Æ
	ISP_AESTAT,				//AEÍ³¼Æ
	ISP_AFSTAT,				//AFÍ³¼Æ
	ISP_AWBSTAT,			//AWBÍ³¼Æ

	ISP_SENSOR,				//sensor²ÎÊý

    ISP_MODULE_ID_NUM
} T_MODULE_ID;

typedef enum {
	MODE_NIGHTTIME,
	MODE_DAYTIME,
} T_MODULE_MODE;

typedef enum {
	EFFECT_HUE = 0,
	EFFECT_BRIGHTNESS,
	EFFECT_SATURATION,
	EFFECT_CONTRAST,
	EFFECT_SHARP,
} T_EFFECT_TYPE;

typedef struct {
	T_S32 a_gain;
	T_S32 d_gain;
	T_S32 isp_d_gain;
} T_AESTAT_INFO;

typedef struct {
	T_U16   g_weight[16];
	T_U16   y_low;               //y_low<=y_high
	T_U16   y_high;
	T_U16   gr_low[5];            //gr_low[i]<=gr_high[i]
	T_U16   gb_low[5];            //gb_low[i]<=gb_high[i]
	T_U16   gr_high[5];
	T_U16   gb_high[5];
	T_U16   rb_low[5];           //rb_low[i]<=rb_high[i]
	T_U16   rb_high[5];

	//awb¿¼þ²¿·¿¿¿¿µ¿¿
	T_U16   auto_wb_step;                 //°¿½º¿³¤¼¿
	T_U16   total_cnt_thresh;         //¿¿¸¿¿µ
	T_U16   colortemp_stable_cnt_thresh;    //¿¶¨¿¿£¬¶¿¿¿¿¿¿»·¾³¿¿¸¿
} T_AWB_ATTR;

T_S32 Isp_Dev_Open(void);

T_S32 Isp_Dev_Close(void);

T_S32 Isp_Module_Init(T_U8* cfgBuf, T_U32 size, T_U32 phyaddr, T_U32 width, T_U32 height);

T_S32 Isp_Module_Get(T_U16 module_id, T_U8* buf, T_U32* size);

T_S32 Isp_Module_Set(T_U16 module_id, T_U8* buf, T_U32 size);

T_S32 Isp_Module_StatInfo_Get(T_U16 module_id, T_U8* buf, T_U32* size);

T_S32 Isp_Sensor_Get(T_U16 addr, T_U16 *value);

T_S32 Isp_Sensor_Set(T_U16 addr, T_U16 value);

T_S32 Isp_Sensor_Load_Conf(T_U16 *sensor_regs);

T_S32 Isp_Module_Set_User_Params(AK_ISP_USER_PARAM *param);

T_S32 Isp_Module_Switch_Mode(T_MODULE_MODE mode);

T_S32 Isp_Module_AeStat_Get(T_AESTAT_INFO *aestat);

T_S32 Isp_Module_AeAttr_Exptime_Max_Set(T_S32 exptime_max);

T_S32 Isp_Module_Set_Isp_Pause(void);

T_S32 Isp_Module_Set_Isp_Resume(void);

T_S32 Isp_Module_Set_awb_attr(T_AWB_ATTR *p_awb);

T_S32 Isp_Module_Get_awb_attr(T_AWB_ATTR *p_awb);


/**
 * NAME         Isp_Effect_Set
 * @BRIEF	¿Í»§¶ËÉèÖÃÊÓÆµÍ¼ÏñÐ§¹û
 * @PARAM	type : 
                    value  [-50, 50],  0Îª²»µ÷ÕûÐ§¹û£¬Ê¹ÓÃisp.confÀïµÄÐ§¹û¡£
 * @RETURN	¶ÓÁÐÎ²Êý¾ÝÏî
 * @RETVAL	-1-->fail; 0-->ok
 */
T_S32 Isp_Effect_Set(T_EFFECT_TYPE type, T_S32 value);


#endif

