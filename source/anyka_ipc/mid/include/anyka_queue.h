#ifndef _ANYKA_QUEUE_H_
#define _ANYKA_QUEUE_H_
#include "basetype.h"

typedef void anyka_queue_free(void *item);
typedef int anyka_queue_compare(void *item1, void *item2);

/**
 * NAME         anyka_queue_init
 * @BRIEF	队列初始化
 * @PARAM	max_item : 队列中最大储存项
 
 * @RETURN	队列句柄
 * @RETVAL	
 */

void * anyka_queue_init(int max_item);


/**
 * NAME         anyka_queue_destroy
 * @BRIEF	队列释放，将释放队列中所有未处理的数据项，只释放相应内存
                    系统将丢失所有数据
 * @PARAM	queue_handle : 队列句柄
                    queue_free     :队列释放数据项的内存回调
 
 * @RETURN	NONE
 * @RETVAL	
 */

void anyka_queue_destroy(void * queue_handle, anyka_queue_free *queue_free);


/**
 * NAME         anyka_queue_push
 * @BRIEF	将用户数据放入队列中
 * @PARAM	queue_handle : 队列句柄
                    data  用户数据指针
 * @RETURN	队列尾数据项
 * @RETVAL	0-->fail; 1-->ok
 */

uint8 anyka_queue_push(void * queue_handle, void *data);

/**
 * NAME         anyka_queue_pop
 * @BRIEF	将用户数据返回给用户，队列中弹出此数据项
 * @PARAM	queue_handle : 队列句柄
                    
 * @RETURN	返回队列头数据
 * @RETVAL	
 */

void * anyka_queue_pop(void * queue_handle);

/**
 * NAME         anyka_queue_sort
 * @BRIEF	对队列中的数据项进行排序
 * @PARAM	queue_handle : 队列句柄
                    compare         :数据项比较回调函数
 * @RETURN	NONE
 * @RETVAL	
 */

void anyka_queue_sort(void * queue_handle, anyka_queue_compare *compare);

/**
 * NAME         anyka_queue_not_empty
 * @BRIEF	判断队列是否为空
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	0-->empty; 1-->have
 * @RETVAL	
 */

uint8 anyka_queue_not_empty(void * queue_handle);

/**
 * NAME         anyka_queue_is_full
 * @BRIEF	判断队列是否为满
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	0-->not full; 1-->full
 * @RETVAL	
 */

uint16 anyka_queue_is_full(void * queue_handle);


/**
 * NAME         anyka_queue_get_tail_note
 * @BRIEF	返回队列尾的数据项，并不解除在队列的位置
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	队列尾数据项
 * @RETVAL	
 */

void* anyka_queue_get_tail_note(void * queue_handle);

/**
 * NAME         anyka_queue_is_full
 * @BRIEF	返回队列数据项数目
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	数据项数目
 * @RETVAL	
 */

uint16 anyka_queue_items(void * queue_handle);


/**
 * NAME         anyka_queue_is_full
 * @BRIEF	返回指定数据项数据
 * @PARAM	queue_handle : 队列句柄
                    index              :数据项序号
 * @RETURN	用户数据项
 * @RETVAL	
 */

void* anyka_queue_get_index_item(void * queue_handle, int index);


#endif

