
/**
* @brief  camera
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include "common.h"
#include "font_lib.h"
#include "camera.h"
#include "akuio.h"
#include "isp_conf_file.h"
#include "isp_module.h"

#define C39E

#define STANDBY_MODE 0 //temp modify by xc

#define FONTDSP_TEST_API
#define FONTDSP_TAG  "[F_DSP]"
#define FONT_DATA_START_IDX    (4)
#define BUFF_NUM 	4
#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define ENOUGH_MEM(min_type, max_type) assert(sizeof(min_type) <= sizeof(max_type))
#define OSD_COLOR   2

#define V4L2_CID_SENSOR_POWERON     (V4L2_CID_USER_CLASS + 1)
#define V4L2_CID_SENSOR_POWEROFF    (V4L2_CID_USER_CLASS + 2)
#define V4L2_CID_SENSOR_STANDBYIN   (V4L2_CID_USER_CLASS + 3)
#define V4L2_CID_SENSOR_STANDBYOUT  (V4L2_CID_USER_CLASS + 4)
#define V4L2_CID_SENSOR_MCLK        (V4L2_CID_USER_CLASS + 5)
#define V4L2_CID_SENSOR_ID          (V4L2_CID_USER_CLASS + 6)
#define V4L2_CID_SENSOR_FPS         (V4L2_CID_USER_CLASS + 7)
#define V4L2_CID_SENSOR_RESOLUTION	(V4L2_CID_USER_CLASS + 8)

/////////////////////////////OSD start/////////////////////////////////////////////////////////////////////
#define MAIN_CHAN_OSD_PER_LINE_BYTES			7680	//max 7680. 主通道每行字体最大占用字节数
#define SUB_CHAN_OSD_PER_LINE_BYTES				5760	//max 5760. 次通道每行字体最大占用字节数
#define OSD_LINES								2		//可显示OSD字体行数
#define MAIN_CHAN_OSD_BUFFER_SIZE				(MAIN_CHAN_OSD_PER_LINE_BYTES * OSD_LINES)	//主通道OSD可用最大buffer
#define SUB_CHAN_OSD_BUFFER_SIZE				(SUB_CHAN_OSD_PER_LINE_BYTES * OSD_LINES)	//次通道OSD可用最大buffer
#define OSD_UNICODE_CODES_NUM					100		//最多可显示字符数

#define OPAQUE_ALPHA							0		//不透明的alpha
#define CRYSTAL_CLEAR_ALPHA						15		//完全透明的alpha

#define CRYSTAL_CLEAR_COLOR_TABLE_INDEX 		0		//完全透明的color table
#define WHITE_COLOR_TABLE_INDEX 				1		//白色的color table
#define BLACK_COLOR_TABLE_INDEX 				2		//黑色的color table

#define DEF_ALPHA								((OPAQUE_ALPHA + CRYSTAL_CLEAR_ALPHA) * 5 / 10)		//全局alpha
#define DEF_1ST_FONTS_COLOR_TABLE_INDEX 		WHITE_COLOR_TABLE_INDEX			//第一行字体颜色: 白色
#define DEF_2ND_FONTS_COLOR_TABLE_INDEX 		WHITE_COLOR_TABLE_INDEX			//第二行字体颜色: 白色
#define DEF_1ST_GROUND_COLOR_TABLE_INDEX		BLACK_COLOR_TABLE_INDEX			//第一行底色: 黑色
#define DEF_2ND_GROUND_COLOR_TABLE_INDEX		BLACK_COLOR_TABLE_INDEX			//第二行底色: 黑色

#define MAIN_OSD_RELATIVE_OFFSET_X				24		//主通道OSD横轴和纵轴偏移
#define MAIN_OSD_RELATIVE_OFFSET_Y				6
#define SUB_OSD_RELATIVE_OFFSET_X				16		//次通道OSD横轴和纵轴偏移
#define SUB_OSD_RELATIVE_OFFSET_Y				6

/////////////////////////////OSD end/////////////////////////////////////////////////////////////////////

struct tagRECORD_VIDEO_FONT_CTRL {
	T_U8 *y;
	T_U8 *u;
	T_U8 *v;
	T_U32 color;
	T_U32 width;
};

struct tagRECORD_VIDEO_FONT_HW_OSD_CTRL {
	T_U8 *p_context;
	T_U16 osd_row_bytes;
	T_U16 font_size;
	T_U16 x_offset_pixels;
	T_U16 color_table_index;
	T_U16 color_table_index_ground;
};

typedef struct {
    T_U32             timeStampColor;
    struct tm         curTime;
    char              timeStr[25];
    T_U8              timeStrLen;
	T_U32             color;
	T_POS             timePos_X;
	T_POS             timePos_Y;
} T_VIDEO_OSD_REC_ICON;

//typedef T_VOID (*FONT_DISP_SET_PIXEL_CB)(T_pVOID param, T_U16 pos_x, T_U16 pos_y);

typedef struct {
	FUNC_DMA_MALLOC dma_malloc;
	FUNC_DMA_FREE	dma_free;
    T_pVOID pframebuf;
} T_CAMERA_STRC;

struct buffer {
        void   *start;
        size_t  length;
};

typedef enum
{
	CAMERA_MIRROR_NORMAL = 0,
    CAMERA_MIRROR_V,
    CAMERA_MIRROR_H,
    CAMERA_MIRROR_FLIP,
    CAMERA_MIRROR_NUM
}T_CAMERA_MIRROR;

struct v4l2_buffer buf;


static char            *dev_name = "/dev/video0";
static int              fd = -1;
struct buffer          *buffers;
static int              force_format = 1;
static void *ion_mem = NULL;
static void *ion_isp_mem = NULL;
char *camera_osd_buf = NULL;

unsigned int def_color_tables[] = 
{
	0x000000, 0xff7f7f, 0x007f7f, 0x266ac0, 0x71408a, 0x4b554a, 0x599540, 0x0ec075,
	0x34aab5, 0x786085, 0x2c8aa0, 0x68d535, 0x34aa5a, 0x43e9ab, 0x4b55a5, 0x008080
};


//RGB 2 YUV (8bit) 
#define RGB2Y(R,G,B)   ((306*R + 601*G + 117*B)>>10)
#define RGB2U(R,G,B)   ((-173*R - 339*G + 512*B + 131072)>>10)
#define RGB2V(R,G,B)   ((512*R - 428*G - 84*B + 131072)>>10)

static void isp_exptime_max_set(void);
static int fps_conf_info_init(void);

static int xioctl(int fh, int request, void *arg)
{
	int r;

	do {
		r = ioctl(fh, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}

/**
* @brief   set fonts hw osd fomat matrix
* @author ye_guohong
* @date 2015-11-14
* @return T_S8	0: success; -1: not enought buffer
*/
static T_S8 SetFontMatrix(struct tagRECORD_VIDEO_FONT_HW_OSD_CTRL *ctrl, T_U16 pos_x,
									T_U16 pos_y, T_S8 pixel_value)
{

	T_U8 *p_osd_context = ctrl->p_context;
	int font_size = ctrl->font_size;
	int per_line_bytes = ctrl->osd_row_bytes;
	T_U8 *which_byte = p_osd_context + ctrl->x_offset_pixels / 2 + pos_y * per_line_bytes + pos_x / 2;
	int is_higher_bits = pos_x & 0x01;
	int osd_table_index = ctrl->color_table_index;
	int osd_table_index_ground = ctrl->color_table_index_ground;
	int value = pixel_value ? osd_table_index:osd_table_index_ground;

	assert(pos_y < font_size);

	if (pos_x + ctrl->x_offset_pixels >= per_line_bytes * 2)
	{
		anyka_print("SetFontsMatrix pos_x too large pos_x:%d,x_offset_pixels:%d\n",pos_x, ctrl->x_offset_pixels);
		return -1;
	}

	if (is_higher_bits)
	{
		*which_byte = (*which_byte & 0xf) | (value << 4);
	}
	else
	{
		*which_byte = (*which_byte & 0xf0) | value;
	}

	return 0;
}
	

/**
* @brief   set pixel on video
* @author wangxi
* @date 2012-10-31
* @return NONE
*/
/*
static T_VOID SetFontPixelOnVideo(struct tagRECORD_VIDEO_FONT_CTRL *ctrl, T_U16 pos_x, T_U16 pos_y)
{
    T_U32 uvpos;

	if ( ctrl == NULL )
    {
		return;
	}
	ctrl->y[(ctrl->width) * pos_y + (pos_x)] = 224; //(T_U8) RGB2Y(232,224,206); //(T_U8)ctrl->color&0xFF;
	uvpos = (((ctrl->width)>>1)*((pos_y)>>1)+((pos_x)>>1));
    ctrl->u[uvpos] = 117;//(T_U8) RGB2U(232,224,206);//(T_U8)(ctrl->color>>8)&0xFF;    
    ctrl->v[uvpos] = 133;//(ctrl->color>>16)&0xFF;
}
*/

//#define mm_mm "2012/12/12 10:10:10"
void camera_u8_to_unicode(char *src, T_U16 *dest)
{
    while(*src)
    {
        *dest = *src;
        dest ++;
        src ++;
    }
}

void camera_format_unicode_digits(T_U16 *buf)
{
	int i;
	char tmp[8];

	for (i = 0; i <= 9; i++)
	{
		sprintf(tmp, "%d", i);
		camera_u8_to_unicode(tmp, buf + i);
	}
}

int camera_format_osd_info(T_U16 *osd_buf, int *week_pos, time_t  osd_time)
{
    int osd_len = 0;
    Pcamera_disp_setting pcamera_info = anyka_get_camera_info();
    T_U16 year[4], month[2], day[2], hour[2], min[2], second[2], week_ch[3]= {0x661f, 0x671f, 0};
    T_U16 year_uni = 0x5e74, mon_uni = 0x6708, day_uni = 0x65e5;
    T_U16 week_day[] = {0x5929, 0x4e00, 0x4e8c, 0x4e09, 0x56db, 0x4e94, 0x516d};
    T_U16 week_day_en[7][4] =
    {   {'S', 'u', 'n', 'n'},
        {'M', 'o', 'n', ' '},
        {'T', 'u', 'e', 's'},
        {'W', 'e', 'd', ' '},
        {'T', 'h', 'u', 'r'},
        {'F', 'r', 'i', ' '},
        {'S', 'a', 't', ' '},
    };
	char tmp[50];
#if 0
	time_t ta;
	time(&ta);		
	struct tm * tt = localtime( &ta );
#endif

	struct tm * tt = localtime(&osd_time);
    sprintf(tmp, "%d", tt->tm_year+1900);
    camera_u8_to_unicode(tmp, year);
    sprintf(tmp, "%02d", tt->tm_mon+ 1);
    camera_u8_to_unicode(tmp, month);
    sprintf(tmp, "%02d", tt->tm_mday);
    camera_u8_to_unicode(tmp, day);
	sprintf(tmp, "%02d", tt->tm_hour);
    camera_u8_to_unicode(tmp, hour);
    sprintf(tmp, "%02d", tt->tm_min);
    camera_u8_to_unicode(tmp, min);
    sprintf(tmp, "%02d", tt->tm_sec);
    camera_u8_to_unicode(tmp, second);


	if (pcamera_info->date_format ||
		pcamera_info->hour_format)
	{
		/* 如果先显示数字，则加入一个空格 */
		osd_buf[osd_len] =' ';
        osd_len++;
	}

    if(pcamera_info->date_format == 1)
    {
        // YYYY-MM-DD    
        memcpy(osd_buf + osd_len, year, sizeof(year));
        osd_len += 4;
        osd_buf[osd_len] ='-';
        osd_len ++;
        memcpy(osd_buf + osd_len, month, sizeof(month));
        osd_len += 2;
        osd_buf[osd_len] ='-';
        osd_len ++;
        memcpy(osd_buf + osd_len, day, sizeof(day));
        osd_len += 2;
        osd_buf[osd_len] =' ';
        osd_len ++;
    }
    else if(pcamera_info->date_format == 2)
    {
        // MM-DD-YYYY    
        memcpy(osd_buf + osd_len, month, sizeof(month));
        osd_len += 2;
        osd_buf[osd_len] ='-';
        osd_len ++;
        memcpy(osd_buf + osd_len, day, sizeof(day));
        osd_len += 2;
        osd_buf[osd_len] ='-';
        osd_len ++;
        memcpy(osd_buf + osd_len, year, sizeof(year));
        osd_len += 4;
        osd_buf[osd_len] =' ';
        osd_len ++;
    }
    else if(pcamera_info->date_format == 3)
    {
        // YYYY年MM月DD日
        memcpy(osd_buf + osd_len, year, sizeof(year));
        osd_len += 4;
        osd_buf[osd_len] = year_uni;
        osd_len ++;
        memcpy(osd_buf + osd_len, month, sizeof(month));
        osd_len += 2;
        osd_buf[osd_len] = mon_uni;
        osd_len ++;
        memcpy(osd_buf + osd_len, day, sizeof(day));
        osd_len += 2;
        osd_buf[osd_len] =day_uni;
        osd_len ++;
        osd_buf[osd_len] =' ';
        osd_len ++;
    }
    else if(pcamera_info->date_format == 4)
    {
        // MM月DD日YYYY年           
        memcpy(osd_buf + osd_len, month, sizeof(month));
        osd_len += 2;
        osd_buf[osd_len] = mon_uni;
        osd_len ++;
		
		memcpy(osd_buf + osd_len, day, sizeof(day));
	   	osd_len += 2;
		osd_buf[osd_len] =day_uni;
	   	osd_len ++;
		
        memcpy(osd_buf + osd_len, year, sizeof(year));
        osd_len += 4;
        osd_buf[osd_len] = year_uni;
		osd_buf[osd_len] =' ';
	   	osd_len ++;
    }
    else if(pcamera_info->date_format == 5)
    {
        // DD-MM-YYYY    
        memcpy(osd_buf + osd_len, day, sizeof(day));
        osd_len += 2;
        osd_buf[osd_len] ='-';
        osd_len ++;
        memcpy(osd_buf + osd_len, month, sizeof(month));
        osd_len += 2;
        osd_buf[osd_len] ='-';
        osd_len ++;
        memcpy(osd_buf + osd_len, year, sizeof(year));
        osd_len += 4;
        osd_buf[osd_len] =' ';
        osd_len ++;
    }
    else if(pcamera_info->date_format == 6)
    {
        // DD日MM月YYYY年
        memcpy(osd_buf + osd_len, day, sizeof(day));
        osd_len += 2;
        osd_buf[osd_len] =day_uni;
        osd_len ++;
        memcpy(osd_buf + osd_len, month, sizeof(month));
        osd_len += 2;
        osd_buf[osd_len] = mon_uni;
        osd_len ++;
        memcpy(osd_buf + osd_len, year, sizeof(year));
        osd_len += 4;
        osd_buf[osd_len] = year_uni;
        osd_len ++;
        osd_buf[osd_len] = ' ';
        osd_len ++;
    }

	// Hour-Min-Sec
    //if(pcamera_info->hour_format)
    if(1) //目前若不显示此内容则后面的星期可能会重叠，后面再修改
    {
        //hour:min:second
        memcpy(osd_buf + osd_len, hour, sizeof(hour));
        osd_len += 2;
        osd_buf[osd_len] =':';
        osd_len ++;
        memcpy(osd_buf + osd_len, min, sizeof(min));
        osd_len += 2;
        osd_buf[osd_len] =':';
        osd_len ++;
        memcpy(osd_buf + osd_len, second, sizeof(second));
        osd_len += 2;
        osd_buf[osd_len] =' ';
        osd_len ++;
    }

	//get week start pos 
	*week_pos = osd_len;
    if(pcamera_info->week_format)
    {
        if(pcamera_info->week_format == 1)
        {
            //中文显示
            week_ch[2] = week_day[tt->tm_wday];
            memcpy(osd_buf + osd_len, week_ch, sizeof(week_ch));
            osd_len += 3;
        }
        else
        {
            //英文显示
            memcpy(osd_buf + osd_len, week_day_en[tt->tm_wday], 8);
            osd_len += 4;
        }
    }

    return osd_len;
}

/**
* @brief  darw time to video 
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/
/*
T_VOID  camera_show_curtime_onvideo(T_U8 * destBuf, T_S32 width, T_S32 height, T_S32 size, time_t osd_time)
{
    int x , y, week_pos, new_x, font_size;
	struct tagRECORD_VIDEO_FONT_CTRL pixelShowCtrl;    
    Pcamera_disp_setting pcamera_info = anyka_get_camera_info();
    T_U16 osd_buf[100], len;
	assert( destBuf );
    
    if(1) //(VIDEO_WIDTH_720P == width)
    {
        font_size = FONT_WIDTH_720;
    }
    else
    {
        font_size = FONT_WIDTH_VGA;
    }

    pixelShowCtrl.y = destBuf;
    pixelShowCtrl.u = pixelShowCtrl.y + size;
    pixelShowCtrl.v = pixelShowCtrl.u + (size>>2);
    pixelShowCtrl.color = 0x50;
    pixelShowCtrl.width  = (T_U32)width;


    //先显示时间信息，
    len = camera_format_osd_info(osd_buf, &week_pos, osd_time);
    x = font_size;
    if(len && pcamera_info->time_switch)
    {
        if(pcamera_info->time_pos_y > 50)
        {
            y = height - (6 + font_size);
        }
        else
        {
            y = 6;
        }
        new_x = FontDisp_DrawStr((FONT_DISP_SET_PIXEL_CB)SetFontPixelOnVideo, &pixelShowCtrl,
            x, y, osd_buf, week_pos, font_size);

		//show weekens
		if(len > week_pos) {
			if(1) //(VIDEO_WIDTH_720P == width)
			{
            	FontDisp_DrawStr((FONT_DISP_SET_PIXEL_CB)SetFontPixelOnVideo, &pixelShowCtrl,
               	 	x + (week_pos - 4) * font_size / 2 , y, osd_buf+week_pos, len - week_pos, font_size);
	        }
	        else
	        {
	            FontDisp_DrawStr((FONT_DISP_SET_PIXEL_CB)SetFontPixelOnVideo, &pixelShowCtrl,
	                x + (week_pos - 4) * font_size / 2 , y, osd_buf+week_pos, len - week_pos, font_size);
	        }
		}
    }
    
    //再显示OSD信息，
    if(pcamera_info->osd_unicode_name_len && pcamera_info->osd_switch)
    {
        if(pcamera_info->time_pos_y > 50)
        {
            y = height - (6 + font_size*2);
        }
        else
        {
            y = 6 + font_size;
        }
        FontDisp_DrawStr((FONT_DISP_SET_PIXEL_CB)SetFontPixelOnVideo, &pixelShowCtrl,
            x, y, pcamera_info->osd_unicode_name, pcamera_info->osd_unicode_name_len, font_size);
    }
}
*/

/**
* @brief  abort setup
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/

static void errno_exit(const char *s)
{
    anyka_print("%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}


/**
* @brief  get one frame data
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/

static int read_frame(void)
{
    unsigned int i;
	CLEAR(buf);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_USERPTR;
	buf.m.userptr = 0;
	if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) 
	{
			switch (errno)
			{
			case EAGAIN:
					return 0;

			case EIO:
					/* Could ignore EIO, see spec. */

					/* fall through */

			default:
					errno_exit("VIDIOC_DQBUF");
			}
	}

	for (i = 0; i < BUFF_NUM; ++i) 
	{
		   if (buf.m.userptr == (unsigned long)buffers[i].start)
		   {
				return 1;
		   }
	}				

    return 0;
}


/**
* @brief  init user buff
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/

static void init_userp(unsigned int buffer_size)
{
    struct v4l2_requestbuffers req;
	int n_buffers;
    int ion_size;
    CLEAR(req);

    req.count  = BUFF_NUM;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

#if !(STANDBY_MODE)
    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
	   anyka_print("init_userp==>REQBUFS failed!\n");
            if (EINVAL == errno) {
                    anyka_print("%s does not support user pointer i/o\n", dev_name);
                    exit(EXIT_FAILURE);
            } else {
                    errno_exit("VIDIOC_REQBUFS");
            }
    }
    anyka_print("init_userp==>REQBUFS succeedded!userptr[count=%d]\n", req.count);
#endif

    buffers = calloc(BUFF_NUM, sizeof(struct buffer));

    if (!buffers) {
            anyka_print("[%s:%d] Out of memory\n", __func__, __LINE__);
            exit(EXIT_FAILURE);
    }

	ion_size = buffer_size * BUFF_NUM;
	ion_mem = akuio_alloc_pmem(ion_size);
	if (!ion_mem) {
		anyka_print("Allocate %d bytes failed\n", ion_size);
	}

	T_U32 temp;
	T_U8 *ptemp;
	
	temp = akuio_vaddr2paddr(ion_mem) & 7;
	//编码buffer 起始地址必须8字节对齐
	ptemp = ((T_U8 *)ion_mem) + ((8-temp)&7);

    for (n_buffers = 0; n_buffers < BUFF_NUM; ++n_buffers) {
        buffers[n_buffers].length = buffer_size;
        buffers[n_buffers].start = ptemp + buffer_size * n_buffers;
        if (!buffers[n_buffers].start) {
                anyka_print("[%s:%d]Out of memory\n", __func__, __LINE__);
                exit(EXIT_FAILURE);
        }
    }
}


static int Zoom(int x, int y, int width, int heigth, int outw, int outh, int channel);


/**
* @brief  set zoom
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/

int camera_set_zoom(int z)
{
	int width = 0;
	int height = 0;
	int outwidth = 0;
	int outheight = 0;
	int stepwidth = 0;
	int stepheight = 0;
	static int zoom = 0;
//	static int zoom2 = 0;
	int x = 0, y = 0, w = 0, h = 0;
	int channel = 1;
	//channel++;
	if(zoom >3 || zoom < 0)
		return 0;
	
	if (1) //(parse.width == 1280 )
	{
		width = 1280;
		height = 720;
		outwidth = 1280;
		outheight = 720;
		stepwidth = 100;
		stepheight = 60;
	}
	if (z == 1 && zoom < 3) {
		zoom++;
		x = zoom*(stepwidth/2);
		y = zoom*(stepheight/2);
		w = width - 2*x;
		h = height - 2*y;
		anyka_print("w = %d, y = %d w =%d, h =%d \n", x, y, w, h);
		
	}
	else if(z== 0 && zoom > 0)
	{
		zoom--;
		x = zoom*(stepwidth/2);
		y = zoom*(stepheight/2);
		w = width - 2*x;
		h = height - 2*y;
	}
	anyka_print("zoom is %d channel = %d\n", zoom, channel );
	{
		Zoom(x, y, w, h, outwidth, outheight, channel );
	}
	return 0;
}


/**
* @brief  set zoom
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/

static int Zoom(int x, int y, int width, int heigth, int outw, int outh, int channel)
{
	int ret;
	AK_ISP_USER_PARAM param;
	struct isp_zoom_info *zoom;

	CLEAR(param);
	ENOUGH_MEM(struct isp_zoom_info, param.data);

	param.id = AK_ISP_USER_CID_SET_ZOOM;

	zoom = (struct isp_zoom_info *)param.data;
	zoom->channel = channel;
	zoom->cut_xpos = x;
	zoom->cut_ypos = y;
	zoom->cut_width = width;
	zoom->cut_height = heigth;
	zoom->out_width = outw;
	zoom->out_height = outh;
	ret = Isp_Module_Set_User_Params(&param);

	return ret;
}

/**
* @brief  set channel
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/

int camera_set_channel2(int width, int height)
{
	int ret;
	AK_ISP_USER_PARAM param;
	struct isp_channel2_info *p_chn;

	CLEAR(param);
	ENOUGH_MEM(struct isp_channel2_info, param.data);

	param.id = AK_ISP_USER_CID_SET_SUB_CHANNEL;

	p_chn = (struct isp_channel2_info *)param.data;
	p_chn->width = width;
	p_chn->height = height;
	ret = Isp_Module_Set_User_Params(&param);

	return ret;
}

/**
* @brief  set occ
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/

int camera_set_occ(int num, int x1, int y1, int x2, int y2)
{
	int ret;
	AK_ISP_USER_PARAM param;
	struct isp_occlusion_info *p_occ;

	CLEAR(param);
	ENOUGH_MEM(struct isp_occlusion_info, param.data);

	param.id = AK_ISP_USER_CID_SET_OCCLUSION;

	p_occ = (struct isp_occlusion_info *)param.data;
	p_occ->channel = 1;
	p_occ->number = num;
	p_occ->start_xpos = x1;
	p_occ->start_ypos = y1;
	p_occ->end_xpos = x2;
	p_occ->end_ypos = y2;
	ret = Isp_Module_Set_User_Params(&param);

	return ret;
}

int camera_set_occ_color(int color_type, int transparency, int y_component, int u_component, int v_component)
{
	int ret;
	AK_ISP_USER_PARAM param;
	struct isp_occlusion_color *p_occ_color;

	CLEAR(param);
	ENOUGH_MEM(struct isp_occlusion_color, param.data);

	param.id = AK_ISP_USER_CID_SET_OCCLUSION_COLOR;

	p_occ_color = (struct isp_occlusion_color *) param.data;
	p_occ_color->color_type = color_type;
	p_occ_color->transparency = transparency;
	p_occ_color->y_component = y_component;
	p_occ_color->u_component = u_component;
	p_occ_color->v_component = v_component;
	ret = Isp_Module_Set_User_Params(&param);

	return ret;
}

/**
* @brief  set gamma
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/
int camera_set_gamma(int cid)
{
	int ret;
	AK_ISP_USER_PARAM param;
	struct isp_gamma_info *p_gamma;

	CLEAR(param);
	ENOUGH_MEM(struct isp_gamma_info, param.data);

	param.id = AK_ISP_USER_CID_SET_GAMMA;

	p_gamma = (struct isp_gamma_info *)param.data;
	p_gamma->value = cid;
	ret = Isp_Module_Set_User_Params(&param);

	return ret;
}

/**
* @brief  set normal
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/
int camera_set_normal(void)
{
	anyka_print("Set_Normal \n");
	struct v4l2_control control;

	Isp_Module_Set_Isp_Pause();

	memset (&control, 0, sizeof (control));
	control.id = V4L2_CID_HFLIP;
	control.value = CAMERA_MIRROR_NORMAL;
	if (-1 == ioctl (fd, VIDIOC_S_CTRL, &control))
	{
		perror ("Set cid err \n");
		Isp_Module_Set_Isp_Resume();
		return -1;
	}

	Isp_Module_Set_Isp_Resume();

	return 0;
}
/**
* @brief  set fflip
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/
int camera_set_flip(void)
{
	struct v4l2_control control;
	memset (&control, 0, sizeof (control));

	Isp_Module_Set_Isp_Pause();

	control.id = V4L2_CID_HFLIP;
	control.value = CAMERA_MIRROR_FLIP;
	if (-1 == ioctl (fd, VIDIOC_S_CTRL, &control))
	{
		anyka_print ("Set cid err \n");
		Isp_Module_Set_Isp_Resume();
		return -1;
	}

	Isp_Module_Set_Isp_Resume();

	return 0;
}

/**
* @brief  set vfilp
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/
int camera_set_vflip(void)
{
	anyka_print("Set_VFlip \n");
	struct v4l2_control control;

	Isp_Module_Set_Isp_Pause();

	memset (&control, 0, sizeof (control));
	control.id = V4L2_CID_HFLIP;
	control.value = CAMERA_MIRROR_V;
	if (-1 == ioctl (fd, VIDIOC_S_CTRL, &control))
	{
		anyka_print ("Set cid err \n");
		Isp_Module_Set_Isp_Resume();
		return -1;
	}

	Isp_Module_Set_Isp_Resume();

	return 0;
}

int camera_set_mirror(void)
{
	anyka_print("Set_mirror \n");
	struct v4l2_control control;

	Isp_Module_Set_Isp_Pause();

	memset (&control, 0, sizeof (control));
	control.id = V4L2_CID_HFLIP;
	control.value = CAMERA_MIRROR_H;
	if (-1 == ioctl (fd, VIDIOC_S_CTRL, &control))
	{
		anyka_print ("Set cid err \n");
		Isp_Module_Set_Isp_Resume();
		return -1;
	}

	Isp_Module_Set_Isp_Resume();

	return 0;
}

int camera_set_saturation(int sat)
{
	int ret;
	AK_ISP_USER_PARAM param;
	struct isp_saturation_info *p_saturation;

	CLEAR(param);
	ENOUGH_MEM(struct isp_saturation_info, param.data);

	param.id = AK_ISP_USER_CID_SET_SATURATION;

	p_saturation = (struct isp_saturation_info *)param.data;
	p_saturation->value = sat;
	ret = Isp_Module_Set_User_Params(&param);

	return ret;
}

/**
* @brief  set bright
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/
int camera_set_brightness(int bright)
{
	int ret;
	AK_ISP_USER_PARAM param;
	struct isp_brightness_info *p_bright;

	CLEAR(param);
	ENOUGH_MEM(struct isp_brightness_info, param.data);

	param.id = AK_ISP_USER_CID_SET_BRIGHTNESS;

	p_bright = (struct isp_brightness_info *)param.data;
	p_bright->value = bright;
	ret = Isp_Module_Set_User_Params(&param);

	return ret;
}

int camera_set_contrast(int constrast)
{
	int ret;
	AK_ISP_USER_PARAM param;
	struct isp_contrast_info *p_contrast;

	CLEAR(param);
	ENOUGH_MEM(struct isp_contrast_info, param.data);

	param.id = AK_ISP_USER_CID_SET_CONTRAST;

	p_contrast = (struct isp_contrast_info *)param.data;
	p_contrast->value = constrast;
	ret = Isp_Module_Set_User_Params(&param);

	return ret;
}

int camera_set_sharpness(int sharp)
{
	int ret;
	AK_ISP_USER_PARAM param;
	struct isp_sharp_info *p_sharp;

	CLEAR(param);
	ENOUGH_MEM(struct isp_sharp_info, param.data);

	param.id = AK_ISP_USER_CID_SET_SHARPNESS;

	p_sharp = (struct isp_sharp_info *)param.data;
	p_sharp->value = sharp;
	ret = Isp_Module_Set_User_Params(&param);

	return ret;
}

int camera_get_sensor_id(void)
{
	struct v4l2_control control;
	memset (&control, 0, sizeof (control));
	control.id = V4L2_CID_SENSOR_ID;
	control.value = 0;
	if (-1 == ioctl (fd, VIDIOC_G_CTRL, &control))
	{
		anyka_print ("Get sensor id err \n");
		return -1;
	}

	return control.value;
}

/**
* @brief  set power frequency
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/
int camera_set_power_frequency(int freq)
{
	int ret;
	AK_ISP_USER_PARAM param;
	struct isp_power_line_freq_info *p_freq;

	CLEAR(param);
	ENOUGH_MEM(struct isp_power_line_freq_info, param.data);

	param.id = AK_ISP_USER_CID_SET_POWER_LINE_FREQUENCY;

	p_freq = (struct isp_power_line_freq_info *)param.data;
	p_freq->value = freq;
	ret = Isp_Module_Set_User_Params(&param);

	return ret;
}

struct osd_para_attr {
	unsigned char *p_main_chan_osd_dma_buffer;
	unsigned char *p_sub_chan_osd_dma_buffer;
	unsigned char *p_main_chan_osd_buffer;
	unsigned char *p_sub_chan_osd_buffer;
	unsigned short *p_osd_unicode_codes;
	int *p_main_chan_osd_fonts_width;
	int *p_sub_chan_osd_fonts_width;
	unsigned int *p_color_tables;
	int osd_unicode_codes_len[OSD_LINES];
	int xpos_main;
	int xpos_sub;
	int ypos_main;
	int ypos_sub;
	int osd_on_right_side;
	int color_table_index[OSD_LINES];
	int color_table_index_ground[OSD_LINES];
	int alpha;
	int font_size_main;
	int font_size_sub;
};

static unsigned char main_chan_osd_dma_buffer[MAIN_CHAN_OSD_BUFFER_SIZE];
static unsigned char sub_chan_osd_dma_buffer[SUB_CHAN_OSD_BUFFER_SIZE];
static unsigned char main_chan_osd_buffer[MAIN_CHAN_OSD_BUFFER_SIZE];
static unsigned char sub_chan_osd_buffer[SUB_CHAN_OSD_BUFFER_SIZE];
static unsigned short osd_unicode_codes[OSD_LINES][OSD_UNICODE_CODES_NUM];
static int main_chan_osd_fonts_width[OSD_LINES][OSD_UNICODE_CODES_NUM];
static int sub_chan_osd_fonts_width[OSD_LINES][OSD_UNICODE_CODES_NUM];
static T_U16 unicode_digits[10];

static struct osd_para_attr osd;

void camera_osd_para_attr_init(void)
{
	struct osd_para_attr *p_osd = &osd;

	memset(p_osd, 0, sizeof(struct osd_para_attr));

	p_osd->p_main_chan_osd_dma_buffer	= main_chan_osd_dma_buffer;
	p_osd->p_sub_chan_osd_dma_buffer	= sub_chan_osd_dma_buffer;
	p_osd->p_main_chan_osd_buffer		= main_chan_osd_buffer;
	p_osd->p_sub_chan_osd_buffer		= sub_chan_osd_buffer;
	p_osd->p_osd_unicode_codes			= &osd_unicode_codes[0][0];
	p_osd->p_main_chan_osd_fonts_width	= &main_chan_osd_fonts_width[0][0];
	p_osd->p_sub_chan_osd_fonts_width	= &sub_chan_osd_fonts_width[0][0];
	p_osd->p_color_tables				= def_color_tables;

	p_osd->font_size_main = FONT_WIDTH_720;
	p_osd->font_size_sub = FONT_WIDTH_720; //FONT_WIDTH_VGA;

	p_osd->color_table_index[0]			= DEF_1ST_FONTS_COLOR_TABLE_INDEX;
	p_osd->color_table_index[1]			= DEF_2ND_FONTS_COLOR_TABLE_INDEX;
	p_osd->color_table_index_ground[0]	= DEF_1ST_GROUND_COLOR_TABLE_INDEX;
	p_osd->color_table_index_ground[1]	= DEF_2ND_GROUND_COLOR_TABLE_INDEX;
	p_osd->alpha						= DEF_ALPHA;

	Pcamera_disp_setting pcamera_info = anyka_get_camera_info();
	int pos_pattern = pcamera_info->osd_position;
	switch (pos_pattern)
	{
	case 1:	//左下
		p_osd->xpos_main	= MAIN_OSD_RELATIVE_OFFSET_X;
		p_osd->ypos_main	= VIDEO_HEIGHT_720P - MAIN_OSD_RELATIVE_OFFSET_Y - (p_osd->font_size_main * OSD_LINES);
		p_osd->xpos_sub		= SUB_OSD_RELATIVE_OFFSET_X;
		p_osd->ypos_sub		= VIDEO_HEIGHT_VGA - SUB_OSD_RELATIVE_OFFSET_Y - (p_osd->font_size_sub * OSD_LINES);
		p_osd->osd_on_right_side	= 0;
		break;
	case 2:	//左上
		p_osd->xpos_main	= MAIN_OSD_RELATIVE_OFFSET_X;
		p_osd->ypos_main	= MAIN_OSD_RELATIVE_OFFSET_Y;
		p_osd->xpos_sub		= SUB_OSD_RELATIVE_OFFSET_X;
		p_osd->ypos_sub		= SUB_OSD_RELATIVE_OFFSET_Y;
		p_osd->osd_on_right_side	= 0;
		break;
	case 3:	//右上
		p_osd->xpos_main	= VIDEO_WIDTH_720P - MAIN_OSD_RELATIVE_OFFSET_X;
		p_osd->ypos_main	= MAIN_OSD_RELATIVE_OFFSET_Y;
		p_osd->xpos_sub		= VIDEO_WIDTH_VGA - SUB_OSD_RELATIVE_OFFSET_X;
		p_osd->ypos_sub		= SUB_OSD_RELATIVE_OFFSET_Y;
		p_osd->osd_on_right_side	= 1;
		break;
	case 4:	//右下
		p_osd->xpos_main	= VIDEO_WIDTH_720P - MAIN_OSD_RELATIVE_OFFSET_X;
		p_osd->ypos_main	= VIDEO_HEIGHT_720P - MAIN_OSD_RELATIVE_OFFSET_Y - (p_osd->font_size_main * OSD_LINES);
		p_osd->xpos_sub		= VIDEO_WIDTH_VGA - SUB_OSD_RELATIVE_OFFSET_X;
		p_osd->ypos_sub		= VIDEO_HEIGHT_VGA - SUB_OSD_RELATIVE_OFFSET_Y - (p_osd->font_size_sub * OSD_LINES);
		p_osd->osd_on_right_side	= 1;
		break;
	default:
		break;
	}

	camera_format_unicode_digits(unicode_digits);
}

/**
* @brief  set hw osd color tables
* 
* @author ye_guohong
* @date 2015-11-13
* @param[void] 
* @return int
* @retval 
*/
int camera_set_osd_color_table_attr(void)
{
	int ret;
	unsigned int *p_color_tables;
	AK_ISP_USER_PARAM param;
  	struct isp_osd_color_table_attr *p_table;
	struct osd_para_attr *p_osd = &osd;

	p_color_tables = p_osd->p_color_tables;

	CLEAR(param);
	ENOUGH_MEM(struct isp_osd_color_table_attr, param.data);

	param.id = AK_ISP_USER_CID_SET_OSD_COLOR_TABLE_ATTR;

	p_table = (struct isp_osd_color_table_attr *)param.data;
	memcpy(p_table->color_table, p_color_tables, 16 * sizeof(unsigned int));
	ret = Isp_Module_Set_User_Params(&param);

	return ret;
}

static int is_unicode_digit(T_U16 ch)
{
	int i;

	for (i = 0; i < 10; i++)
		if (ch == unicode_digits[i])
			return 1;
	return 0;
}

/**
* @brief  画画布
* 
* @author ye_guohong
* @date 2015-11-13
* @param[int] 	-1: 没更改
				>=0: 成功在画布画的字符个数。
* @return int
* @retval 
*/
static int camera_osd_draw_canvas(int line, T_U16 *disp_string, int len)
{
	struct osd_para_attr *p_osd = &osd;
	unsigned char *p_main_chan_osd_buffer;
	unsigned char *p_sub_chan_osd_buffer;
	unsigned short *p_osd_unicode_codes;
	int *p_main_chan_osd_fonts_width;
	int *p_sub_chan_osd_fonts_width;
	int *p_osd_unicode_codes_len;
	int *p_color_table_index		= p_osd->color_table_index + line;
	int *p_color_table_index_ground	= p_osd->color_table_index_ground + line;
	int *p_font_size_main	= &p_osd->font_size_main; 
	int *p_font_size_sub	= &p_osd->font_size_sub;

	int font_size_main	= *p_font_size_main;
	int font_size_sub	= *p_font_size_sub;

	int i;
	int flag_redraw_remain = 0;
	int xpos_m = 0, xpos_s = 0;

	if(line >= OSD_LINES)
		return -1;

	p_main_chan_osd_buffer		= p_osd->p_main_chan_osd_buffer + line * MAIN_CHAN_OSD_PER_LINE_BYTES;
	p_sub_chan_osd_buffer		= p_osd->p_sub_chan_osd_buffer + line * SUB_CHAN_OSD_PER_LINE_BYTES;
	p_osd_unicode_codes			= p_osd->p_osd_unicode_codes + line * OSD_UNICODE_CODES_NUM;
	p_main_chan_osd_fonts_width	= p_osd->p_main_chan_osd_fonts_width + line * OSD_UNICODE_CODES_NUM;
	p_sub_chan_osd_fonts_width	= p_osd->p_sub_chan_osd_fonts_width + line * OSD_UNICODE_CODES_NUM;
	p_osd_unicode_codes_len		= p_osd->osd_unicode_codes_len + line;

	if (len == *p_osd_unicode_codes_len)
	{
		for (i = 0; i < len; i++)
			if (disp_string[i] != p_osd_unicode_codes[i])
				break;

		if (i == len)
			return -1;
		
		//anyka_print("line:%d code(s) change,cur_len:%d,pre_len:%d, change index:%d, change code: 0x%x-->0x%x\n",line,len,*p_osd_unicode_codes_len, i,disp_string[i], p_osd_unicode_codes[i]);
	}

	for (i = 0; i < len; i++)
	{
		if (disp_string[i] != p_osd_unicode_codes[i] || 
			len != *p_osd_unicode_codes_len ||
			flag_redraw_remain)
		{
			if (!(is_unicode_digit(disp_string[i]) &&
				is_unicode_digit(p_osd_unicode_codes[i])))
			{
				//anyka_print("line:%d no digit i:%d, cur:0x%x, pre:0x%x\n",line,i,disp_string[i],p_osd_unicode_codes[i]);
				flag_redraw_remain = 1;
			}

			int width_m;
			int width_s;


			struct tagRECORD_VIDEO_FONT_HW_OSD_CTRL hw_osd_ctrl;
			hw_osd_ctrl.p_context = p_main_chan_osd_buffer;
			hw_osd_ctrl.x_offset_pixels = xpos_m;
			hw_osd_ctrl.font_size = font_size_main;
			hw_osd_ctrl.osd_row_bytes = MAIN_CHAN_OSD_PER_LINE_BYTES / hw_osd_ctrl.font_size;
			hw_osd_ctrl.color_table_index = *p_color_table_index;
			hw_osd_ctrl.color_table_index_ground = *p_color_table_index_ground;
			
			width_m = FontDisp_SetFontMatrix((FONT_DISP_SET_PIXEL_TO_HW_OSD_CB)SetFontMatrix, 
				&hw_osd_ctrl, disp_string[i], hw_osd_ctrl.font_size);
			if (!width_m)
				break;

			hw_osd_ctrl.p_context = p_sub_chan_osd_buffer;
			hw_osd_ctrl.x_offset_pixels = xpos_s;
			hw_osd_ctrl.font_size = font_size_sub;
			hw_osd_ctrl.osd_row_bytes = SUB_CHAN_OSD_PER_LINE_BYTES / hw_osd_ctrl.font_size;
			hw_osd_ctrl.color_table_index = *p_color_table_index;
			hw_osd_ctrl.color_table_index_ground = *p_color_table_index_ground;
			
			width_s = FontDisp_SetFontMatrix((FONT_DISP_SET_PIXEL_TO_HW_OSD_CB)SetFontMatrix, 
				&hw_osd_ctrl, disp_string[i], hw_osd_ctrl.font_size);
			if (!width_s)
				break;

			//anyka_print("width_m:%d,width_s:%d\n",width_m,width_s);
			//p_osd_unicode_codes_line2[i] = osd_codes[i];
			p_main_chan_osd_fonts_width[i] = width_m;
			p_sub_chan_osd_fonts_width[i] = width_s;
		}

		xpos_m += p_main_chan_osd_fonts_width[i];
		xpos_s += p_sub_chan_osd_fonts_width[i];
	}
	//*p_osd_unicode_codes_len_line2 = i;
	//anyka_print("xpos_m:%d,xpos_s:%d\n",xpos_m,xpos_s);

	if (i != len)
		anyka_print("Small buffer to disp osd LINE:%d, only disp %d characters of total %d\n", line,i,len);

	memcpy(p_osd_unicode_codes, disp_string, len * sizeof(disp_string[0]));
	*p_osd_unicode_codes_len = len;

	T_U8 *line_buffer = p_main_chan_osd_buffer;
	int per_pixel_row_bytes = MAIN_CHAN_OSD_PER_LINE_BYTES / font_size_main;
	int offset_bytes = xpos_m / 2;
	int set_bytes = per_pixel_row_bytes - offset_bytes;
	for (i = 0; i < font_size_main; i++)
		memset(line_buffer + offset_bytes + i * per_pixel_row_bytes, 0, set_bytes);

	line_buffer = p_sub_chan_osd_buffer;
	per_pixel_row_bytes = SUB_CHAN_OSD_PER_LINE_BYTES / font_size_sub;
	offset_bytes = xpos_s / 2;
	set_bytes = per_pixel_row_bytes - offset_bytes;
	for (i = 0; i < font_size_sub; i++)
		memset(line_buffer + offset_bytes + i * per_pixel_row_bytes, 0, set_bytes);

	unsigned char *p_main_chan_osd_dma_buffer		= p_osd->p_main_chan_osd_dma_buffer + line * MAIN_CHAN_OSD_PER_LINE_BYTES;
	unsigned char *p_sub_chan_osd_dma_buffer		= p_osd->p_sub_chan_osd_dma_buffer + line * SUB_CHAN_OSD_PER_LINE_BYTES;
	int osd_on_right_side = p_osd->osd_on_right_side;
	int main_Xstart, sub_Xstart;

	memset(p_main_chan_osd_dma_buffer, 0, MAIN_CHAN_OSD_PER_LINE_BYTES);
	memset(p_sub_chan_osd_dma_buffer, 0, SUB_CHAN_OSD_PER_LINE_BYTES);
	if (osd_on_right_side)
	{
		/* 如果osd在右边显示，那么将osd搬移到osd buffer的右侧 */
		main_Xstart = MAIN_CHAN_OSD_PER_LINE_BYTES / font_size_main * 2 - xpos_m;
		sub_Xstart = SUB_CHAN_OSD_PER_LINE_BYTES / font_size_sub * 2 - xpos_s;
	}
	else
	{
		main_Xstart = 0;
		sub_Xstart = 0;
	}

	per_pixel_row_bytes = MAIN_CHAN_OSD_PER_LINE_BYTES / font_size_main;
	offset_bytes = xpos_m / 2;
	for (i = 0; i < font_size_main; i++)
		memcpy(p_main_chan_osd_dma_buffer + main_Xstart / 2 + i * per_pixel_row_bytes,
				p_main_chan_osd_buffer + i * per_pixel_row_bytes, offset_bytes);

	per_pixel_row_bytes = SUB_CHAN_OSD_PER_LINE_BYTES / font_size_sub;
	offset_bytes = xpos_s / 2;
	for (i = 0; i < font_size_sub; i++)
		memcpy(p_sub_chan_osd_dma_buffer + sub_Xstart / 2 + i * per_pixel_row_bytes,
				p_sub_chan_osd_buffer + i * per_pixel_row_bytes, offset_bytes);

	return len;
}

static int camera_osd_draw(void)
{
	struct osd_para_attr *p_osd = &osd;
	unsigned char *p_main_chan_osd_dma_buffer = p_osd->p_main_chan_osd_dma_buffer;
	unsigned char *p_sub_chan_osd_dma_buffer = p_osd->p_sub_chan_osd_dma_buffer;

	int alpha = p_osd->alpha;
	int font_size_main = p_osd->font_size_main;
	int font_size_sub = p_osd->font_size_sub;
	int osd_on_right_side = p_osd->osd_on_right_side;

	int xpos_main = p_osd->xpos_main;
	int xpos_sub = p_osd->xpos_sub;
	int ypos_main = p_osd->ypos_main;
	int ypos_sub = p_osd->ypos_sub;

	if (osd_on_right_side)
	{
		if (xpos_main > MAIN_CHAN_OSD_PER_LINE_BYTES / font_size_main * 2)
		{
			xpos_main -= MAIN_CHAN_OSD_PER_LINE_BYTES / font_size_main * 2;
		}
		else
		{
			xpos_main = 0;
			anyka_print("xpos_main too small, or main channel buffer too long\n");
		}

		if (xpos_sub > SUB_CHAN_OSD_PER_LINE_BYTES / font_size_sub * 2)
		{
			xpos_sub -= SUB_CHAN_OSD_PER_LINE_BYTES / font_size_sub * 2;
		}
		else
		{
			xpos_sub = 0;
			anyka_print("xpos_sub too small, or sub channel buffer too long\n");
		}
	}
	
	int ret;
	AK_ISP_USER_PARAM param;
  	struct isp_osd_context_attr *p_osd_context_attr;

	CLEAR(param);
	ENOUGH_MEM(struct isp_osd_context_attr, param.data);

	p_osd_context_attr = (struct isp_osd_context_attr *)param.data;
	p_osd_context_attr->osd_context_addr	= p_main_chan_osd_dma_buffer;
	p_osd_context_attr->osd_width		= MAIN_CHAN_OSD_PER_LINE_BYTES / font_size_main * 2;
	p_osd_context_attr->osd_height		= font_size_main * OSD_LINES;
	p_osd_context_attr->start_xpos		= xpos_main;
	p_osd_context_attr->start_ypos		= ypos_main;
	p_osd_context_attr->alpha			= alpha;
	p_osd_context_attr->enable			= 1;

	param.id = AK_ISP_USER_CID_SET_MAIN_CHANNEL_OSD_CONTEXT_ATTR;
	ret = Isp_Module_Set_User_Params(&param);

	p_osd_context_attr->osd_context_addr	= p_sub_chan_osd_dma_buffer;
	p_osd_context_attr->osd_width		= SUB_CHAN_OSD_PER_LINE_BYTES / font_size_sub * 2;
	p_osd_context_attr->osd_height		= font_size_sub * OSD_LINES;
	p_osd_context_attr->start_xpos		= xpos_sub;
	p_osd_context_attr->start_ypos		= ypos_sub;
	p_osd_context_attr->alpha			= alpha;
	p_osd_context_attr->enable			= 1;
	param.id = AK_ISP_USER_CID_SET_SUB_CHANNEL_OSD_CONTEXT_ATTR;
	ret = Isp_Module_Set_User_Params(&param);

	return ret;
}

/**
* @brief  set main && sub channels hw osd
* 
* @author ye_guohong
* @date 2015-11-13
* @param[void] 
* @return int
* @retval 
*/
int camera_set_osd_context_attr(time_t osd_time)
{
	int canvas_change = 0;
	int len;
	T_U16 *p_codes;
	Pcamera_disp_setting pcamera_info = anyka_get_camera_info();

	len = pcamera_info->osd_unicode_name_len;
	p_codes = pcamera_info->osd_unicode_name;
	if (pcamera_info->osd_switch)
	{
		/* 加入一个空格对齐第二行 */
		T_U16 *disp_string = malloc((len + 1) * sizeof(T_U16));
		static char line0_osd_too_long = 0;
		
		if (!disp_string)
			return -1;

		disp_string[0] = ' ';
		memcpy(disp_string + 1, p_codes, len * sizeof(disp_string[0]));
		len++;

		if (len > OSD_UNICODE_CODES_NUM)
		{
			if (!line0_osd_too_long)
				anyka_print("OSD LINE 0 too long, len:%d\n", len);
			line0_osd_too_long = 1;
			len = OSD_UNICODE_CODES_NUM;
		}
		else
		{
			line0_osd_too_long = 0;
		}
		
		if (camera_osd_draw_canvas(0, disp_string, len) >= 0)
			canvas_change |= 1;

		free(disp_string);
	}

	int week_pos;
	T_U16 osd_codes[OSD_UNICODE_CODES_NUM];
	p_codes = osd_codes;
	len = camera_format_osd_info(osd_codes, &week_pos, osd_time);
	if (len && pcamera_info->time_switch)
	{
		if (camera_osd_draw_canvas(1, p_codes, len) >= 0)
			canvas_change |= 1 << 1;
	}

	if (canvas_change)
		return camera_osd_draw();

	return 0;
}

int camera_set_osd_disable(void)
{
	int ret;
	AK_ISP_USER_PARAM param;
	
	CLEAR(param);
	ENOUGH_MEM(struct isp_osd_context_attr, param.data);
	
	param.id = AK_ISP_USER_CID_SET_MAIN_CHANNEL_OSD_CONTEXT_ATTR;
	ret = Isp_Module_Set_User_Params(&param);

	param.id = AK_ISP_USER_CID_SET_SUB_CHANNEL_OSD_CONTEXT_ATTR;
	ret |= Isp_Module_Set_User_Params(&param);

	return ret;
}

int camera_set_fps(int fps)
{
	int ret;
	struct v4l2_control control;

	Isp_Module_Set_Isp_Pause();

	memset (&control, 0, sizeof (control));
	control.id = V4L2_CID_SENSOR_FPS;
	control.value = fps;
	ret = xioctl (fd, VIDIOC_S_CTRL, &control);
	if (ret == -1)
	{
		perror ("Set fps err \n");
	}
	Isp_Module_Set_Isp_Resume();
	anyka_print("%s==>fps: %d\n", __func__, fps);

	return ret;
}

int camera_get_fps(void)
{
#ifdef C39E
	struct v4l2_control control;
	memset (&control, 0, sizeof (control));
	control.id = V4L2_CID_SENSOR_FPS;
	control.value = 0;
	if (-1 == xioctl (fd, VIDIOC_G_CTRL, &control))
	{   
		perror ("Get fps err \n");
	}   

	return control.value;
#else
	return 25;
#endif
}

int camera_get_height(void)
{
#ifdef C39E
	struct v4l2_control control;
	memset (&control, 0, sizeof (control));
	control.id = V4L2_CID_SENSOR_RESOLUTION;
	control.value = 0;
	if (-1 == xioctl (fd, VIDIOC_G_CTRL, &control))
	{
		perror ("Get resolution err \n");
		control.value = 0x5002D0;
	}

	return control.value & 0xfff;
#else
	return 720;
#endif
}



/**
* @brief  free osd
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/

static int cur_camera_frame, max_camera_frame = 25, camera_lost_frame = BUFF_NUM + 1;
int camera_get_max_frame(void)
{
    return cur_camera_frame;
}

int isp_open(void)
{
	return Isp_Dev_Open();
}

int isp_close(void)
{
    if (ion_isp_mem)
    {
        akuio_free_pmem(ion_isp_mem);
        ion_isp_mem = NULL;
    }
	return Isp_Dev_Close();
}

int isp_init_param(int width, int height)
{
#ifdef C39E
	T_U8 *cfgbuf = NULL;
	T_U32 cfgsize = 0;

	cfgbuf = (T_U8*)malloc(ISP_CFG_MAX_SIZE);

	ISP_Conf_FileLoad(AK_TRUE, cfgbuf, &cfgsize);
	printf("cfgsize = %lu\n", cfgsize);

    ion_isp_mem = akuio_alloc_pmem(width*height*3/2*2);//扩大一倍是3D降噪需要
    
	int ret = Isp_Module_Init(cfgbuf, cfgsize, akuio_vaddr2paddr(ion_isp_mem), width, height);
	if (NULL != cfgbuf)
	{
		free(cfgbuf);
	}
	return ret;
#else
	return 0;
#endif
}

static int auto_awb_step = 100;
int load_def_isp_auto_awb_step(void)
{
	T_AWB_ATTR  awb;
	if (Isp_Module_Get_awb_attr(&awb)) {
		anyka_print("%s get awb_attr failed\n", __func__);
		return -1;
	}

	auto_awb_step = awb.auto_wb_step;
	anyka_print("%s get awb_attr ok, auto_awb_step:%d\n", __func__, auto_awb_step);
	awb.auto_wb_step = 10;
	return Isp_Module_Set_awb_attr(&awb);
}

int reload_config_isp_auto_awb_step(void)
{
	T_AWB_ATTR  awb;
	if (Isp_Module_Get_awb_attr(&awb)) {
		anyka_print("%s get awb_attr failed\n", __func__);
		return -1;
	}

	anyka_print("%s get awb_attr ok, cur auto_awb_step:%d, set to step:%d\n", __func__, awb.auto_wb_step, auto_awb_step);
	awb.auto_wb_step = auto_awb_step;
	return Isp_Module_Set_awb_attr(&awb);
}

int camera_open(void)
{
    Pcamera_disp_setting pcamera_info = anyka_get_camera_info();

    struct stat st;
	
	//open isp char
	if (isp_open()) {
		anyka_print("[%s:%d] isp open failed.\n", __func__, __LINE__);
		exit(EXIT_FAILURE);
	}
		
    if (-1 == stat(dev_name, &st)) {
		isp_close();
        anyka_print("Cannot identify '%s': %d, %s\n",
                 dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!S_ISCHR(st.st_mode)) {
		isp_close();
        anyka_print("%s is no device\n", dev_name);
        exit(EXIT_FAILURE);
    }

    //open video
    fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
    if (-1 == fd) {
		isp_close();
        anyka_print("Cannot open '%s': %d, %s\n",
                 dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

	if(fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
	{
		 anyka_print("[%s:%d] error:%s\n", __func__, __LINE__, strerror(errno));
	}

    //init isp param
	int ret = isp_init_param(pcamera_info->width, pcamera_info->height);
	if(ret) {
    	close(fd);
		isp_close();
		anyka_print("init isp failed\n");
		return -1;
	} else 
		anyka_print("init isp success\n");

	/* reset auto awb step */
	load_def_isp_auto_awb_step();

	struct v4l2_cropcap cropcap;
	
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        if(pcamera_info->height > cropcap.bounds.height)
		pcamera_info->height = cropcap.bounds.height;
        anyka_print("camera height: %d, %d\n\n", pcamera_info->height, cropcap.bounds.height);
	}
	
	return 0;
}


/**
* @brief open camera to get picture
* @author dengzhou 
* @date 2013-04-25
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
int camera_ioctl(unsigned long width, unsigned long height)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	unsigned int min;

	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			anyka_print("%s is no V4L2 device\n",
					 dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}
    cur_camera_frame = max_camera_frame = camera_get_fps();
    anyka_print("[camera:]max frames:%d\n", max_camera_frame);
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		anyka_print("%s is no video capture device\n",
				 dev_name);
		exit(EXIT_FAILURE);
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		anyka_print("%s does not support streaming i/o\n",
				 dev_name);
		exit(EXIT_FAILURE);
	}
	/* Select video input, video standard and tune here. */

	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
			crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			crop.c = cropcap.defrect; /* reset to default */

			if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
				switch (errno) {
				case EINVAL:
						/* Cropping not supported. */
						break;
				default:
						/* Errors ignored. */
						break;
				}
			} else {
				anyka_print("init_device==>S_CROP succeedded!reset to defrect[%d, %d]\n", 
					crop.c.width, crop.c.height);
			}
			
	} else {
			/* Errors ignored. */
	}

	CLEAR(fmt);

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (force_format) {
		int m_width = width;
		int m_height = height;
		if( 320 == width )
		{
			m_width = 640;
			m_height = 320;//480;
		}
		fmt.fmt.pix.width       = m_width;
		fmt.fmt.pix.height      = m_height;
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
		fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

		if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
			errno_exit("VIDIOC_S_FMT");
		else
			anyka_print("init_device==>S_FMT succeedded!\n");
			/* Note VIDIOC_S_FMT may change width and height. */
	} else {
		/* Preserve original settings as set by v4l2-ctl for example */
		if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
			errno_exit("VIDIOC_G_FMT");
	}
	
	camera_set_channel2(VIDEO_WIDTH_VGA, VIDEO_HEIGHT_VGA);

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	
	fmt.fmt.pix.sizeimage += (VIDEO_WIDTH_VGA*VIDEO_HEIGHT_VGA*3/2);
	//fmt.fmt.pix.sizeimage = (parse.width*parse.height*3/2)+(parse.width2*parse.height2*3/2);
	init_userp(fmt.fmt.pix.sizeimage);

#if !(STANDBY_MODE)
    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < BUFF_NUM; ++i) {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;
        buf.index = i;
        buf.m.userptr = (unsigned long)buffers[i].start;
        buf.length = buffers[i].length;

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            errno_exit("VIDIOC_QBUF");
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
        errno_exit("VIDIOC_STREAMON");
	else		
        anyka_print("start_capturing==>STREAMON succeedded!\n");
#endif

	if (camera_set_occ_color(0, 0, RGB2Y(0, 0, 0), RGB2U(0, 0, 0), RGB2V(0, 0, 0)))
		errno_exit("set occlusion color err\n");
	else
		anyka_print("set occlusion color succeedded!\n");

	camera_osd_para_attr_init();
	camera_set_osd_color_table_attr();
	fps_conf_info_init();

	return 1;
}

/**
* @brief get one frame from camera
* @author dengzhou 
* @date 2013-04-25
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
void * camera_getframe(void)
{
	//T_VIDEO_OSD_REC_ICON mVideoStamp;
    struct v4l2_buffer *cur_frame;

	while(1)
	{
	    fd_set fds;
	    struct timeval tv;
	    int r;

	    FD_ZERO(&fds);
	    FD_SET(fd, &fds);

	    /* Timeout. */
	    tv.tv_sec = 2;
	    tv.tv_usec = 0;

	    r = select(fd + 1, &fds, NULL, NULL, &tv);

	    if (-1 == r) {
		    anyka_print("[%s:%d]select err\n", __func__, __LINE__);
	        return NULL;
	    }

	    if (0 == r) {
	        anyka_print("[%s:%d]select timeout\n", __func__, __LINE__);
	        return NULL;
	    }

	    if (read_frame())
		{
            cur_frame = (struct v4l2_buffer *)malloc(sizeof(struct v4l2_buffer));
            memcpy(cur_frame, &buf, sizeof(struct v4l2_buffer));
			//*pbuf = (void*)buf.m.userptr;
			//*size = buf.length;//g_camera.width*g_camera.height*3/2;
			//*timeStamp = buf.timestamp.tv_sec * 1000ULL + buf.timestamp.tv_usec / 1000ULL;
	        break;
		}
	    /* EAGAIN - continue select loop. */
	}
    if(camera_lost_frame>0)
    {
		time_t osd_time;
		time(&osd_time);
		camera_set_osd_context_attr(osd_time);

        camera_lost_frame --;
        if(camera_lost_frame)
        {
        	camera_usebufok(cur_frame);
            return NULL;
        }
    }
    return (void *)cur_frame;
}

void camera_get_camera_info(void *camera_buf, void **pbuf, long *size, unsigned long *timeStamp)
{
    struct v4l2_buffer *pframe = (struct v4l2_buffer *)camera_buf;

    *pbuf = (void*)pframe->m.userptr;
    *size = pframe->length;
    *timeStamp = pframe->timestamp.tv_sec * 1000ULL + pframe->timestamp.tv_usec / 1000ULL;
}

/**
* @brief call this when camera buffer unused
* @author dengzhou 
* @date 2013-04-25
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
int camera_usebufok(void *pbuf)
{
	if (-1 == xioctl(fd, VIDIOC_QBUF, pbuf))
			errno_exit("VIDIOC_QBUF");
    free(pbuf);
	return 1;
}
/**
* @brief off camera
* @author aijun 
* @date 2014-07-29
* @param[in] 
* @return none
* @retval none
*/
void camera_off(void)
{
#if STANDBY_MODE
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
		anyka_print("VIDIOC_STREAMOFF");
#endif
}
/**
* @brief on camera
* @author aijun 
* @date 2014-07-29
* @param[in] 
* @return none
* @retval none
*/
void camera_on(void)
{
#if STANDBY_MODE
	unsigned int i;
    struct v4l2_requestbuffers req;
	enum v4l2_buf_type type;
    CLEAR(req);

    req.count  = BUFF_NUM;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
            if (EINVAL == errno) {
                    anyka_print("%s does not support user pointer i/o\n", dev_name);
                    exit(EXIT_FAILURE);
            } else {
                    errno_exit("VIDIOC_REQBUFS");
            }
    }

	for (i = 0; i < BUFF_NUM; ++i) {
		struct v4l2_buffer buf;

		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.index = i;
		buf.m.userptr = (unsigned long)buffers[i].start;
		buf.length = buffers[i].length;

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
		errno_exit("VIDIOC_STREAMON");
	else		
		anyka_print("start_capturing==>STREAMON succeedded!\n");
#endif
    camera_lost_frame = BUFF_NUM + 1;
}
/**
* @brief close camera
* @author dengzhou 
* @date 2013-04-25
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
int camera_close(void)
{
#if !(STANDBY_MODE)
    enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
		anyka_print("VIDIOC_STREAMOFF");
#endif
	
    if (ion_mem) {
		akuio_free_pmem(ion_mem);
		ion_mem = NULL;
	}

	free(buffers);

    close(fd);
    fd = -1;

	isp_close();

	return 1;
}

int camera_get_ch1_height(void)
{
    static int ch1_height = 0;

    if (ch1_height)
        return ch1_height;

    ch1_height = camera_get_height();
    anyka_print("ch1_height:%d\n", ch1_height);
	
#ifdef CONFIG_TENCENT_SUPPORT
	/* 腾讯QQ会将640*480和1280*960切小显示 */
	if (ch1_height > 720)
		ch1_height = 720;
	anyka_print("actual ch1_height:%d\n", ch1_height);
#endif

    return ch1_height;
}


int camera_get_ch2_height(void)
{
	return camera_get_ch1_height() / 2;
}

///////////////////////////////////////////////////////////////////////////


#define IR_FEED_DAYTIME_VALUE	1
#define STRING_LEN	128
#define IR_FEED_FILE_NAME "/sys/user-gpio/gpio-rf_feed"
#define IRCUT_A_FILE_NAME "/sys/user-gpio/gpio-ircut_a"
#define IRCUT_B_FILE_NAME "/sys/user-gpio/gpio-ircut_b"

enum ircut_mode{
	IRCUT_MODE_1LINE,
	IRCUT_MODE_2LINE,
};

static int ircut_pthread_running = 0;

/**
* @brief 获取光敏输入电平
* @author ygh
* @date 2015-10-25
* @param[in] 
* @return int
* @retval < 0失败，否则电平值 
*/
static int camera_get_ir_feed(void)
{
	char cmd[STRING_LEN];
	char result[STRING_LEN];

	memset(result, '\0', STRING_LEN);

    if (access("/tmp/ircut_test", F_OK) == 0) //create a test file to switch ircut by manually
	    sprintf(cmd, "cat %s", "/tmp/ircut_test");
    else
	    sprintf(cmd, "cat %s", IR_FEED_FILE_NAME);
    
	do_syscmd(cmd, result);

	//anyka_print("%s get: %s\n", __func__, result);

	if (!strlen(result))
		return -1;

	return atoi(result);
}

static int camera_set_ircut(int value, char *name)
{
	char cmd[STRING_LEN];
	char result[STRING_LEN];

	memset(result, '\0', STRING_LEN);
	sprintf(cmd, "echo %d > %s", value, name);
	do_syscmd(cmd, result);

	return 0;
}


/**
* @brief 设置ircut a输出电平
* @author ygh
* @date 2015-10-25
* @param[in] 
*			value	输出电平
* @return int
* @retval 0成功，否则失败
*/
static int camera_set_ircut_a(int value)
{
	char cmd[STRING_LEN];
	char result[STRING_LEN];

	memset(result, '\0', STRING_LEN);
	sprintf(cmd, "echo %d > %s", value, IRCUT_A_FILE_NAME);
	do_syscmd(cmd, result);

	return 0;
}

/**
* @brief 设置ircut b输出电平
* @author ygh
* @date 2015-10-25
* @param[in] 
*			value	输出电平
* @return int
* @retval 0成功，否则失败
*/
static int camera_set_ircut_b(int value)
{
	char cmd[STRING_LEN];
	char result[STRING_LEN];

	memset(result, '\0', STRING_LEN);
	sprintf(cmd, "echo %d > %s", value, IRCUT_B_FILE_NAME);
	do_syscmd(cmd, result);

	return 0;
}

/**
* @brief ircut线程
* @author ygh
* @date 2015-10-25
* @param[in] 
* @return int
* @retval 
*/

/*********** 光敏输入信号 真值表 ***************
	1 : 白天模式，红外灯未开启。
	0 : 夜间模式，红外灯开启。
*/

/*********** ircut 真值表 ***************
	ircut有效 : ircut_a=0, ircut_b=1
	ircut无效 : ircut_a=1, ircut_b=0
	ircut空闲 : ircut_a=0, ircut_b=0
*/

static void camera_ircut_set_idly(void)
{
	camera_set_ircut_a(0);
	camera_set_ircut_b(0);
}

static int nighttime_mode = 0;

int camera_is_nighttime_mode(void)
{
	return nighttime_mode;
}

static void camera_ircut_set(int ir_feed, int mode, char *name)
{
	if (mode == IRCUT_MODE_1LINE) {
		anyka_print("Ircut 1line mode, ir_feed:%d\n",ir_feed);
		if (ir_feed == IR_FEED_DAYTIME_VALUE)
		{
			/*白天*/
			camera_set_ircut(0, name);
			Isp_Module_Switch_Mode(MODE_DAYTIME);
			nighttime_mode = 0;
		}
		else if (ir_feed == (1 - IR_FEED_DAYTIME_VALUE))
		{
			/*夜间*/
			Isp_Module_Switch_Mode(MODE_NIGHTTIME);
			camera_set_ircut(1, name);
			nighttime_mode = 1;
		}
        isp_exptime_max_set(); //重新设置当前帧率下的曝光时间
        return ;
	}

	anyka_print("Ircut 2line mode, ir_feed:%d\n",ir_feed);
	if (ir_feed == IR_FEED_DAYTIME_VALUE) {
		/*白天*/
		camera_set_ircut_a(0);
		camera_set_ircut_b(1);
		Isp_Module_Switch_Mode(MODE_DAYTIME);
		nighttime_mode = 0;
	} else if (ir_feed == (1 - IR_FEED_DAYTIME_VALUE)) {
		/*夜间*/
		Isp_Module_Switch_Mode(MODE_NIGHTTIME);
		camera_set_ircut_a(1);
		camera_set_ircut_b(0);
		nighttime_mode = 1;
	}

	usleep(10000);
	camera_ircut_set_idly();

	isp_exptime_max_set(); //重新设置当前帧率下的曝光时间
}

void *camera_ircut_pthread(void)
{
	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());

	char *ircut_name;
	int ircut_mode;
	int flags = 0;
	int rf_feed_old_level;
	struct stat st;

	sleep(3);
	reload_config_isp_auto_awb_step();
		
	/* 如果没有光敏接口文件，则认为不存在ircut的功能而退出线程 */
    /*if (-1 == stat(IR_FEED_FILE_NAME, &st)) {
        anyka_print("Cannot identify '%s': %d, %s\n",
                 IR_FEED_FILE_NAME, errno, strerror(errno));
		return NULL;
    }*/

    if (-1 == stat(IRCUT_A_FILE_NAME, &st)) {
        anyka_print("Cannot identify '%s': %d, %s\n",
                 IRCUT_A_FILE_NAME, errno, strerror(errno));
		flags |= 1;
    } else {
		ircut_name = IRCUT_A_FILE_NAME;
	}

    if (-1 == stat(IRCUT_B_FILE_NAME, &st)) {
        anyka_print("Cannot identify '%s': %d, %s\n",
                 IRCUT_B_FILE_NAME, errno, strerror(errno));
		flags |= 1 << 1;
    } else {
		ircut_name = IRCUT_B_FILE_NAME;
	}

	if (!flags) {
		anyka_print("Ircut a & b interface all can access\n");
		ircut_mode = IRCUT_MODE_2LINE;
	} else if (flags == 0x3) {
		anyka_print("Ircut a & b interface can't access\n");
		return NULL;
	} else {
		anyka_print("Only can access:%s\n", ircut_name);
		ircut_mode = IRCUT_MODE_1LINE;
	}


	rf_feed_old_level = camera_get_ir_feed();

	camera_ircut_set(rf_feed_old_level, ircut_mode, ircut_name);
	
	ircut_pthread_running = 1;

	while (ircut_pthread_running)
	{
		int rf_feed_level = camera_get_ir_feed();

		if (rf_feed_level < 0)
		{
			sleep(2);
			continue;
		}
		//anyka_print("ir feed level: %d\n", rf_feed_level);

		if (rf_feed_level != rf_feed_old_level)
		{
			/* 以下延时为了去抖。*/
			sleep(2);
			if (rf_feed_level == camera_get_ir_feed())
			{
				camera_ircut_set(rf_feed_level, ircut_mode, ircut_name);

				rf_feed_old_level = rf_feed_level;
				sleep(5);
			}
		} else {
			sleep(2);
		}
	}

	return NULL;
}

enum FPS_STAT {
	LOW_FPS_STAT = 0,
	HIGH_FPS_STAT,
};

enum GAIN_STAT {
	GAIN_NO_CHANGE_STAT = 0,
	GAIN_LOW_STAT,
	GAIN_HIGH_STAT,
};

struct stru_sensor_info {
	int sensor_id;
	int high_fps_exptime_max;
	int low_fps_exptime_max;
	int high_fps_switch_gain;
	int low_fps_switch_gain;
};

struct stru_fps_conf_info {
	enum FPS_STAT fps_stat;
	int cur_fps;
	int daytime_low_fps;
	int daytime_high_fps;
	int nighttime_fps;

	struct stru_sensor_info *sensor_info;
};

static char switch_fps_pthread_running = 1;
static struct stru_fps_conf_info fps_conf_info;
static struct stru_sensor_info sensor_info[] = {
	{
		//sc1045
		.sensor_id				= 0x1045,
		.high_fps_exptime_max	= 1796,
		.low_fps_exptime_max	= 1796,
		.high_fps_switch_gain	= 20,
		.low_fps_switch_gain	= 8,
	},
	{
		//sc1035
		.sensor_id				= 0x1035,
		.high_fps_exptime_max	= 1996,
		.low_fps_exptime_max	= 1996,
		.high_fps_switch_gain	= 20,
		.low_fps_switch_gain	= 8,
	},
	{
		//h42
		.sensor_id				= 0xa042,
		.high_fps_exptime_max	= 1800,
		.low_fps_exptime_max	= 1800,
		.high_fps_switch_gain	= 20,
		.low_fps_switch_gain	= 8,
	},
};

static int fps_conf_info_init(void)
{
	struct stru_fps_conf_info *p_fps_conf_info = &fps_conf_info;

    //针对Q物联或大拿等云平台版本，此处会设置sensor的输出帧率为25的一半，这样会获取比较好的图像效果。
    //注意必须用13帧，否则会引起音视频不同步。
    //如果需要高帧率，可以注释该语句重新获得25帧。
    #if (defined(CONFIG_DANA_SUPPORT) || defined(CONFIG_TENCENT_SUPPORT))
    cur_camera_frame = max_camera_frame = 13;
    #endif
    
	p_fps_conf_info->fps_stat			= HIGH_FPS_STAT;
	p_fps_conf_info->cur_fps			= max_camera_frame;
	p_fps_conf_info->daytime_low_fps	= max_camera_frame;//max_camera_frame / 2;
	p_fps_conf_info->daytime_high_fps	= max_camera_frame;
	p_fps_conf_info->nighttime_fps		= max_camera_frame;

	int id = camera_get_sensor_id();
	if (id < 0) {
		anyka_print("%s read sensor id fail\n", __func__);
		return -1;
	}

	int i;
	int sensor_info_num = sizeof(sensor_info) / sizeof(sensor_info[0]);
	for (i = 0; i < sensor_info_num; i++) {
		if (id == sensor_info[i].sensor_id) {
			break;
		}
	}
	
	if (i >= sensor_info_num) {
		anyka_print("%s id:0x%x have not info to switch fps\n", __func__, id);
		return -1;
	}

	p_fps_conf_info->sensor_info = &sensor_info[i];

	return 0;
}

static int get_gain_stat(int *res_gain)
{
	enum GAIN_STAT stat = GAIN_NO_CHANGE_STAT;
	struct stru_fps_conf_info *p_fps_conf_info = &fps_conf_info;
	
#if 0
	struct stru_fps_conf_info *p_fps_conf_info = &fps_conf_info;

	switch (p_fps_conf_info->fps_stat) {
		case HIGH_FPS_STAT:
		if (fps_conf_info_is_all_gain_high()) {
			p_fps_conf_info->fps_stat = LOW_FPS_STAT;
			stat = GAIN_LOW_STAT;
		}
		break;
	
		case LOW_FPS_STAT:
		if (fps_conf_info_is_all_gain_low()) {
			p_fps_conf_info->fps_stat = HIGH_FPS_STAT;
			stat = GAIN_HIGH_STAT;
		}
		break;
	
		default:
		break;
	}
#endif

	int gain = 0;
	T_AESTAT_INFO ae_stat;

#if 1
	char cmd[STRING_LEN];
	char result[STRING_LEN];

	memset(result, '\0', STRING_LEN);

	if (access("/tmp/isp_gain", F_OK) == 0) {
		sprintf(cmd, "cat %s", "/tmp/isp_gain");
		do_syscmd(cmd, result);
		gain = atoi(result);
	} else 
#endif
	{
		if (Isp_Module_AeStat_Get(&ae_stat)) {
			stat = GAIN_NO_CHANGE_STAT;
			goto end;
		}

		//anyka_print("%s a_gain:%d, d_gain:%d, isp_d_gain:%d\n", __func__, (int)ae_stat.a_gain, (int)ae_stat.d_gain, (int)ae_stat.isp_d_gain);
		gain = (ae_stat.a_gain >> 8) * (ae_stat.d_gain >> 8) * (ae_stat.isp_d_gain >> 8);
	}

	if (gain > p_fps_conf_info->sensor_info->high_fps_switch_gain) {
		stat = GAIN_HIGH_STAT;
	} else if (gain < p_fps_conf_info->sensor_info->low_fps_switch_gain) {
		stat = GAIN_LOW_STAT;
	} else {
		stat = GAIN_NO_CHANGE_STAT;
	}

end:
	//anyka_print("%s gain:%d stat:%d\n", __func__, gain, stat);
	*res_gain = gain;
	return stat;
}

static void isp_exptime_max_set(void)
{
	struct stru_fps_conf_info *p_fps_conf_info = &fps_conf_info;
	int i=0;

    //此处要注意change_fps_thread跑的慢的情况，要等待它的初始化完毕
    while(!p_fps_conf_info->sensor_info && i++<2)
    {
        usleep(300);
    }
	if (p_fps_conf_info->sensor_info)
	{
		if (cur_camera_frame >= max_camera_frame)
			Isp_Module_AeAttr_Exptime_Max_Set(p_fps_conf_info->sensor_info->high_fps_exptime_max);
		else
			Isp_Module_AeAttr_Exptime_Max_Set(p_fps_conf_info->sensor_info->low_fps_exptime_max);
	}
}


static int camera_need_set_fps(void)
{
#if 0
	#define SWITCH_FPS_TEST_FILE "/tmp/switch_fps"
	static int old_fps = 0;
	int fps;
	struct stat st;
	char cmd[64];
	char result[64];

	if (-1 == stat(SWITCH_FPS_TEST_FILE, &st))
		return 0;
	sprintf(cmd, "cat %s", SWITCH_FPS_TEST_FILE);
	memset(result, 0, 64);
	do_syscmd(cmd, result);
	fps = atoi(result);
	if (fps == old_fps)
		return 0;
	old_fps = fps;
	anyka_print("%s fps:%d\n", __func__, fps);
	return fps;
#else
	int gain;
	int new_fps;
	struct stru_fps_conf_info *p_fps_conf_info = &fps_conf_info;

	enum GAIN_STAT stat = get_gain_stat(&gain);
	switch (stat) {
		case GAIN_NO_CHANGE_STAT:
		goto no_change;
		break;

		case GAIN_LOW_STAT:
		new_fps = p_fps_conf_info->daytime_high_fps;
		break;

		case GAIN_HIGH_STAT:
		new_fps = p_fps_conf_info->daytime_low_fps;
		break;

		default:
		break;
	}

	if (new_fps != p_fps_conf_info->cur_fps) {
		p_fps_conf_info->cur_fps = new_fps;
		anyka_print("[%s]change sensor fps, new_fps:%d, current gain:%d\n", __func__, new_fps, gain);
		return new_fps;
	}

no_change:
	return -1;
#endif
}

void *camera_change_fps_pthread(void)
{
	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());

	int fps;
	struct stru_fps_conf_info *p_fps_conf_info = &fps_conf_info;
	
	switch_fps_pthread_running = 1;

	if (!p_fps_conf_info->sensor_info)
		goto exit;
	
	camera_set_fps(25);
	anyka_print("[%s:%d] cur_camera_frame: %d\n", __func__, __LINE__, cur_camera_frame);
	camera_set_fps(cur_camera_frame);
	isp_exptime_max_set();

	while (switch_fps_pthread_running)
	{
		fps = camera_need_set_fps();
		if ((fps > 0) && !camera_set_fps(fps))
		{
			cur_camera_frame = camera_get_fps();
			isp_exptime_max_set();
			anyka_print("%s cur_camera_frame:%d\n", __func__, cur_camera_frame);
		}

		sleep(5);
	}

exit:
	anyka_print("[%s:%d] This thread exit\n", __func__, __LINE__);
	return NULL;
}

static int save_yuv_flg = 0;

void camera_do_save_yuv(void *pbuf, long size)
{
	if(!save_yuv_flg)
		return;
	anyka_print("[%s:%d] save yuv, size: %ld\n", __func__, __LINE__, size);

	int save_yuv_fd = open("/tmp/isp_yuv.img", O_CREAT | O_APPEND | O_TRUNC | O_WRONLY);
	if(save_yuv_fd < 0) {
		perror("open");
		return;
	}

	int ret = 0;
	while(size > 0) {
		ret = write(save_yuv_fd, pbuf, size);
		pbuf += ret;
		size -= ret;
	}

	close(save_yuv_fd);

	char cmd[100] = {0};
	sprintf(cmd, "echo %s > /tmp/img_status", "img ok:/tmp/isp_yuv.img");
	system(cmd);

	save_yuv_flg = 0;

}

void camera_save_yuv(void)
{
	save_yuv_flg = 1;
}


