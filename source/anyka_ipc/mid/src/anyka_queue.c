
#include "common.h"
#include "anyka_queue.h"



struct _anyka_queue_note_info;

typedef struct _anyka_queue_note_info
{
    void *data;
    struct _anyka_queue_note_info *next;
}anyka_queue_note_info, *p_anyka_queue_note_info;

typedef struct _anyka_queue_con_info
{
    unsigned short total_item;      //队列中有的当前保存的数据项
    unsigned short max_item;        //队列最大储存项
    pthread_mutex_t queue_mutex;
    p_anyka_queue_note_info head;
    p_anyka_queue_note_info tail;
}anyka_queue_con_info, *p_anyka_queue_con_info;




/**
 * NAME         anyka_queue_init
 * @BRIEF	队列初始化
 * @PARAM	max_item : 队列中最大储存项
 
 * @RETURN	队列句柄
 * @RETVAL	
 */



void * anyka_queue_init(int max_item)
{
    p_anyka_queue_con_info new_queue;

    new_queue = malloc(sizeof(anyka_queue_con_info));
    if(NULL == new_queue)
    {
        anyka_print("[%s:%d] fails to malloc!\n", __func__, __LINE__);
        return NULL;
    }
    new_queue->head = NULL;
    new_queue->tail = NULL;
    new_queue->total_item = 0;
    new_queue->max_item = max_item;
    pthread_mutex_init(&new_queue->queue_mutex, NULL);
    //anyka_print("[anyka_queue_init:], the queue has been created!(max item:%d)\n", max_item);
    return new_queue;
}



/**
 * NAME         anyka_queue_destroy
 * @BRIEF	队列释放，将释放队列中所有未处理的数据项，只释放相应内存
                    系统将丢失所有数据
 * @PARAM	queue_handle : 队列句柄
                    queue_free     :队列释放数据项的内存回调
 
 * @RETURN	NONE
 * @RETVAL	
 */

void anyka_queue_destroy(void * queue_handle, anyka_queue_free *queue_free)
{
    p_anyka_queue_con_info queue_info = (p_anyka_queue_con_info)queue_handle;
    p_anyka_queue_note_info head, next;

    if(queue_handle == NULL)
    {
        return;
    }
    pthread_mutex_lock(&queue_info->queue_mutex);
    head = queue_info->head;
    while(head)
    {
        next = head->next;
        queue_free(head->data);
        free(head);
        head = next;        
    }
    anyka_pthread_mutex_destroy(&queue_info->queue_mutex);
    
    free(queue_info);
    //anyka_print("[anyka_queue_destroy:], the queue has been destroy!\n");
    
}


/**
 * NAME         anyka_queue_get_tail_note
 * @BRIEF	返回队列尾的数据项，并不解除在队列的位置
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	队列尾数据项
 * @RETVAL	
 */

void* anyka_queue_get_tail_note(void * queue_handle)
{
    p_anyka_queue_con_info queue_info = (p_anyka_queue_con_info)queue_handle;
    
    if(queue_handle == NULL)
    {
        return NULL;
    }
    return queue_info->tail->data;
}

/**
 * NAME         anyka_queue_push
 * @BRIEF	将用户数据放入队列中
 * @PARAM	queue_handle : 队列句柄
                    data  用户数据指针
 * @RETURN	队列尾数据项
 * @RETVAL	0-->fail; 1-->ok
 */

uint8 anyka_queue_push(void * queue_handle, void *data)
{
    
    p_anyka_queue_con_info queue_info = (p_anyka_queue_con_info)queue_handle;

    p_anyka_queue_note_info new_note;

    
    if(queue_handle == NULL)
    {
        return 0;
    }
    
    pthread_mutex_lock(&queue_info->queue_mutex);

    if(queue_info->total_item >= queue_info->max_item)
    {
        anyka_debug("[%s] the queue items is too many(max:%d)!\n", __func__, queue_info->max_item);
        
        pthread_mutex_unlock(&queue_info->queue_mutex);
        return 0;
    }
    queue_info->total_item ++;
    new_note= malloc(sizeof(anyka_queue_note_info));
    if(new_note == NULL)
    {
        anyka_print("[%s] it fails to malloc!\n", __func__);
        pthread_mutex_unlock(&queue_info->queue_mutex);
        return 0;
    }
    new_note->data = data;
    new_note->next = NULL;

    if(queue_info->tail)
    {
        queue_info->tail->next = new_note;
        queue_info->tail = new_note;
    }
    else
    {
        queue_info->tail = new_note;        
        queue_info->head = new_note;
    } 

    pthread_mutex_unlock(&queue_info->queue_mutex);
    return 1;
    
}


/**
 * NAME         anyka_queue_pop
 * @BRIEF	将用户数据返回给用户，队列中弹出此数据项
 * @PARAM	queue_handle : 队列句柄
                    
 * @RETURN	返回队列头数据
 * @RETVAL	
 */

void * anyka_queue_pop(void * queue_handle)
{
    p_anyka_queue_con_info queue_info = (p_anyka_queue_con_info)queue_handle;
    void *data;
    p_anyka_queue_note_info head;

    
    if(queue_handle == NULL)
    {
        return NULL;
    }
    

    pthread_mutex_lock(&queue_info->queue_mutex);
    if(NULL == queue_info->head)
    {        
        pthread_mutex_unlock(&queue_info->queue_mutex);
        return NULL;
    }

    queue_info->total_item --;
    
    data = queue_info->head->data;
    head = queue_info->head;
    queue_info->head = queue_info->head->next;

    if(queue_info->head == NULL)
    {
        queue_info->head = NULL;
        queue_info->tail = NULL;
    }


    pthread_mutex_unlock(&queue_info->queue_mutex);

    free(head);
    return data;

}


/**
 * NAME         anyka_queue_sort
 * @BRIEF	对队列中的数据项进行排序
 * @PARAM	queue_handle : 队列句柄
                    compare         :数据项比较回调函数
 * @RETURN	NONE
 * @RETVAL	
 */

void anyka_queue_sort(void * queue_handle, anyka_queue_compare *compare)
{
    int i, total = 0;
    p_anyka_queue_con_info queue_info = (p_anyka_queue_con_info)queue_handle;
    p_anyka_queue_note_info *sorted_queue, head;

    if(queue_handle == NULL)
    {
        return ;
    }
    
    if(queue_info->total_item == 0)
    {
        return;
    }
    
    sorted_queue = malloc(sizeof(p_anyka_queue_note_info) * queue_info->total_item);
    if(sorted_queue == NULL)
    {
        return;
    }
    
    memset(sorted_queue, 0, sizeof(p_anyka_queue_note_info) * queue_info->total_item);
    pthread_mutex_lock(&queue_info->queue_mutex);
    head = queue_info->head; 
    while(head)
    {
        for(i = 0; i < total; i++)
        {
            if(compare(head->data, sorted_queue[i]->data) < 0)
            {
                memmove((void *)((uint32)sorted_queue + (i + 1)* sizeof(p_anyka_queue_note_info)), 
                    (void *)((uint32)sorted_queue + i * sizeof(p_anyka_queue_note_info)), (total - i) * sizeof(p_anyka_queue_note_info));
                break;
            }
        }
        sorted_queue[i] = head;
        total ++;
        head = head->next;
    }
    queue_info->head = sorted_queue[0];
    queue_info->tail = sorted_queue[queue_info->total_item-1];
    queue_info->tail->next = NULL;
    for(i = 1; i < queue_info->total_item; i ++)
    {
        sorted_queue[i-1]->next = sorted_queue[i];
    }
    free(sorted_queue);
    pthread_mutex_unlock(&queue_info->queue_mutex);
}



/**
 * NAME         anyka_queue_not_empty
 * @BRIEF	判断队列是否为空
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	0-->empty; 1-->have
 * @RETVAL	
 */

uint8 anyka_queue_not_empty(void * queue_handle)
{
    p_anyka_queue_con_info queue_info = (p_anyka_queue_con_info)queue_handle;
    uint8 ret; 
    
    if(queue_handle == NULL)
    {
        return 0;
    }

    pthread_mutex_lock(&queue_info->queue_mutex);
    ret = queue_info && queue_info->head;
    pthread_mutex_unlock(&queue_info->queue_mutex);

    return ret;
}


/**
 * NAME         anyka_queue_is_full
 * @BRIEF	判断队列是否为满
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	0-->not full; 1-->full
 * @RETVAL	
 */

uint16 anyka_queue_is_full(void * queue_handle)
{
    p_anyka_queue_con_info queue_info = (p_anyka_queue_con_info)queue_handle;
    uint8 ret; 
    
    if(queue_handle == NULL)
    {
        return 0;
    }

    pthread_mutex_lock(&queue_info->queue_mutex);
    ret = (queue_info->total_item + 1 >= queue_info->max_item);
    pthread_mutex_unlock(&queue_info->queue_mutex);

    return ret;
}


/**
 * NAME         anyka_queue_is_full
 * @BRIEF	返回队列数据项数目
 * @PARAM	queue_handle : 队列句柄
 
 * @RETURN	数据项数目
 * @RETVAL	
 */

uint16 anyka_queue_items(void * queue_handle)
{
    p_anyka_queue_con_info queue_info = (p_anyka_queue_con_info)queue_handle;
    uint16 ret; 
    
    if(queue_handle == NULL)
    {
        return 0;
    }

    pthread_mutex_lock(&queue_info->queue_mutex);
    ret = queue_info->total_item;
    pthread_mutex_unlock(&queue_info->queue_mutex);

    return ret;
}



/**
 * NAME         anyka_queue_is_full
 * @BRIEF	返回指定数据项数据
 * @PARAM	queue_handle : 队列句柄
                    index              :数据项序号
 * @RETURN	用户数据项
 * @RETVAL	
 */

void* anyka_queue_get_index_item(void * queue_handle, int index)
{
    p_anyka_queue_con_info queue_info = (p_anyka_queue_con_info)queue_handle;
    p_anyka_queue_note_info head;
    
    if(queue_handle == NULL)
    {
        return NULL;
    }

   
   	pthread_mutex_lock(&queue_info->queue_mutex);

	if(index >= queue_info->total_item)
    {
		pthread_mutex_unlock(&queue_info->queue_mutex);
        return NULL;
    }
	
    head = queue_info->head;
	if(head == NULL){
		pthread_mutex_unlock(&queue_info->queue_mutex);
		return NULL;
	}
		
    while(index && head)
    {
        head = head->next;
        index --;
    }
	
    pthread_mutex_unlock(&queue_info->queue_mutex);
	
	if(head)
	{
    	return head->data;
	}
	else
	{
		return NULL;
	}
}



