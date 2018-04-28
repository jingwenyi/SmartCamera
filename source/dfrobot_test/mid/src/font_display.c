/*
MODELNAME:Font Display
DISCRIPTION:
AUTHOR:WangXi
DATE:2011-02-15
*/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "common.h"
#include "font_display.h"

#define FONTDSP_TEST_API
#define FONTDSP_TAG  "[F_DSP]" //
#define FONT_DATA_START_IDX    (4)//

/**
* @brief draw font 
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/
#define DISP_INTERNEL_N_DOP   2
#define DISP_INTERNEL_A_DOP   2

int FontDisp_F_DispEmpty(T_U8 *FontBuf, T_U16 Font_Height, T_U16 Font_Width)
{
    int i , j, curPixelPos;

      
    for(i = 0; i < Font_Width; i ++)
    {
        for(j = 0; j < Font_Height; j ++)
        {
            curPixelPos = j * Font_Height + i;
            if ((FontBuf[curPixelPos>>3] << (curPixelPos & 7)) & 0x80)
            {
                break;
            }
        }
        if(j != Font_Height)
        {
            return i;
        }
    }
    return i ;
}


int FontDisp_B_DispEmpty(T_U8 *FontBuf, T_U16 Font_Height, T_U16 Font_Width)
{
    int i , j, curPixelPos;
    
    for(i = Font_Width - 1; i > 0; i --)
    {
        for(j = Font_Height - 1; j >= 0; j --)
        {
            curPixelPos = j * Font_Height + i;
            if ((FontBuf[curPixelPos>>3] << (curPixelPos & 7)) & 0x80)
            {
                break;
            }
        }
        if(j >= 0)
        {
            return i;
        }
    }
    return i ;
}

T_U32 FontDisp_DrawStr(FONT_DISP_SET_PIXEL_CB setPixelCbf, T_pVOID param,
        T_POS x, T_POS y, T_pWSTR disp_string, T_U16 strLen, int font_dot)
{
	T_S16  i, j,offset;
    T_POS  xPos;
    T_S16  strlength = (T_S16)strLen;
    T_U8   *FontMatrix = AK_NULL;
    T_U8    fontHeight;
    T_U16   ch;
    T_U32  curPixelPos = 0,prePixelNum=0;
    T_U32  font_w_left  = 0;
    T_U32  font_w_right = 0;

    fontHeight = font_dot;
    xPos = x;
    for(offset=0; (*(disp_string+offset)!='\0') && (strlength>0); offset++, strlength--)
    {
        ch = *(disp_string+offset);

        if(ch == 0x20)
        {
            xPos += font_dot/2;
            continue;
        }
        
        font_w_left  = 0;
        font_w_right = font_dot;
		FontMatrix = font_lib_get_data(ch, font_dot); // get font 
		if (AK_NULL == FontMatrix)
		{
			anyka_print(FONTDSP_TAG"Er:NoFont 0x%x,%c, size %d\n",ch,ch, fontHeight);
			continue;
		}

        if(ch < 0x80 && ch != 0x20)
        {
            int internel;

            if(ch == ':')
            {
                xPos += DISP_INTERNEL_N_DOP;
                internel = DISP_INTERNEL_N_DOP;
            }
            else if(ch =='/' || (ch >= 0x30 && ch < 0x39))
            {
                internel = DISP_INTERNEL_N_DOP;
            }
            else
            {
                internel = DISP_INTERNEL_A_DOP;
            }
            
            font_w_left = FontDisp_F_DispEmpty(FontMatrix, font_dot, font_dot);

            font_w_right = FontDisp_B_DispEmpty(FontMatrix, font_dot, font_dot);

            if(font_w_left > internel)
            {
                font_w_left -= internel ;
            }
            font_w_right += internel;
            if(font_w_left > font_dot)
            {
                font_w_left = font_dot ;
            }
        }
        if ((AK_NULL != setPixelCbf) && (AK_NULL != FontMatrix))
		{
            if(font_w_left>2)
                font_w_left -= 2;
			for (i=0,prePixelNum =0; i<fontHeight; i++, prePixelNum+=fontHeight)// scan height
			{
				for (j=font_w_left; j < font_w_right; j++) //scan  width
				{
					curPixelPos = prePixelNum + j;
					if ((FontMatrix[curPixelPos>>3] << (curPixelPos & 7)) & 0x80)
					{
						setPixelCbf(param, (T_U16)(xPos + j), (T_U16)(y + i)); //fill
					}
				}
			}
		}
		
        xPos += (font_w_right - font_w_left);//
        if (xPos < 0)
        {
            anyka_print("xPos");
            xPos = 0;
        }
    }

    return (T_U32)xPos;
}

T_U32 FontDisp_SetFontMatrix(FONT_DISP_SET_PIXEL_TO_HW_OSD_CB setPixelCbf,
	T_pVOID param, T_U16 code,int font_dot)
{
	T_S16  i, j;
	T_POS  xPos;
	T_U8   *FontMatrix = AK_NULL;
	T_U8   fontHeight;
	T_U16  ch;
	T_U32  curPixelPos = 0, prePixelNum=0;
	T_U32  font_w_left	= 0;
	T_U32  font_w_right = 0;
	T_U8   array_24x24[24 * 24 / 8];
 
	fontHeight = font_dot;	  
	xPos = 0;
	//for (offset=0; (*(disp_string+offset)!='\0') && (strlength>0); offset++, strlength--)
	ch = code;

	if(ch == 0x20)//空格
	{
		font_w_left  = 0;
		font_w_right = fontHeight / 2;
		memset(array_24x24, 0, sizeof(array_24x24));
		FontMatrix = array_24x24;
		goto fill_osd;
	}
	
	font_w_left  = 0;
	font_w_right = fontHeight;
	{
		FontMatrix = font_lib_get_data(ch, fontHeight); // get font 
		
		if (AK_NULL == FontMatrix)
		{
			anyka_print(FONTDSP_TAG"Er:NoFont 0x%x,%c, size %d\n",ch,ch, fontHeight);
			return 0;
		}

		if (ch >= '0' && ch <= '9' )
		{
			font_w_left = 0;
			font_w_right = fontHeight / 2;
		}
		else if(ch < 0x80 && ch != 0x20)
		{
			int internel;

			if(ch == ':')
			{
				//xPos += DISP_INTERNEL_N_DOP;
				internel = DISP_INTERNEL_N_DOP * 2;
			}
			else if(ch =='/' || (ch >= 0x30 && ch < 0x39))
			{
				internel = DISP_INTERNEL_N_DOP;
			}
			else
			{
				internel = DISP_INTERNEL_A_DOP;
			}
			
			font_w_left = FontDisp_F_DispEmpty(FontMatrix, fontHeight, fontHeight);

			font_w_right = FontDisp_B_DispEmpty(FontMatrix, fontHeight, fontHeight);

			if(font_w_left > internel)
			{
				font_w_left -= internel ;
			}
			font_w_right += internel;
			if(font_w_left > fontHeight)
			{
				font_w_left = fontHeight;
			}
		}
	}
fill_osd:
	if ((AK_NULL != setPixelCbf) && (AK_NULL != FontMatrix))
	{
		if(font_w_left>2)
			font_w_left -= 2;

		/* 保证每个font都是偶数width */
		if ((font_w_right - font_w_left) & 0x1)
		{
			if (font_w_right < fontHeight)
				font_w_right++;
			else
				font_w_left--;
		}
		//anyka_print("font width:%d\n",(font_w_right - font_w_left));
		for (i=0,prePixelNum =0; i<fontHeight; i++, prePixelNum+=fontHeight)// scan height
		{
			for (j=font_w_left; j < font_w_right; j++) //scan  width
			{
				curPixelPos = prePixelNum + j;
				int pixel_value = (FontMatrix[curPixelPos>>3] << (curPixelPos & 7)) & 0x80;
				if (setPixelCbf(param, (T_U16)(j - font_w_left), (T_U16)i, pixel_value))
					return xPos;
			}
		}
	}
	xPos += (font_w_right - font_w_left);//
	
	if (xPos < 0)
	{
		anyka_print("xPos");
		xPos = 0;
	}

	return (T_U32)xPos;
}

#if 0
int FontDisp_init_osd_info(T_pWSTR disp_string, T_U16 strLen, Posd_camera_info osd_info)
{
	T_S16  i, offset;
    T_S16  strlength = (T_S16)strLen;
    T_U8   *FontMatrix = AK_NULL;
    T_U8    fontHeight;
    T_U16   ch;
    T_U32  font_w_left  = 0;
    T_U32  font_w_right = 0;
    int osd_width = 0;

 
    fontHeight = FONT_HEIGHT_DOT;    
    for (offset=0; (*(disp_string+offset)!='\0') && (strlength>0); offset++, strlength--)
    {
        ch = *(disp_string+offset);

        if(ch == 0x20)
        {
            osd_info[offset].start_x = 0;
            osd_info[offset].dot_x = FONT_WIDTH_DOT/2;
            osd_info[offset].font_buf = NULL;
            osd_width += osd_info[offset].dot_x;
            continue;
        }
        
        font_w_left  = 0;
        font_w_right = FONT_WIDTH_DOT;

		{
			FontMatrix = font_lib_get_data(ch); // get font 
            osd_info[offset].font_buf = FontMatrix;
			
			if (AK_NULL == FontMatrix)
			{
                osd_info[offset].start_x = 0;
                osd_info[offset].dot_x = FONT_WIDTH_DOT/2;
                osd_width += osd_info[offset].dot_x;
				continue;
			}

            if(ch < 0x80 && ch != 0x20)
            {
                int internel;

                if(ch == ':' || ch =='/' || (ch >= 0x30 && ch < 0x39))
                {
                    internel = DISP_INTERNEL_N_DOP;
                }
                else
                {
                    internel = DISP_INTERNEL_A_DOP;
                }
                i = FontDisp_F_DispEmpty(FontMatrix, FONT_HEIGHT_DOT, FONT_HEIGHT_DOT);
                
                if(i > internel)
                {
                    font_w_left = i - internel;
                }          
                i = FontDisp_B_DispEmpty(FontMatrix, FONT_HEIGHT_DOT, FONT_HEIGHT_DOT);
                
                font_w_right = i + internel;
                if(font_w_right > FONT_HEIGHT_DOT)
                {
                    font_w_right = FONT_HEIGHT_DOT;
                }
                
            }
		}
        
        if(font_w_left>2)
            font_w_left -= 2;


        osd_info[offset].start_x = 0;
        osd_info[offset].dot_x = font_w_right - font_w_left;
        if(osd_info[offset].dot_x & 1)
        {
            osd_info[offset].dot_x ++;
        }
        osd_width += osd_info[offset].dot_x;
        
    }

    return osd_width;
}
#endif

