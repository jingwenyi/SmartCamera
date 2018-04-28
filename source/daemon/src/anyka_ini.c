
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>

#include "daemon_inc.h"


#define CONFIG_ITEM_SIZE    50

struct _anyka_config_item_info;
typedef struct _anyka_config_item_info
{
    char name[CONFIG_ITEM_SIZE];
    char value[CONFIG_ITEM_SIZE];
    struct _anyka_config_item_info *next;
}anyka_config_item_info, *Panyka_config_item_info;

struct _anyka_config_title_info;
typedef struct _anyka_config_title_info
{
    int update;
    char title[CONFIG_ITEM_SIZE];
    Panyka_config_item_info item;
    struct _anyka_config_title_info *next;
}anyka_config_title_info, *Panyka_config_title_info;


int anyka_config_save(char *config_name, void *config_handle);

/**
* @brief  过滤字符串前面和后面的空格与TAB
* 
* @author aijun
* @date 2014-11-28
* @param[in]  char *src
                     char *dest
* @return none
* @retval 
*/

void anyka_config_filter_space(char *src, char *dest)
{
	int len;
	char *start;
	
	start = strstr(src, "####");
	if(start)
	{
		*start = 0;
	}
	
    while(*src && (*src == ' ' || *src == '\t'))
    {
        src ++;
    }
	
	len = strlen(src) - 1;
    while((src[len] == ' ' || src[len] == '\t' || src[len] == '\r' || src[len] == '\n'))
    {
        len --;
    }
    src[len + 1] = 0;    
	strcpy(dest, src);
}

Panyka_config_item_info anyka_config_insert_item(Panyka_config_title_info title, Panyka_config_item_info cur)
{	
    Panyka_config_item_info head = title->item;
    
    if(head == NULL)
    {
        cur->next = NULL;
        title->item = cur;
        return title->item;
    }

    while(head->next)
    {
        head = head->next;
    }
    cur->next = NULL;
    head->next = cur;
		
    return title->item;
}

Panyka_config_title_info anyka_config_insert_title(Panyka_config_title_info head, Panyka_config_title_info cur)
{	
	Panyka_config_title_info link_head = head;

	cur->update = 0;
    if(head == NULL)
    {
        cur->next = NULL;
        return cur;
    }

    while(head->next)
    {
        head = head->next;
    }
    head->next = cur;
    cur->next = NULL;
    cur->item = NULL;
	
    return link_head;
}

Panyka_config_title_info anyka_config_insert_null_title()
{
	Panyka_config_title_info config;
	config = (Panyka_config_title_info)malloc(sizeof(anyka_config_title_info)); 
	strcpy(config->title, "anyka_title");
	config->next = NULL;
	config->item = NULL;
	config->update = 0;
	
    return config;
}



/**
* @brief  初始化配置文件系统，将完成从配置文件读取内容，并完成数据结构的初始化
* 
* @author aijun
* @date 2014-11-28
* @param[in]  void *config_name 配置文件名
* @return T_S32
* @retval if return 0 fail, otherwise the handle of the config
*/

void * anyka_config_init(char *config_name)
{	
    Panyka_config_title_info head = NULL, config = NULL;    
    Panyka_config_item_info item, next;
    FILE *file_id = fopen(config_name, "r");
    char *file_buf = NULL, *find;
    char info[100], name[100];
    
    if(file_id == NULL)
    {
        anyka_print("[%s,%d]:config file didn't open : %s, create it.\n",__func__, __LINE__, config_name);
		head = anyka_config_insert_null_title();
        return head;
    }
    
    file_buf = (char *)malloc(512);
    if(file_buf == NULL)
    {
        anyka_print("[%s,%d]:it fails to malloc!\n",__func__, __LINE__);
        fclose(file_id);
        return NULL;
    }

    while(!feof(file_id))
    {
        if(fgets(file_buf, 512, file_id) == NULL)
        {
            break;
        }
        anyka_config_filter_space(file_buf, info);
        if(info[0] == '[')
        {
            config = (Panyka_config_title_info)malloc(sizeof(anyka_config_title_info));
            if(config == NULL)
            {
                goto anyka_config_init_err_tbl;
            }
            find = strchr(info, ']');
            *find = 0;
            strcpy(config->title, info + 1);
			config->item = NULL;
            head = anyka_config_insert_title(head, config);
        }
        else if((find = strchr(info, '=')))
        {
            if(config == NULL)
            {
                anyka_print("[%s,%d]:we find item before title\n",__func__, __LINE__);
				continue;
				/*config = (Panyka_config_title_info)malloc(sizeof(anyka_config_title_info));
				strcpy(config->title, "anyka_title");
				config->item = NULL;
				head = anyka_config_insert_title(head, config);	
				*/
            }
            item = (Panyka_config_item_info)malloc(sizeof(anyka_config_item_info));
            if(item == NULL)
            {
                goto anyka_config_init_err_tbl;
            }
            *find = 0;
            find ++;
			
            anyka_config_filter_space(info, name);
            strcpy(item->name, name);
            anyka_config_filter_space(find, name);
            strcpy(item->value, name);
			
            config->item = anyka_config_insert_item(config, item);
        }
        else
        {			
            continue;
        }
    }        
    free(file_buf);
    fclose(file_id);    

	if(head == NULL)
	{		
		head = anyka_config_insert_null_title();
	}

    return head;

anyka_config_init_err_tbl:
    if(file_buf)
    {
        free(file_buf);
    }
    if(config)
    {
        free(config);
    }
    if(file_id >= 0)
    {
        fclose(file_id);
    }
    
    while(head)
    {
        item = head->item;
        while(item)
        {
            next = item->next;
            free(item);
            item = next;
        }
        config = head->next;
        free(head);
        head = config;
    }
	
    return NULL;
}



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

int anyka_config_destroy(char *config_name, void *config_handle)
{
    int ret = 0;
    Panyka_config_title_info head, config = (Panyka_config_title_info)config_handle;
    Panyka_config_item_info item, next;

    if(config->update)
    {
        ret = anyka_config_save(config_name, config_handle);
    }
    head = config;
    while(head)
    {
        item = head->item;
        while(item)
        {
            next = item->next;
            free(item);
            item = next;
        }
        config = head->next;
        free(head);
        head = config;
    }

    return ret;
    
}



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

int anyka_config_set_title(void *config_handle, char *title, char *name, char *value)
{
    Panyka_config_title_info config = (Panyka_config_title_info)config_handle;
    Panyka_config_item_info item;

	config->update = 1;

    while(config)
    {
        if(strcmp(title, config->title) == 0)
        {
            item = config->item;
            while(item)
            {
                if(strcmp(name, item->name) == 0)
                {
                    strcpy(item->value, value);
                    return 0;
                }
                item = item->next;
            }
            anyka_print("[%s,%d]:config file don't find the name:%s in title:%s\n",__func__, __LINE__, name, title);
            return 1;
        }
		config = config->next;
    }
    anyka_print("[%s,%d]:config file don't find title:%s\n",__func__, __LINE__, title);
    return 1;
}


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

int anyka_config_get_title(void *config_handle, char *title, char *name, char *value, char *default_value)
{
    Panyka_config_title_info config = (Panyka_config_title_info)config_handle;
    Panyka_config_item_info item;
	
    while(config)
    {
        if(strcmp(title, config->title) == 0)
        {
            item = config->item;
            while(item)
            {
                //anyka_print("get name:%s, %s, %s\n", title, name, item->name);
                if(strcmp(name, item->name) == 0)
                {
					if(item->value)
					{
	                    strcpy(value, item->value);
	                    return 0;
					}
                }
                item = item->next;
            }

			/*didn't find any item*/
            if(item == NULL)
            {
                anyka_print("[%s,%d]:we didn't find %s, use default value.\n",__func__, __LINE__, name);
            }
            item = (Panyka_config_item_info)malloc(sizeof(anyka_config_item_info));
            if(item == NULL)
            {
                anyka_print("[%s,%d]:Fails to malloc.\n",__func__, __LINE__);
				return -1;
            }
			strcpy(item->name, name);
			strcpy(item->value, default_value);
			strcpy(value, item->value);	
			
            config->item = anyka_config_insert_item(config, item);
			config = (Panyka_config_title_info)config_handle;
			config->update = 1;
            return 1;
        }
		config = config->next;
    }
	
	/** we didn't find the title , than we create it **/	
    anyka_print("[%s,%d]:config file didn't find title : %s\n",__func__, __LINE__, title);
	Panyka_config_title_info new_title = (Panyka_config_title_info)malloc(sizeof(anyka_config_title_info));
	Panyka_config_item_info new_item = (Panyka_config_item_info)malloc(sizeof(anyka_config_item_info));

	new_title->item = NULL;
	strcpy(new_title->title, title);
	anyka_config_insert_title(config_handle, new_title);
	
	strcpy(new_item->name, name);
	strcpy(new_item->value, default_value);
	strcpy(value, new_item->value);	
	
	anyka_config_insert_item(new_title, new_item);	
	
	config = (Panyka_config_title_info)config_handle;
	config->update = 1;

	anyka_print("[%s:%s:%d] create default title:%s and item:%s, value :%s success.\n", __func__, __func__,
		__LINE__, new_title->title, new_title->item->name, new_title->item->value);
    return 1;
}



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

int anyka_config_set_item(char *config_name, char *title, char *name, char *value)
{
    int ret = 1;
    Panyka_config_title_info config;

    config = (Panyka_config_title_info)anyka_config_init(config_name);
    if(config)
    {
        ret = anyka_config_set_title(config, title, name, value);

        if(anyka_config_destroy(config_name, config))
        {
            ret = 1;
        }
    }
    return ret;
}


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

int anyka_config_get_item(char *config_name, char *title, char *name, char *value, char *default_value)
{
    int ret = 1;
    Panyka_config_title_info config;

    config = (Panyka_config_title_info)anyka_config_init(config_name);
    if(config)
    {
        ret = anyka_config_get_title(config, title, name, value, default_value);

        if(anyka_config_destroy(config_name, config))
        {
            ret = 1;
        }
    }
    return ret;
}
/**
* @brief  保存配置文件
* 
* @author aijun
* @date 2014-11-28
* @param[in]  void *config_name  配置文件名
                     void *config_handle 配置文件句柄
* @return int
* @retval if return 0 success, otherwise failed 
*/

int anyka_config_save(char *config_name, void *config_handle)
{    
    int ret = 0;
    Panyka_config_title_info config = (Panyka_config_title_info)config_handle;
    Panyka_config_item_info item;

    if(config->update)
    {
        FILE *file_id = fopen(config_name, "w");
        char *file_buf;
        
        if(file_id == NULL)
        {
            anyka_print("[%s,%d]:config file don't open:%s\n",__func__, __LINE__, config_name);
            return 1;
        }

        file_buf = (char *)malloc(512);
        if(file_buf == NULL)
        {
            anyka_print("[%s,%d]:it fails to malloc!\n",__func__, __LINE__);
            fclose(file_id);
            return 1;
        }
        
        
        while(config)
        {
            sprintf(file_buf,"\n[%s]\n", config->title);
            fputs(file_buf, file_id);
            item = config->item;
            while(item)
            {
                sprintf(file_buf,"%s\t\t\t= %s\n", item->name, item->value);
                fputs(file_buf, file_id);
                item = item->next;
            }
			config = config->next;
        }        
        free(file_buf);
        fclose(file_id);        
    }

    return ret;
}

