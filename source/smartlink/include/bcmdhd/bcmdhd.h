#ifndef __BCMDHD_H__
#define __BCMDHD_H__

/**
* @brief 是否加载了bcmdhd wifi驱动
* @author ye_guohong
* @date 2015-03-31
* @param[in]    void
* @return int
* @retval if return 0 success, otherwise failed
*/
int is_bcmdhd(void);

/**
* @brief bcmdhd wifi smartlink总入口
* @author ye_guohong
* @date 2015-03-31
* @param[in]    void
* @return int
* @retval if return 0 success, otherwise failed
*/
int bcmdhd_smartlink(void);

#endif
