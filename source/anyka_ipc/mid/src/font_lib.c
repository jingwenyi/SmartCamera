#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "common.h"    
#include "font_lib.h"
    

#define UNICODE_START_CODE  0X4E00
#define UNICODE_END_CODE    0X9FA5

#define FONT_720_DATA_PATH  "/usr/bin/anyka_font_24.bin"
#define FONT_VGA_DATA_PATH  "/usr/bin/anyka_font_16.bin"
#define GB_UNICODE_PATH  	"/usr/bin/gb_un.bin"
//#define TWO_FONT_SWITCH
/**
* @brief  return font buf
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
typedef struct _font_data_720_
{
    unsigned short unicode;
    unsigned char   data[FONT_720_BUT_SIZE];
}font_data_720, *Pfont_data_720;

typedef struct _font_data_vga_
{
    unsigned short unicode;
    unsigned char   data[FONT_VGA_BUT_SIZE];
}font_data_vga, *Pfont_data_vga;

typedef struct _font_data_info_
{
    int font_num;
    int font_fd_vga ;
    int font_fd_720 ;
    unsigned char *all_font_buf_vga ;
    unsigned char *all_font_buf_720 ;
    int total_font_num;
    Pfont_data_vga font_buf_vga;
    Pfont_data_720 font_buf_720;
}font_data_info;

font_data_info all_font_buffer = {0, -1, -1,NULL, NULL, 0, NULL, NULL}, all_font_buffer_in_used;

static pthread_mutex_t font_mutex;

#ifdef CONFIG_TENCENT_SUPPORT

/**
 * NAME        font_lib_get_font_from_bin
 * @BRIEF	从bin 文件获取字体
                  
 * @PARAM	T_U16 code,  
 			unsigned char *font_buffer_720,  store 720p font data
			unsigned char *font_buffer_vga,  store vga font data
 			
 * @RETURN	void
 * @RETVAL	
 */

void font_lib_get_font_from_bin(T_U16 code,  unsigned char *font_buffer_720,  unsigned char *font_buffer_vga)
{
    int i, cur;

    if(all_font_buffer.font_fd_720 < 0)
    {
        #ifdef TWO_FONT_SWITCH
        all_font_buffer.font_fd_vga = open(FONT_VGA_DATA_PATH, O_RDONLY);
        #endif
        all_font_buffer.font_fd_720 = open(FONT_720_DATA_PATH, O_RDONLY);
        all_font_buffer.all_font_buf_720 = (unsigned char *)malloc(20*1024);
        #ifdef TWO_FONT_SWITCH
        all_font_buffer.all_font_buf_vga = (unsigned char *)malloc(20*1024);
        #endif
        lseek(all_font_buffer.font_fd_720, 0, SEEK_SET);
        all_font_buffer.total_font_num = read(all_font_buffer.font_fd_720, all_font_buffer.all_font_buf_720, 20*1024);
        all_font_buffer.total_font_num = all_font_buffer.total_font_num / (FONT_720_BUT_SIZE + 2);        
        #ifdef TWO_FONT_SWITCH
        lseek(all_font_buffer.font_fd_vga, 0, SEEK_SET);
        read(all_font_buffer.font_fd_vga, all_font_buffer.all_font_buf_vga, 20*1024);
        #endif
    }

    if(all_font_buffer.font_fd_720 < 0)
    {
        anyka_print("[%s:%d] we can't open the font data bin\n", __func__, __LINE__);
        return ;
    }
    

    for(i = 0; i < all_font_buffer.total_font_num; i++)
    {
        cur = *(unsigned short *)(all_font_buffer.all_font_buf_720 + i *(FONT_720_BUT_SIZE + 2));
        if(cur == code)
        {
            memcpy(font_buffer_720, all_font_buffer.all_font_buf_720 + i *(FONT_720_BUT_SIZE + 2) + 2, FONT_720_BUT_SIZE);
            break;
        }
    }
    
    #ifdef TWO_FONT_SWITCH
    for(i = 0; i < all_font_buffer.total_font_num; i++)
    {
        cur = *(unsigned short *)(all_font_buffer.all_font_buf_vga + i *(FONT_VGA_BUT_SIZE + 2));
        if(cur == code)
        {
            memcpy(font_buffer_vga, all_font_buffer.all_font_buf_vga + i *(FONT_VGA_BUT_SIZE + 2) + 2, FONT_VGA_BUT_SIZE);
            break;
        }
    }
    #endif
}
#else


/**
 * NAME        font_lib_get_font_from_bin
 * @BRIEF	从bin 文件获取字体
                  
 * @PARAM	T_U16 code,  
 			unsigned char *font_buffer_720,  store 720p font data
			unsigned char *font_buffer_vga,  store vga font data
 			
 * @RETURN	void
 * @RETVAL	
 */

void font_lib_get_font_from_bin(T_U16 code,  unsigned char *font_buffer_720,  unsigned char *font_buffer_vga)
{
    int offset;

    
    if(code < 0x80)
    {
        offset = (code - 1);
    }
    else if(code >= UNICODE_START_CODE && code <= UNICODE_END_CODE)
    {
        offset = (127 + code - UNICODE_START_CODE);
    }
    else
    {
        //系统中无此字体信息
        offset = 0x1F;
        //anyka_print("[%s:%d]we don't include the font(0x%04x)\n", __func__, __LINE__, code);
    }
    if(all_font_buffer.font_fd_720 < 0)
    {
        all_font_buffer.font_fd_720= open(FONT_720_DATA_PATH, O_RDONLY);        
        #ifdef TWO_FONT_SWITCH
        all_font_buffer.font_fd_vga = open(FONT_VGA_DATA_PATH, O_RDONLY);
        #endif
    }

    if(all_font_buffer.font_fd_720 < 0)
    {
        anyka_print("[%s:%d] we can't open the font data bin\n", __func__, __LINE__);
        return ;
    }
    lseek(all_font_buffer.font_fd_720, offset * FONT_720_BUT_SIZE, SEEK_SET);
    read(all_font_buffer.font_fd_720, font_buffer_720, FONT_720_BUT_SIZE);
    #ifdef TWO_FONT_SWITCH
    lseek(all_font_buffer.font_fd_vga, offset * FONT_VGA_BUT_SIZE, SEEK_SET);
    read(all_font_buffer.font_fd_vga, font_buffer_vga, FONT_VGA_BUT_SIZE);
    #endif
}
#endif


/**
 * NAME        font_lib_init
 * @BRIEF	初始化字体库
                  
 * @PARAM	unsigned short *unicode_data,  unicode data
 			int num,	unicode data number
 * @RETURN	void
 * @RETVAL	
 */

void font_lib_init(unsigned short *unicode_data, int num)
{
	
    int i, len;
    //标准的显示汉字，包括"星期一二三四五六天年月日"
    unsigned short all_number[] = {'-', ':', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    		'S', 'u', 'M', 'o', 'n','T', 'e', 's', 'W', 'd', 'h', 'r', 'F',  'i', 'S', 'a', 0x5e74,
        	0x6708, 0x65e5, 0x661f, 0x671f, 0x5929, 0x4e00, 0x4e8c, 0x4e09, 0x56db, 0x4e94, 0x516d};

    len = sizeof(all_number) / sizeof(all_number[1]);
	
#if 0
    if(all_font_buffer.font_buf_720)
    {
        free(all_font_buffer.font_buf_720);
#ifdef TWO_FONT_SWITCH
        free(all_font_buffer.font_buf_vga);
#endif
    }
#endif


    all_font_buffer.font_num = len + num;
    all_font_buffer.font_buf_720 = malloc(sizeof(font_data_720) *(len + num));
#ifdef TWO_FONT_SWITCH
    all_font_buffer.font_buf_vga = malloc(sizeof(font_data_vga) *(len + num));
#endif

    for(i = 0; i < len; i++)
    {
        all_font_buffer.font_buf_720[i].unicode = all_number[i];        
#ifdef TWO_FONT_SWITCH
        all_font_buffer.font_buf_vga[i].unicode = all_number[i];
        font_lib_get_font_from_bin(all_font_buffer.font_buf_720[i].unicode, all_font_buffer.font_buf_720[i].data, all_font_buffer.font_buf_vga[i].data);
#else
        font_lib_get_font_from_bin(all_font_buffer.font_buf_720[i].unicode, all_font_buffer.font_buf_720[i].data, NULL);

#endif
    }
    for(i = 0; i < num; i++)
    {
        all_font_buffer.font_buf_720[i+len].unicode = unicode_data[i];
#ifdef TWO_FONT_SWITCH
        all_font_buffer.font_buf_vga[i+len].unicode = unicode_data[i];
        font_lib_get_font_from_bin(all_font_buffer.font_buf_720[i+len].unicode, all_font_buffer.font_buf_720[i+len].data, all_font_buffer.font_buf_vga[i+len].data);
#else
        font_lib_get_font_from_bin(all_font_buffer.font_buf_720[i+len].unicode, all_font_buffer.font_buf_720[i+len].data, NULL);
#endif
    }
    
#ifdef CONFIG_TENCENT_SUPPORT
    close(all_font_buffer.font_fd_720);
	all_font_buffer.font_fd_720 = -1;
    free(all_font_buffer.all_font_buf_720);
#ifdef TWO_FONT_SWITCH
    close(all_font_buffer.font_fd_vga);
    free(all_font_buffer.all_font_buf_vga);
	all_font_buffer.font_fd_vga = -1;
#endif

#endif   

	/* init */
	if(all_font_buffer_in_used.font_num == 0) {
		pthread_mutex_init(&font_mutex, NULL);
		all_font_buffer_in_used = all_font_buffer;
		all_font_buffer.font_buf_720 = NULL;
		anyka_print("init all font buffer inused, font_num: %d\n", all_font_buffer_in_used.font_num);
	}else 
		anyka_print("all font buffer in used font_num: %d\n", all_font_buffer_in_used.font_num);

}


/**
 * NAME        font_update_osd_info
 * @BRIEF	更新字体库
                  
 * @PARAM	unsigned short *unicode_data,  unicode data
 			int num,	unicode data number
 * @RETURN	void
 * @RETVAL	
 */

void font_update_osd_info(unsigned short *unicode_data, int num)
{	
	font_data_info tmp_font_buffer = {0};

	font_lib_init(unicode_data, num); //all_font_buffer re init

	pthread_mutex_lock(&font_mutex);
	tmp_font_buffer = all_font_buffer_in_used;
	all_font_buffer_in_used = all_font_buffer;
	pthread_mutex_unlock(&font_mutex);
	
	/** release resource **/
	if(tmp_font_buffer.font_buf_720) {
		free(tmp_font_buffer.font_buf_720);
#ifdef TWO_FONT_SWITCH
        free(tmp_font_buffer.font_buf_vga);
#endif
	}
	
}


/**
 * NAME        font_lib_get_data
 * @BRIEF	获取字体库数据
                  
 * @PARAM	unsigned short code, 
 			int font_dot
 * @RETURN	void
 * @RETVAL	
 */

unsigned char *font_lib_get_data(unsigned short code, int font_dot)
{
    int i;
	
    pthread_mutex_lock(&font_mutex);
    for(i = 0; i < all_font_buffer_in_used.font_num; i++)
    {
        if(all_font_buffer_in_used.font_buf_720[i].unicode == code)
        {
        #ifndef TWO_FONT_SWITCH
			pthread_mutex_unlock(&font_mutex);
            return all_font_buffer_in_used.font_buf_720[i].data;
        #else
            if(font_dot == FONT_HEIGHT_720)
            {
            	pthread_mutex_unlock(&font_mutex);
                return all_font_buffer_in_used.font_buf_720[i].data;
            }            
            else
            {
            	pthread_mutex_unlock(&font_mutex);
                return all_font_buffer_in_used.font_buf_vga[i].data;
            }
        #endif
            
        }
    }
	pthread_mutex_unlock(&font_mutex);
    return NULL;
    
}


/**
 * NAME        font_gb_to_unicode
 * @BRIEF	gb 码转unicode
                  
 * @PARAM	unsigned char * gb_code, 
 			unsigned short * un_code
 * @RETURN	int
 * @RETVAL	0 failed, success un_len.
 */


int font_gb_to_unicode(unsigned char * gb_code, unsigned short * un_code)
{
    unsigned short cur_code;
    int i, un_gb_file, max_code_num, un_len = 0;
    char *src = ( char *)gb_code;
    unsigned short * file_buf;
    un_gb_file = open(GB_UNICODE_PATH, O_RDONLY);
    if(un_gb_file < 0)
    {
        anyka_print("[%s:%d]it fails to open :%s\n", __func__, __LINE__, GB_UNICODE_PATH);
        return 0;
    }
    max_code_num = UNICODE_END_CODE - UNICODE_START_CODE + 10;
    file_buf = (unsigned short * )malloc(max_code_num* 2);
    if(file_buf == NULL)
    {
        close(un_gb_file);
        return 0;
    }
    
    max_code_num = read(un_gb_file, file_buf, max_code_num* 2);
    max_code_num >>= 1;

    while(*src)
    {
        if(*src < 0x80)
        {
            *un_code = *src;
        }
        else
        {
            cur_code = *(src + 1);
            cur_code = (cur_code << 8) + *src;
            src ++;
            for(i = 0; i < max_code_num; i++)
            {
                if(cur_code == file_buf[i])
                {
                    break;
                }
            }
            if(i != max_code_num)
            {
                *un_code = UNICODE_START_CODE + i;
            }
            else
            {
                *un_code = 0x20;
                anyka_print("[%s:%d]:the gb code don't find!(0x%x)\n", __func__, __LINE__, cur_code);
            }
        }
        un_code ++;
        src ++;
        un_len ++;
    }
    *un_code = 0;
    close(un_gb_file);
    free(file_buf);
    return un_len;
}


/**
 * NAME        font_unicode_to_gb
 * @BRIEF	unicode 码转gb 
                  
 * @PARAM	unsigned short * un_code, 
 			unsigned char * gb_code
 * @RETURN	int
 * @RETVAL	0 failed, success un_len.
 */

int font_unicode_to_gb(unsigned short * un_code, unsigned char * gb_code)
{
    unsigned short cur_code;
    int un_gb_file, max_code_num, len = 0;
    unsigned short * file_buf;
    un_gb_file = open(GB_UNICODE_PATH, O_RDONLY);
    if(un_gb_file < 0)
    {
        anyka_print("[%s,%d], it fails to open :%s\n", __func__, __LINE__, GB_UNICODE_PATH);
        return 0;
    }
    max_code_num = 0x9fa5 - 0x4e00 + 10;
    file_buf = (unsigned short *)malloc(max_code_num* 2);
    if(file_buf == NULL)
    {
        anyka_print("[%s,%d], it fails to malloc:%d\n", __func__, __LINE__, max_code_num* 2);
        close(un_gb_file);
        return 0;
    }
    
    max_code_num = read(un_gb_file, file_buf, max_code_num* 2);
    max_code_num >>= 1;
    while(*un_code)
    {
        if(*un_code < 0x80)
        {
            *gb_code = *un_code;
        }
        else if(*un_code >= UNICODE_START_CODE && *un_code <= UNICODE_END_CODE)
        {
            cur_code = file_buf[*un_code - UNICODE_START_CODE];
            *gb_code = cur_code & 0xFF;
            gb_code ++;
            *gb_code = cur_code >> 8;
        }
        else
        {
            anyka_print("[%s:%d]the unicode code don't find!(0x%x)\n", __func__, __LINE__, *un_code);
            *gb_code = ' ';
        }
        gb_code ++;
        un_code ++;
        len ++;
    }
    *un_code = 0;
    close(un_gb_file);
    free(file_buf);
    return len;
}



/**
 * NAME        font_utf8_to_unicode
 * @BRIEF	utf8 码转unicode 
                  
 * @PARAM	char* pInput, 
 			char* ppOutput,
 * @RETURN	int
 * @RETVAL	0 failed, success un_len.
 */

int font_utf8_to_unicode(char* pInput, char* ppOutput)
{
    int outputSize = 0; 
    
    memset(ppOutput, 0, strlen(pInput) * 2);
    char *tmp = ppOutput; //临时变量，用于遍历输出字符串
 
    while (*pInput)
    {
        if (*pInput > 0x00 && *pInput <= 0x7F) //处理单字节UTF8字符（英文字母、数字）
        {
            *tmp = *pInput;
            tmp++;
            *tmp = 0; //小端法表示，在高地址填补0
        }
        else if (((*pInput) & 0xE0) == 0xC0) //处理双字节UTF8字符
        {
            char high = *pInput;
            pInput++;
            char low = *pInput;            
            if ((low & 0xC0) != 0x80)  //检查是否为合法的UTF8字符表示
            {
                return 0; //如果不是则报错
            }
   
            *tmp = (high << 6) + (low & 0x3F);
            tmp++;
            *tmp = (high >> 2) & 0x07;
        }
        else if (((*pInput) & 0xF0) == 0xE0) //处理三字节UTF8字符
        {
            char high = *pInput;
            pInput++;
            char middle = *pInput;
            pInput++;
            char low = *pInput;            
            if (((middle & 0xC0) != 0x80) || ((low & 0xC0) != 0x80))
            {
                return 0;
            }            
			*tmp = (middle << 6) + (low & 0x7F);
            tmp++;
            *tmp = (high << 4) + ((middle >> 2) & 0x0F); 
        }
        else //对于其他字节数的UTF8字符不进行处理
        {
            return 0;
        }        
		pInput ++;
        tmp ++;
        outputSize += 2;
    }    
	*tmp = 0;
    tmp++;
    *tmp = 0;    
    return outputSize;
} 

/**
 * NAME        font_unicode_to_utf8
 * @BRIEF	unicode 码转utf8
                  
 * @PARAM	const unsigned short *unicode, 
 			int ucLen, 
 			unsigned char *utf8, 
 			int utf8BufLen
 * @RETURN	int
 * @RETVAL	0 failed, success un_len.
 */

int font_unicode_to_utf8(const unsigned short *unicode, int ucLen, unsigned char *utf8, int utf8BufLen)
{
    int i=0, j=0;
    int uc = 0;
 
    while (unicode[i]!=0 && i<ucLen)
    {
        uc = unicode[i];
 
        if (uc < (unsigned short)0x80)
        {
            if(0!=utf8BufLen)
            {
                utf8[j] = (unsigned char)uc;
            }
            ++j;
        }
        else if (uc < (unsigned short)0x800)
        {
            if(0!=utf8BufLen)
            {
                utf8[j] = (uc>>6)|0xc0;
                utf8[j+1] = (uc&0x3f)|0x80;
            }
            j+=2;
        }
        else
        {
            if(0!=utf8BufLen)
            {
                utf8[j] = ((uc>>12)&0x0f)|0xe0;
                utf8[j+1] = ((uc>>6)&0x3f)|0x80;
                utf8[j+2] = (uc&0x3f)|0x80;
            }
            j+=3;
        }
        ++i;
    }
    if(0 != utf8BufLen)
    {
        utf8[j] = 0;
    }
    ++j;
    return j;
 
}

/**
* @brief 	font_update_osd_font_info
* 			更新字库内容并调用回调函数将内容更新显示到屏幕上
* @date 	2015/12
* @param:	utf 8 编码的字符串， 回调函数
* @return 	int
* @retval 	0->success, -1 failed
*/

int font_update_osd_font_info(char *input_strings, update_osd_show update_osd_cb)
{
	/*
	** 只完成更新font lib 相关东西，要最终显示改变还要改字体显示用到的句柄的内容
	** 这部分在回调里面处理，因为考虑到后面实际修改句柄信息相关内容会改变。
	*/
	int uni_len;
	unsigned short osd_unicode[100] = {0};

	/** must check this return val **/
	if(strlen(input_strings) > 45) {
		anyka_print("[%s:%d] Error, input string is too long\n", __func__, __LINE__);				
		return -1;
	}

	/** must convert encode to unicode, here we check whether the input is valid **/
	uni_len = font_utf8_to_unicode(input_strings, (char *)osd_unicode);
	if(!uni_len) {
		anyka_print("[%s:%d] input string error, it's encode was not utf8\n", __func__, __LINE__);
		return -1;
	}

	/** update font info at first **/
	font_update_osd_info(osd_unicode, uni_len/2);

	if(update_osd_cb) {
		/** call call_back func to update handle info **/
		update_osd_cb(input_strings, osd_unicode, uni_len);
	} else {
		anyka_print("[%s:%d] Warning, callback is null, we don't show font on screen\n", __func__, __LINE__);
	}
	anyka_print("[%s:%d] update osd info done ...\n", __func__, __LINE__);
	return 0;
}

