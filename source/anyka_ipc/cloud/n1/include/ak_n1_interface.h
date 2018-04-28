
#ifndef _AK_NI_INTERFACE_H_
#define _AK_NI_INTERFACE_H_


#define IMG_EFFECT_DEF_VAL	50

#define OSD_TITLE_LEN_MAX	45

typedef struct 
{
	T_U32	hue;
	T_U32	brightness;
	T_U32	saturation;
	T_U32	contrast;
	T_U32	sharp;

	T_BOOL	flip;
	T_BOOL	mirror;

}T_IMG_SET_VALUE;

typedef struct 
{
	unsigned int mainbps;
	unsigned int subbps;
	unsigned int videomode;		//0:CBR; 1:VBR
	
}T_VIDEO_SET_VALUE;

extern T_IMG_SET_VALUE g_img_val;
extern T_VIDEO_SET_VALUE g_video_set;


#define CONNECT_NUM_MAX	4


typedef struct
{
    pthread_mutex_t     lock;
	pthread_mutex_t     streamlock;
    void *video_queue;
	void *audio_queue;
	int video_run_flag;
	int audio_run_flag;
	bool bValid;
	bool bMainStream;
	bool biFrameflag;
}N1_CONNECT;

typedef struct 
{
	pthread_mutex_t     lock;
	pthread_mutex_t     mdFilelock;
	pthread_mutex_t     usrFilelock;
	uint32_t	conn_cnt;
    N1_CONNECT	conn[CONNECT_NUM_MAX];
}N1_CTRL;

extern N1_CTRL *pn1_ctrl;

/**
* @brief 	ak_n1_update_osd_font
* 			更新osd显示
* @date 	2015/12
* @param:	utf 8 编码的字符串
* @return 	int
* @retval 	0->success, -1 failed
*/

int ak_n1_update_osd_font(char *input);


void ak_n1_update_osd_off(void);


#endif
