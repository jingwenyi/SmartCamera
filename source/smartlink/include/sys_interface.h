#ifndef __SYS_INTERFACE_H__
#define __SYS_INTERFACE_H__

#include <stdbool.h>

struct smartlink_wifi_info {
	char ssid[128];
	char passwd[128];
};

/**
* @brief 从文件读取指定长度数据
* @author ye_guohong
* @date 2015-03-31
* @param[in]    pPath       文件名
* @param[out]   pBuffer     读取数据存放位置
* @param[out]   nInSize     已读取数据长度
* @return bool
* @retval if return true success, otherwise failed
*/
bool readBufferFromFile(char *pPath, unsigned char *pBuffer, int nInSize, int *pSizeUsed);

/**
 * @brief 保存smartlink的ip地址和端口号信息到临时文件
 * @param[in] ip  ip地址
 * @param[in] port 端口号
 */
void save2file(char *ip, int port);

/**
* @brief 存储wifi配置信息总入口
* @author ye_guohong
* @date 2015-03-31
* @param[in]    wifi_info       wifi配置信息结构体
* @return int
* @retval if return 0 success, otherwise failed
*/
int store_wifi_info(struct smartlink_wifi_info *wifi_info);

/**
* @brief save_ssid_to_tmp
* 		 save the ssid to tmp file
* @author cyh
* @date 2015-11-05
* @param[in]  ssid
* @return int
* @retval if return 0 success, otherwise failed
*/

int save_ssid_to_tmp(char *ssid, unsigned int ssid_len);

#endif
