#ifndef _ANYKA_INI_H_
#define _ANYKA_INI_H_
/**
* @brief  初始化配置文件系统，将完成从配置文件读取内容，并完成数据结构的初始化
* 
* @author aijun
* @date 2014-11-28
* @param[in]  void *config_name 配置文件名
* @return T_S32
* @retval if return 0 fail, otherwise the handle of the config
*/

void * anyka_config_init(char *config_name);



/**
* @brief  释放配置句柄所申请的内存,如果配置项产生变化，将保存文件
* 
* @author aijun
* @date 2014-11-28
* @param[in]  void *config_name  配置文件名
                     void *config_handle 配置文件句柄
* @return T_S32
* @retval none
*/

int anyka_config_destroy(char *config_name, void *config_handle);



/**
* @brief  更新配置文件的相关值,只设置，并不保存,只写入缓冲
* 
* @author aijun
* @date 2014-11-28
* @param[in]  void *config_handle 配置文件句柄
                     char *title   配置头项
                     char *name 配置项名称
                     char *value 要设置的值
* @return int
* @retval if return 0 success, otherwise failed 
*/

int anyka_config_set_title(void *config_handle, char *title, char *name, char *value);



/**
* @brief 得到配置文件的相关值，没有打开文件操作，是从之前的缓冲中读取
* 
* @author aijun
* @date 2014-11-28
* @param[in]  void *config_handle 配置文件句柄
                     char *title   配置头项
                     char *name 配置项名称
                     char *value 要设置的值
* @return int
* @retval if return 0 success, otherwise failed 
*/

//int anyka_config_get_title(void *config_handle, char *title, char *name, char *value);
int anyka_config_get_title(void *config_handle, char *title, char *name, char *value, char *default_value);




/**
* @brief  更新配置文件的相关值,只设置，将直接修改配置文件的相关项
* 
* @author aijun
* @date 2014-11-28
* @param[in]  void *config_handle 配置文件句柄
                     char *title   配置头项
                     char *name 配置项名称
                     char *value 要设置的值
* @return int
* @retval if return 0 success, otherwise failed 
*/

int anyka_config_set_item(char *config_name, char *title, char *name, char *value);


/**
* @brief 得到配置文件的相关值,是直接从文件中读取，
* 
* @author aijun
* @date 2014-11-28
* @param[in]  char *config_name 配置文件名
                     char *title   配置头项
                     char *name 配置项名称
                     char *value 要设置的值
* @return int
* @retval if return 0 success, otherwise failed 
*/

//int anyka_config_get_item(char *config_name, char *title, char *name, char *value);
int anyka_config_get_item(char *config_name, char *title, char *name, char *value, char *default_value);



#endif

