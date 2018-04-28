#include "common.h"
#include "app.h"
#include "mid.h"



/*
 * update showing on screen call back func
 */
void ak_n1_update_osd_off(void)
{
	/* change ini info and enable change */

	Pcamera_disp_setting pcamera_info = anyka_get_camera_info();

	pcamera_info->osd_switch = 0;
	anyka_set_camera_info(pcamera_info);

	anyka_print("[%s:%d] finished\n", __func__, __LINE__);
}



/*
 * update showing on screen call back func
 */
void ak_n1_update_osd_show(char *input, unsigned short *unicode, int unicode_len)
{
	/* change ini info and enable change */

	Pcamera_disp_setting pcamera_info = anyka_get_camera_info();

	/** after next three line, the new info will be show on screen **/
	memcpy(pcamera_info->osd_unicode_name, unicode, unicode_len);
	pcamera_info->osd_unicode_name_len = unicode_len/2;
	pcamera_info->osd_switch = 1;

	/** save utf8 osd name to ini config file **/
	strcpy((char *)pcamera_info->osd_name, input);
	anyka_set_camera_info(pcamera_info);

	anyka_print("[%s:%d] finished\n", __func__, __LINE__);
}

/**
* @brief 	ak_n1_update_osd_font
* 			更新osd显示
* @date 	2015/12
* @param:	utf 8 编码的字符串
* @return 	int
* @retval 	0->success, -1 failed
*/

int ak_n1_update_osd_font(char *input)
{
	int ret;
	
	/** here, if font type is not utf8, you should convert it to utf8 **/
	/** ... **/
	/** code convert **/

	ret = font_update_osd_font_info(input, ak_n1_update_osd_show);
	if(ret) {
		anyka_print("[%s:%d] update osd info failed, ret: %d\n", 
				__func__, __LINE__, ret);
	}
	
	return ret;
}

