#ifndef _anyka_fs_h_
#define _anyka_fs_h_


/**
 * NAME         anyka_fs_init
 * @BRIEF	创建异步写的句柄
 * @PARAM	void
**/                   
    
void anyka_fs_init(void);


/**
 * NAME         anyka_fs_reset
 * @BRIEF	创建异步写的句柄
 * @PARAM	void
**/                   

void anyka_fs_reset(void);



/**
 * NAME         anyka_fs_flush
 * @BRIEF	结束异步写,并将对应的文件ID的异步写数据全部写完
 * @PARAM	file_id   文件ID
                   
 
 * @RETURN	void 
 * @RETVAL	
 */

void anyka_fs_flush(int file_id);
/**
 * NAME         anyka_fs_read
 * @BRIEF	读数据，目前没有需要进行异步方式 ，所以直接读
 * @PARAM	hFile
                    buf
                    size                   
 
 * @RETURN	void 
 * @RETVAL	
 */

T_S32 anyka_fs_read(T_S32 hFile, T_pVOID buf, T_S32 size);
/**
 * NAME         anyka_fs_write
 * @BRIEF	异步写数据，如果不在异步写队列里，直接写
 * @PARAM	hFile
                    buf
                    size                   
 
 * @RETURN	void 
 * @RETVAL	
 */

T_S32 anyka_fs_write(T_S32 hFile, T_pVOID buf, T_S32 size);
/**
 * NAME         anyka_fs_seek
 * @BRIEF	SEEK
 * @PARAM	hFile
                    offset
                    whence                   
 
 * @RETURN	void 
 * @RETVAL	
 */

T_S32 anyka_fs_seek(T_S32 hFile, T_S32 offset, T_S32 whence);
/**
 * NAME         anyka_fs_tell
 * @BRIEF	得到当前文件的大小
 * @PARAM	hFile                
 
 * @RETURN	void 
 * @RETVAL	
 */
T_S32 anyka_fs_tell(T_S32 hFile);
/**
 * NAME         anyka_fs_isexist
 * @BRIEF	文件是否存在
 * @PARAM	hFile                
 
 * @RETURN	void 
 * @RETVAL	
 */

T_S32 anyka_fs_isexist(T_S32 hFile);
/**
 * NAME         anyka_fs_remove_file
 * @BRIEF	将文件移出异步写队列中
 * @PARAM	file_id   文件ID
                   
 
 * @RETURN	void 
 * @RETVAL	
 */

void anyka_fs_remove_file(int file_id);

/**
 * NAME         anyka_fs_insert_file
 * @BRIEF	将一个文件插入到异步写队列中
 * @PARAM	file_id   文件ID
                   
 
 * @RETURN	void 
 * @RETVAL	
 */

void anyka_fs_insert_file(int file_id);

#endif


