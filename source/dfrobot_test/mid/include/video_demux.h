#ifndef _video_demux_h_
#define _video_demux_h_


/**
 * NAME         video_demux_open
 * @BRIEF	open the dumxer
                  
 * @PARAM	char *file_name, 文件名
 			int start，开始标志
                    
 * @RETURN	void 
 * @RETVAL	
 */

void * video_demux_open(char *file_name, int start);

/**
 * NAME        video_demux_get_data
 * @BRIEF	get date after demux
                  
 * @PARAM	void* handle, handle return by open
 			int* ntype, demux types
 * @RETURN	T_STM_STRUCT * 
 * @RETVAL	return the stream after demux
 */

T_STM_STRUCT * video_demux_get_data(void* handle, int* ntype);

/**
 * NAME        video_demux_close
 * @BRIEF	close demux
                  
 * @PARAM	void* handle, handle return by open
 * @RETURN	void 
 * @RETVAL	
 */

void video_demux_close(void* handle);

/**
 * NAME        video_demux_get_total_time
 * @BRIEF	get total time
                  
 * @PARAM	char *file_name, file name which you want to get its times
 		
 * @RETURN	int 
 * @RETVAL	file times
 */

int video_demux_get_total_time(char *file_name);

/**
 * NAME        video_demux_free_data
 * @BRIEF	free the dumux resource
                  
 * @PARAM	T_STM_STRUCT *pstream, resource which you want to free
 * @RETURN	void 
 * @RETVAL	
 */

void video_demux_free_data(T_STM_STRUCT *pstream);


#endif

