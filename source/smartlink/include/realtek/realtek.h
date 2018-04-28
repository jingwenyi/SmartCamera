#ifndef __REALTEK_H__
#define __REALTEK_H__

/**
* @brief 判别是否加载了realtek驱动
* @author ye_guohong
* @date 2015-03-31
* @param[out]   
* @return int
* @retval if return 1 success, otherwise failed
*/
int is_realtek(void);

/**
* @brief realtek smartlink总入口
* @author ye_guohong
* @date 2015-03-31
* @param[out]   
* @return int
* @retval if return 0 success, otherwise failed
*/
int realtek_smartlink(void);

#endif
