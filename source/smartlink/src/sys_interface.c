#include "common.h"
#include "convert.h"

#define CONFIG_WIFI_FILE_NAME  "/etc/jffs2/anyka_cfg.ini"
#define WIRE_LESS_TMP_DIR      "/tmp/wireless/"

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
bool readBufferFromFile(char *pPath, unsigned char *pBuffer, int nInSize, int *pSizeUsed)
{
	if (!pPath || !pBuffer)
	{
        anyka_print("[%s,%d]the param is error\n ", __FUNCTION__, __LINE__);
		return false;
	}

	int uLen = 0;
	FILE * file = fopen(pPath, "rb");
	if (!file)
	{
        anyka_print("[%s,%d]it fail to open the file\n ", __FUNCTION__, __LINE__);
	    return false;
	}

	do
	{
	    fseek(file, 0L, SEEK_END);
	    uLen = ftell(file);
	    fseek(file, 2, SEEK_SET);

	    if (0 == uLen)
	    {
	    	printf("invalide file or buffer size is too small...\n");
	        break;
	    }

	    *pSizeUsed = fread(pBuffer, 1, nInSize, file);

	    fclose(file);
	    return true;

	}while(0);

    fclose(file);
	return false;
}

/**
 * @brief 保存smartlink的ip地址和端口号信息到临时文件
 * @param[in] ip  ip地址
 * @param[in] port 端口号
 */
void save2file(char *ip, int port)
{
	FILE * file = fopen("/tmp/smartlink", "w");
	if (file)
	{
        fprintf(file, "%s\n%d\n", ip, port);
        fclose(file);
	}
}

/**
* @brief 从系统配置文件获取wifi配置信息
* @author ye_guohong
* @date 2015-03-31
* @param[out]   wifi_info       wifi配置信息结构体
* @return int
* @retval if return 0 success, otherwise failed
*/
static int get_sys_wifi_info(struct smartlink_wifi_info *wifi_info)
{
	void *config_handle;

	config_handle = anyka_config_init(CONFIG_WIFI_FILE_NAME);
	if (config_handle == NULL) {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
		return -1;	
	}

	anyka_config_get_title(config_handle, "wireless", "ssid", wifi_info->ssid, "");
	anyka_config_get_title(config_handle, "wireless", "password", wifi_info->passwd, "123456789");


	anyka_config_destroy(CONFIG_WIFI_FILE_NAME, config_handle);

	return 0;
}

/**
* @brief 将wifi配置信息写入配置文件
* @author ye_guohong
* @date 2015-03-31
* @param[in]    wifi_info       wifi配置信息结构体
* @return int
* @retval if return 0 success, otherwise failed
*/
static int set_sys_wifi_info(struct smartlink_wifi_info *wifi_info)
{
	void *config_handle;

	config_handle = anyka_config_init(CONFIG_WIFI_FILE_NAME);
	if (config_handle == NULL) {
		anyka_print("[%s:%d] init config file failed.\n", __func__, __LINE__);
		return -1;	
	}

	//anyka_config_set_title(config_handle, "wireless", "ssid", wifi_info->ssid);
	anyka_config_set_title(config_handle, "wireless", "password", wifi_info->passwd);


	anyka_config_destroy(CONFIG_WIFI_FILE_NAME, config_handle);

	return 0;
}

/**
* @brief 存储wifi配置信息总入口
* @author ye_guohong
* @date 2015-03-31
* @param[in]    wifi_info       wifi配置信息结构体
* @return int
* @retval if return 0 success, otherwise failed
*/
int store_wifi_info(struct smartlink_wifi_info *wifi_info)
{
	struct smartlink_wifi_info tmp_info;

	get_sys_wifi_info(&tmp_info);

	set_sys_wifi_info(wifi_info);

	return 0;
}

/**
* @brief save_ssid_to_tmp
* 		 save the ssid to tmp file
* @author cyh
* @date 2015-11-05
* @param[in]  ssid
* @return int
* @retval if return 0 success, otherwise failed
*/

int save_ssid_to_tmp(char *ssid, unsigned int ssid_len)
{
	int ret;
	char gbk_exist = 0, utf8_exist = 0;
	char gbk_path_name[32] = {0}; //path name
	char utf8_path_name[32] = {0};
	char gbk_ssid[32] = {0}; //store ssid after change encode func

	sprintf(gbk_path_name, "%s/gbk_ssid", WIRE_LESS_TMP_DIR);
	sprintf(utf8_path_name, "%s/utf8_ssid", WIRE_LESS_TMP_DIR);

	ret = mkdir(WIRE_LESS_TMP_DIR, O_CREAT | 0666);
	if(ret < 0) {
		if(errno == EEXIST)	{
			printf("[%s:%d] the %s exist\n",
				   	__func__, __LINE__, WIRE_LESS_TMP_DIR);

			/** check file exist **/
			if(access(gbk_path_name, F_OK) == 0)
				gbk_exist = 1;
			
			if(access(utf8_path_name, F_OK) == 0)
				utf8_exist = 1;	

			if(gbk_exist && utf8_exist) {
				printf("[%s:%d] the wireless config has been saved," 
					"now do nothing\n", __func__, __LINE__);
				return 0;
			}
			
		} else {
			printf("[%s:%d] make directory %s, %s\n", __func__,
				   	__LINE__, WIRE_LESS_TMP_DIR, strerror(errno));
			return -1;
		}
	}

	if(utf8_exist == 0) {

		FILE *filp_utf8 = fopen(utf8_path_name, "w+");	
		if(!filp_utf8) {
			printf("[%s:%d] open: %s, %s\n", 
					__func__, __LINE__, utf8_path_name, strerror(errno));
			return -1;
		}

		ret = fwrite(ssid, 1, ssid_len + 1, filp_utf8);
		if(ret != ssid_len + 1) {
			printf("[%s:%d] fails write data\n", __func__, __LINE__);
			fclose(filp_utf8);
			return -1;
		}
		fclose(filp_utf8);
	}

	if(gbk_exist == 0) {
		
		/** utf-8 to gbk code change **/
		ret = u2g(ssid, ssid_len, gbk_ssid, 32);
		if(ret < 0) {
			printf("[%s:%d] faile to change code from utf8 to gbk\n", 
				__func__, __LINE__);
			return -1;
		}
		printf("*** u2g changed[%d], %s\n", ret, gbk_ssid);

		FILE *filp_gbk = fopen(gbk_path_name, "w+");
		if(!filp_gbk) {
			printf("[%s:%d] open: %s, %s\n", 
					__func__, __LINE__, gbk_path_name, strerror(errno));
			return -1;
		}
		
		ret = fwrite(gbk_ssid, 1, strlen(gbk_ssid) + 1, filp_gbk);
		if(ret != strlen(gbk_ssid) + 1) {
			printf("[%s:%d] fails write data\n", __func__, __LINE__);
			fclose(filp_gbk);
			return -1;
		}
		fclose(filp_gbk);

	}
	
	return 0;
}

