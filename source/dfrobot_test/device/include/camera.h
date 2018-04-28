#ifndef CAMERA_H
#define CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void* (*FUNC_DMA_MALLOC)(unsigned long size);
typedef void (*FUNC_DMA_FREE)(void *point);

typedef struct {
	FUNC_DMA_MALLOC dma_malloc;
	FUNC_DMA_FREE	dma_free;
	unsigned long width;
	unsigned long height;
} T_CAMERA_INPUT;




/**
* @brief open camera to get picture
* @author dengzhou 
* @date 2013-04-25
* @T_U32 width
* @T_U32 height
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
//int camera_open(void);
int camera_open(int w, int h);


int camera_ioctl(unsigned long width, unsigned long height);


/**
* @brief get one frame from camera
* @author dengzhou 
* @date 2013-04-25
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
void * camera_getframe(void);

void camera_get_camera_info(void *camera_buf, void **pbuf, long *size, unsigned long *timeStamp);

/**
* @brief call this when camera buffer unused
* @author dengzhou 
* @date 2013-04-25
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/


int camera_usebufok(void *pbuf);

/**
* @brief close camera
* @author dengzhou 
* @date 2013-04-25
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
int camera_close(void);
void camera_off(void);
void camera_on(void);
int camera_set_gamma(int cid);
int camera_set_occ(int num, int x1, int y1, int x2, int y2);
int camera_set_channel2(int width, int height);
int camera_set_brightness(int bright);
int camera_set_zoom(int z);
int camera_set_saturation(int sat);
int camera_set_power_frequency(int freq );
T_VOID  camera_show_curtime_onvideo(T_U8 * destBuf, T_S32 width, T_S32 height, T_S32 size, time_t osd_time);
int camera_set_osd_context_attr(time_t osd_time);
int camera_set_vflip(void);
int camera_set_mirror(void);
int camera_set_normal(void);
int camera_set_flip(void);
int camera_get_ch1_height(void);
int camera_get_ch2_height(void);

int camera_get_max_frame(void);
void camera_hardware_time_stamp(void);

void *camera_ircut_pthread(void);

void camera_save_yuv(void);

void camera_do_save_yuv(void *pbuf, long size);
int camera_is_nighttime_mode(void);
void *camera_change_fps_pthread(void);

int camera_get_fps(void);
int camera_set_fps(int fps);



#ifdef __cplusplus
} /* end extern "C" */
#endif
#endif // CAMERA_H

