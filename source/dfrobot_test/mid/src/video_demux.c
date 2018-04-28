#include "common.h"
#include "video_stream_lib.h"
#include "akuio.h"
#include "video_demux.h"
#include "media_demuxer_lib.h"



typedef struct _VIDEO_DEMUX_INFO
{
    int file_id;
    void *handle;    
}video_demux_info, *Pvideo_demux_info;

#define RET_OK 0
#define RET_ERROR -1
#define RET_NO_BUFFER -2
#define RET_END 1


/*
 * 读取数据回调函数
 * 参数：
 * hFile:指向的文件指针
 * buf:  放置存储的缓冲区
 * size: 读取的数据大小
 * 返回值：
 * 实际读取到的数据大小
 */
static T_S32 video_demux_fread(T_S32 hFile, T_pVOID buf, T_S32 size)
{
	return read( hFile, buf, size);
}
/*
 * 写数据回调函数
 * 参数：
 * hFile:指向的文件指针
 * buf:  放置存储的缓冲区
 * size: 写入的数据大小
 * 返回值：
 * 总是返回-1
 */
static T_S32 video_demux_fwrite(T_S32 hFile, T_pVOID buf, T_S32 size)
{
    return -1;
}


/*
 * 写数据回调函数
 * 参数：
 * hFile:指向的文件指针
 * buf:  放置存储的缓冲区
 * size: 写入的数据大小
 * 返回值：
 * 实际写入的数据大小
 */
static T_S32 video_demux_fseek(T_S32 hFile, T_S32 offset, T_S32 whence)
{
    return lseek(hFile, offset, whence);
}

/*
 * 获取文件的大小
 * 参数：
 * hFile:文件句柄
 * 返回值：
 * 文件的大小
 */
static T_S32 video_demux_ftell(T_S32 hFile)
{
    
    return lseek(hFile, 0, SEEK_CUR);
}

/*
 * 判断文件是否存在
 * 参数：
 * hFile:文件句柄
 * 返回值：
 * 存在返回1，不存在返回0
 */
static T_S32 video_demux_exist(T_S32 hFile)
{
    if (hFile <= 0) {
        return 0;
    }
    
    if (lseek(hFile, 0, SEEK_CUR) < 0) {
        return 0;
    }

    return 1;
}


/**
 * NAME         video_demux_print
 * @BRIEF	解码打印
                  
 * @PARAM	可变参数
                    
 * @RETURN	void 
 * @RETVAL	
 */

T_VOID video_demux_print(char* fmt, ...)
{
}


/**
 * NAME         video_demux_open
 * @BRIEF	open the dumxer
                  
 * @PARAM	char *file_name, 文件名
 			int start，开始标志
                    
 * @RETURN	void 
 * @RETVAL	
 */

void * video_demux_open(char *file_name, int start)
{
    Pvideo_demux_info pdemux_info;
    void * hmedia = NULL;
    T_MEDIALIB_DMX_INFO mediaInfo;
    T_MEDIALIB_DMX_OPEN_INPUT open_input;
    T_MEDIALIB_DMX_OPEN_OUTPUT open_output;

    int ifile = open(file_name, O_RDONLY);
    if(ifile < 0)
    {
    	return NULL;
    }

    pdemux_info = (Pvideo_demux_info)malloc(sizeof(video_demux_info));
    if(NULL == pdemux_info)
    {
        close(ifile);
        return NULL;
    }
    pdemux_info->file_id = ifile;
    memset(&open_input, 0, sizeof(T_MEDIALIB_DMX_OPEN_INPUT));
    memset(&open_output, 0, sizeof(T_MEDIALIB_DMX_OPEN_OUTPUT));
    open_input.m_hMediaSource = (T_S32)ifile;
    open_input.m_CBFunc.m_FunPrintf = (MEDIALIB_CALLBACK_FUN_PRINTF)video_demux_print;
    open_input.m_CBFunc.m_FunRead = (MEDIALIB_CALLBACK_FUN_READ)video_demux_fread;
    open_input.m_CBFunc.m_FunWrite = (MEDIALIB_CALLBACK_FUN_WRITE)video_demux_fwrite;
    open_input.m_CBFunc.m_FunSeek = (MEDIALIB_CALLBACK_FUN_SEEK)video_demux_fseek;
    open_input.m_CBFunc.m_FunTell = (MEDIALIB_CALLBACK_FUN_TELL)video_demux_ftell;
    open_input.m_CBFunc.m_FunMalloc = (MEDIALIB_CALLBACK_FUN_MALLOC)malloc;
    open_input.m_CBFunc.m_FunFree = (MEDIALIB_CALLBACK_FUN_FREE)free;
    open_input.m_CBFunc.m_FunFileHandleExist = video_demux_exist;

    // open the dumxer
    hmedia = MediaLib_Dmx_Open(&open_input, &open_output);
    if (AK_NULL == hmedia)
    {
        close(ifile);
        free(pdemux_info);
        return NULL;
    }
    pdemux_info->handle = hmedia;
    // get media info
    memset(&mediaInfo, 0, sizeof(T_MEDIALIB_DMX_INFO));
    MediaLib_Dmx_GetInfo(hmedia, &mediaInfo);
    // release the info memory
    MediaLib_Dmx_ReleaseInfoMem(hmedia);
	
    unsigned int buffLen = MediaLib_Dmx_GetFirstVideoSize(hmedia);
    anyka_print("first video size:%u\n", (unsigned int)buffLen);	
    unsigned char* buff = (unsigned char *)malloc(buffLen);// new unsigned char[buffLen];

    if(AK_FALSE == MediaLib_Dmx_GetFirstVideo(hmedia, buff, (T_U32*)&buffLen))
    {
    	anyka_print("MediaLib_Dmx_GetFirstVideo error\n");	
    }

    free( buff);
    MediaLib_Dmx_Start(hmedia, start);
    return (void *)pdemux_info;
}

/**
 * NAME        video_demux_free_data
 * @BRIEF	free the dumux resource
                  
 * @PARAM	T_STM_STRUCT *pstream, resource which you want to free
 * @RETURN	void 
 * @RETVAL	
 */

void video_demux_free_data(T_STM_STRUCT *pstream)
{
    if(pstream)
    {
        free(pstream->buf);
        free(pstream);
    }
}

/**
 * NAME        video_demux_close
 * @BRIEF	close demux
                  
 * @PARAM	void* handle, handle return by open
 * @RETURN	void 
 * @RETVAL	
 */

void video_demux_close(void* handle)
{
    Pvideo_demux_info pdemux_info = (Pvideo_demux_info)handle;
    close(pdemux_info->file_id);    
    MediaLib_Dmx_Stop(pdemux_info->handle);
    MediaLib_Dmx_Close(pdemux_info->handle);
    free(pdemux_info);
}


/**
 * NAME        video_demux_get_data
 * @BRIEF	get date after demux
                  
 * @PARAM	void* handle, handle return by open
 			int* ntype, demux types
 * @RETURN	T_STM_STRUCT * 
 * @RETVAL	return the stream after demux
 */

T_STM_STRUCT *video_demux_get_data(void *handle, int *ntype)
{
    Pvideo_demux_info pdemux_info = (Pvideo_demux_info)handle;
    void *hMedia = pdemux_info->handle;
	T_MEDIALIB_DMX_BLKINFO dmxBlockInfo;
    T_STM_STRUCT *pstream;

    pstream = (T_STM_STRUCT *)malloc(sizeof(T_STM_STRUCT));
    if(NULL == pstream)
    {
        anyka_print("[%s,%d]it fail to malloc!\n", __func__, __LINE__);
        return NULL;
    }
    
	if (MediaLib_Dmx_GetNextBlockInfo(hMedia, &dmxBlockInfo) == AK_FALSE)
	{
        free(pstream);
        anyka_print("[%s,%d]it fail to get block info!\n", __func__, __LINE__);
		return NULL;
	}

	pstream->size = dmxBlockInfo.m_ulBlkLen;
    pstream->buf = malloc(pstream->size + 4);
    if(pstream->buf == NULL)
    {
        free(pstream);
        anyka_print("[%s,%d]it fail to malloc!\n", __func__, __LINE__);
        return NULL;
    }
    
	switch (dmxBlockInfo.m_eBlkType)
	{
        case T_eMEDIALIB_BLKTYPE_VIDEO:
           *ntype = 1;
            if (pstream->size != 0)
            {
                T_U8 * streamBuf = pstream->buf;
            	T_U8 tmpBuf[64] = {0};
            	T_U8* framehead = AK_NULL;
            	T_U8* p = AK_NULL;
            	T_U8* q = AK_NULL;
            	T_U32 m = 0;
            	T_U32 n = 0;
                
            	MediaLib_Dmx_GetVideoFrame(hMedia, (T_U8*)pstream->buf, (T_U32 *)&pstream->size);
                pstream->timestamp = *(T_U32 *)(pstream->buf + 4);
				if ((streamBuf[12]&0x1F) == 7)
				{
					framehead = tmpBuf;
					memcpy(tmpBuf, (streamBuf+8), 64);

					tmpBuf[0] &= 0x00;
					tmpBuf[1] &= 0x00;
					tmpBuf[2] &= 0x00;
					m = tmpBuf[3];
					tmpBuf[3] = (tmpBuf[3]&0x00)|0x01;
					
					p = tmpBuf+m+4;
					p[0] &= 0x00;
					p[1] &= 0x00;
					p[2] &= 0x00;
					n = p[3];
					p[3] = (p[3]&0x00)|0x01;

					q = p+n+4;
					q[0] &= 0x00;
					q[1] &= 0x00;
					q[2] &= 0x00;
					q[3] = (q[3]&0x00)|0x01;

					memcpy((streamBuf+8), framehead, 64);
                    pstream->iFrame = 1;

				}
				else
				{
					streamBuf[8] &= 0x00;
					streamBuf[9] &= 0x00;
					streamBuf[10] &= 0x00;
					streamBuf[11] = ((streamBuf[11]&0x00) | 0x01);

                    pstream->iFrame = 0;

				}
            }
            break;
            
        case T_eMEDIALIB_BLKTYPE_AUDIO:
            *ntype = 2;
            if (pstream->size != 0)
            {
				MediaLib_Dmx_GetAudioPts (hMedia , &pstream->timestamp);
            	MediaLib_Dmx_GetAudioData(hMedia, (T_U8*)pstream->buf, (T_U32)pstream->size);
            }

            if(pstream->size == 4096)
            	pstream->size = 0;
            break;
            
        default:
            anyka_print("unknow type\n");
            *ntype = 0;
            break;
	}

	T_U32 demux_status = MediaLib_Dmx_GetStatus(hMedia);
	if (demux_status == MEDIALIB_DMX_END || demux_status == MEDIALIB_DMX_ERR)
	{
        free(pstream->buf);
        free(pstream);
        anyka_print("[%s:%d]it fail to get media status!\n", __func__, __LINE__);
        return NULL;
	}	
	return pstream;
}

/**
 * NAME        video_demux_get_total_time
 * @BRIEF	get total time
                  
 * @PARAM	char *file_name, file name which you want to get its times
 		
 * @RETURN	int 
 * @RETVAL	file times
 */

int video_demux_get_total_time(char *file_name)
{
    void * hmedia = NULL;
    T_MEDIALIB_DMX_INFO mediaInfo;
    T_MEDIALIB_DMX_OPEN_INPUT open_input;
    T_MEDIALIB_DMX_OPEN_OUTPUT open_output;

    int ifile = open(file_name, O_RDONLY);
    if(ifile < 0)
    {
    	return -1;
    }

    memset(&open_input, 0, sizeof(T_MEDIALIB_DMX_OPEN_INPUT));
    memset(&open_output, 0, sizeof(T_MEDIALIB_DMX_OPEN_OUTPUT));
    open_input.m_hMediaSource = (T_S32)ifile;
    open_input.m_CBFunc.m_FunPrintf = (MEDIALIB_CALLBACK_FUN_PRINTF)video_demux_print;
    open_input.m_CBFunc.m_FunRead = (MEDIALIB_CALLBACK_FUN_READ)video_demux_fread;
    open_input.m_CBFunc.m_FunWrite = (MEDIALIB_CALLBACK_FUN_WRITE)video_demux_fwrite;
    open_input.m_CBFunc.m_FunSeek = (MEDIALIB_CALLBACK_FUN_SEEK)video_demux_fseek;
    open_input.m_CBFunc.m_FunTell = (MEDIALIB_CALLBACK_FUN_TELL)video_demux_ftell;
    open_input.m_CBFunc.m_FunMalloc = (MEDIALIB_CALLBACK_FUN_MALLOC)malloc;
    open_input.m_CBFunc.m_FunFree = (MEDIALIB_CALLBACK_FUN_FREE)free;
    open_input.m_CBFunc.m_FunFileHandleExist = video_demux_exist;

    // open the dumxer
    hmedia = MediaLib_Dmx_Open(&open_input, &open_output);
    if (AK_NULL == hmedia)
    {
        close(ifile);
        return -1;
    }
    // get media info
    memset(&mediaInfo, 0, sizeof(T_MEDIALIB_DMX_INFO));
    MediaLib_Dmx_GetInfo(hmedia, &mediaInfo);
    // release the info memory
    MediaLib_Dmx_ReleaseInfoMem(hmedia);

    close(ifile);  
    MediaLib_Dmx_Stop(hmedia);
    MediaLib_Dmx_Close(hmedia);
    return mediaInfo.m_ulTotalTime_ms;
}

