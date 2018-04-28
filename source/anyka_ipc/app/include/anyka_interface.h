#ifndef _anyka_interface_h_
#define _anyka_interface_h_


typedef enum
{
	S_Upright=0,
	S_Horizontal,
	S_Vertical,
	S_FLIP	
}T_CAMER_FLIP_STATE;

typedef struct _anyka_ap_info {
    char essid[33];
    uint32_t enc_type;
    uint32_t quality;
} anyka_ap_info, *Panyka_ap_info;

typedef struct _anyka_ap_list{
    int ap_num;
    anyka_ap_info ap_info[32];
} anyka_ap_list, *Panyka_ap_list;

/**
 * NAME         GetCamerFilp
 * @BRIEF	获取图像画面值，正转，反转等
 * @PARAM	void
 * @RETURN	int
 * @RETVAL	当前画面状态值
 */

int GetCamerFilp( void );


/**
 * NAME         SetCamerFlip
 * @BRIEF	设置图像画面值，正转，反转等
 * @PARAM	int state
 			要设置的状态值
 * @RETURN	int
 * @RETVAL	0
 */

int SetCamerFlip(int state);

/**
 * NAME         anyka_clear_all_resource
 * @BRIEF	清空对讲的所有资源
 * @PARAM	void *mydata
 			要清空的资源
 * @RETURN	void
 * @RETVAL	
 */

void anyka_clear_all_resource(void *mydata);

/**
* @brief  Get camer para
* 
* @author liqing
* @date 2013-10-21
* @cl[out]return camer info
* @return int
* @retval if return 0 success, otherwise failed 
*/
int Get_Color(int *bright, int *constast, int *saturation, int *hue);

/**
* @brief  set camer para
* 
* @author liqing
* @date 2013-10-21
* @bright[in] bright
* @constast[in] constast
* @saturation[in]stauration
* @hue[in]hue
* @return int
* @retval if return 0 success, otherwise failed 
*/
int Set_Color(int bright, int constast, int saturation, int hue);

/**
 * NAME         Set_Freq
 * @BRIEF	设置电源频率
 * @PARAM	int freq，要设置的电源频率值
 * @RETURN	int
 * @RETVAL	0
 */

int Set_Freq(int freq);

/**
 * NAME         Get_Freq
 * @BRIEF	获取电源频率
 * @PARAM	void
 * @RETURN	int
 * @RETVAL	返回当前电源频率
 */

int Get_Freq( void );

/**
 * NAME         SetCamerFlip
 * @BRIEF	设置图像画面值，正转，反转等
 * @PARAM	int state
 			要设置的状态值
 * @RETURN	int
 * @RETVAL	0
 */

int SetCamerFlip(int state);

/**
 * NAME         anyka_clear_all_resource
 * @BRIEF	清空对讲的所有资源
 * @PARAM	void *mydata
 			要清空的资源
 * @RETURN	void
 * @RETVAL	
 */

int GetCamerFilp( void );

/**
 * NAME         anyka_get_ap_list
 * @BRIEF	获取AP 列表
 * @PARAM	void
 * @RETURN	Panyka_ap_list
 * @RETVAL	指向AP 列表的指针
 */

Panyka_ap_list anyka_get_ap_list();

#endif

