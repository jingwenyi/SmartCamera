/** @file
* @brief Font defined head file
*
* Copyright (C) 2006 Anyka (GuangZhou) Software Technology Co., Ltd.
* @author
* @date 2006-01-16
* @version 1.0
*/

#ifndef __AKFONT_LIB_H__
#define __AKFONT_LIB_H__

#ifdef __cplusplus
extern "C" {
#endif

#define FONT_HEIGHT_720 24
#define FONT_WIDTH_720  24
#define FONT_720_BUT_SIZE  (FONT_HEIGHT_720 * FONT_WIDTH_720 / 8)
#define FONT_HEIGHT_VGA 16
#define FONT_WIDTH_VGA  16
#define FONT_VGA_BUT_SIZE  (FONT_HEIGHT_VGA * FONT_WIDTH_VGA / 8)

/**
* @brief   get the font buff
* 
* @author hankejia
* @date 2012-07-05
* @param[in] pstrFilePath  file or dir is path.
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/

unsigned char *font_lib_get_data(unsigned short code, int font_dot);

void font_lib_init(unsigned short *unicode_data, int num);
int font_gb_to_unicode(unsigned char * gb_code, unsigned short * un_code);
int font_unicode_to_gb(unsigned short * un_code, unsigned char * gb_code);
int font_utf8_to_unicode(char* pInput, char* ppOutput);
int font_unicode_to_utf8(const unsigned short *unicode, int ucLen, unsigned char *utf8, int utf8BufLen);

void font_update_osd_info(unsigned short *unicode_data, int num);

typedef void (*update_osd_show)(char * input, unsigned short *unicode, int unicode_len);

int font_update_osd_font_info(char *input_strings, update_osd_show update_osd_cb);



#ifdef __cplusplus
}
#endif
#endif



