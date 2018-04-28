#ifndef _anyka_photo_h_
#define _anyka_photo_h_


typedef void anyka_photo_callabck(void *parm, char *pic_path);

/**
 * NAME         anyka_photo_init
 * @BRIEF	启动拍照线程，等待用户拍照指令
 * @PARAM	photo_path  拍照的路径
 * @RETURN	
 * @RETVAL	
 */

int anyka_photo_init(char *photo_path);


/**
 * NAME         anyka_photo_start
 * @BRIEF	开始启动拍照
 * @PARAM	total_time   拍照时间，拍照将会持续的时间,如果为-1,将持续拍照
                    time_per_phone     拍照的间隔时间
                    photo_path  拍照的路径，如果为NULL,将使用上次拍照的路径
                    ppic_tell     如果拍照结束，将通知用户
                    para           回调函数的参数
 * @RETURN	
 * @RETVAL	
 */

int anyka_photo_start(int total_time, int time_per_phone, char *photo_path, anyka_photo_callabck *ppic_tell, void *para);


/**
 * NAME         anyka_photo_stop
 * @BRIEF	提前结束之前启动的拍照
 * @PARAM	void
 * @RETURN	void
 * @RETVAL	
 */

void anyka_photo_stop();

#endif

