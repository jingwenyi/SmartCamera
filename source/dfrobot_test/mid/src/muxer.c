#include "common.h"
#include "muxer.h"
#include "anyka_fs.h"


typedef struct _anyka_muxer_handle
{
    long index_fd;
    long record_fd;
    T_VOID *midea_handle;
    pthread_mutex_t     muxer_lock;
}anyka_muxer_handle, *Panyka_muxer_handle;



extern video_setting *pvideo_default_config;


/**
 * NAME        mux_open
 * @BRIEF	open mux lib
                  
 * @PARAM	T_MUX_INPUT *mux_input,  set mux lib attribute
 			int record_fd, save file descriptor
 			int index_fd, tmp file descriptor
 * @RETURN	void * 
 * @RETVAL	mux handle
 */

void* mux_open(T_MUX_INPUT *mux_input, int record_fd, int index_fd)
{
    video_setting *pvideo_default_config;
    Panyka_muxer_handle panyka_muxer;
    T_MEDIALIB_MUX_OPEN_INPUT mux_open_input;
    T_MEDIALIB_MUX_OPEN_OUTPUT mux_open_output;
    T_MEDIALIB_MUX_INFO MuxInfo;


    pvideo_default_config = anyka_get_sys_video_setting();
    panyka_muxer = (Panyka_muxer_handle)malloc(sizeof(anyka_muxer_handle));
    if(NULL == panyka_muxer)
    {
        return NULL;            
    }
    memset(panyka_muxer, 0, sizeof(anyka_muxer_handle));
    panyka_muxer->record_fd = record_fd; //Get the file handle and file name
    
    panyka_muxer->index_fd = index_fd;
    
    pthread_mutex_init(&panyka_muxer->muxer_lock, NULL); 

    anyka_fs_reset();
    memset(&mux_open_input, 0, sizeof(T_MEDIALIB_MUX_OPEN_INPUT));
    mux_open_input.m_MediaRecType   = mux_input->m_MediaRecType;//MEDIALIB_REC_AVI_NORMAL;
    mux_open_input.m_hMediaDest     = (T_S32)panyka_muxer->record_fd;
    mux_open_input.m_bCaptureAudio  = mux_input->m_bCaptureAudio;
    mux_open_input.m_bNeedSYN       = AK_TRUE;
    mux_open_input.m_bLocalMode     = AK_TRUE;
    mux_open_input.m_bIdxInMem      = AK_FALSE;
    mux_open_input.m_ulIndexMemSize = 0;
    mux_open_input.m_hIndexFile     = (T_S32)panyka_muxer->index_fd;

    //for syn
    mux_open_input.m_ulVFifoSize    = 200*1024; //video fifo size
    mux_open_input.m_ulAFifoSize    = 100*1024; //audio fifo size
    mux_open_input.m_ulTimeScale    = 1000;     //time scale

    // set video open info
    mux_open_input.m_eVideoType     = mux_input->m_eVideoType;//MEDIALIB_VIDEO_H264;
    mux_open_input.m_nWidth         = mux_input->m_nWidth;//640;
    mux_open_input.m_nHeight        = mux_input->m_nHeight;//480;
    mux_open_input.m_nFPS           = pvideo_default_config->savefilefps + 0; //考虑到编码帧率可能有时会高于合成帧率，此处多增加几个空帧，避免出现录像时间不准的情况。
    mux_open_input.m_nKeyframeInterval  = pvideo_default_config->savefilefps * pvideo_default_config->gopLen-1;
    
    // set audio open info
    if(mux_input->m_bCaptureAudio == 1)
    {
    mux_open_input.m_eAudioType         = mux_input->m_eAudioType;//MEDIALIB_AUDIO_PCM;
    mux_open_input.m_nSampleRate        = mux_input->m_nSampleRate;//8000;
    mux_open_input.m_nChannels          = 1;
    mux_open_input.m_wBitsPerSample     = 16;
    //audio type
    switch (mux_input->m_eAudioType)
    {
        case MEDIALIB_AUDIO_PCM:
        {
            mux_open_input.m_ulSamplesPerPack = mux_input->m_nSampleRate*32/1000;
            mux_open_input.m_ulAudioBitrate = mux_input->m_nSampleRate*mux_open_input.m_wBitsPerSample
                *mux_open_input.m_ulSamplesPerPack;
            break;
        }
        case MEDIALIB_AUDIO_G711:
        {
            mux_open_input.m_wFormatTag = 0x6;          
            mux_open_input.m_nBlockAlign = 1;
            mux_open_input.m_cbSize = 0;
            mux_open_input.m_wBitsPerSample = 8;
            mux_open_input.m_ulSamplesPerPack = 256;//mux_input->m_nSampleRate*32/1000;
            mux_open_input.m_ulAudioBitrate = mux_input->m_nSampleRate*mux_open_input.m_wBitsPerSample
                *mux_open_input.m_ulSamplesPerPack;
            mux_open_input.m_nAvgBytesPerSec = mux_open_input.m_nSampleRate
                        * mux_open_input.m_nBlockAlign;
                    //  / mux_open_input.m_ulSamplesPerPack;
            break;
        }
        case MEDIALIB_AUDIO_AAC:
        {
            mux_open_input.m_ulSamplesPerPack = 1024*mux_open_input.m_nChannels;
            mux_open_input.m_cbSize = 2;
            switch(mux_open_input.m_nSampleRate)
            {
                case 8000 :
                    mux_open_input.m_ulAudioBitrate = 8000;
                    break;
                case 11025 :
                    mux_open_input.m_ulAudioBitrate = 11025;
                    break;
                case 12000 :
                    mux_open_input.m_ulAudioBitrate = 12000;
                    break;
            
                case 16000:
                    mux_open_input.m_ulAudioBitrate = 16000;
                    break;
                case 22050:
                    mux_open_input.m_ulAudioBitrate = 22050;
                    break;
                case 24000:
                    mux_open_input.m_ulAudioBitrate = 24000;
                    break;
                case 32000:
                    mux_open_input.m_ulAudioBitrate = 32000;
                    break;
                case 44100:
                    mux_open_input.m_ulAudioBitrate = 44100;
                    break;
                case 48000:
                    mux_open_input.m_ulAudioBitrate = 48000;
                    break;
                default:
                    mux_open_input.m_ulAudioBitrate = 48000;
                    break;
            }
        
            break;
        }
        case MEDIALIB_AUDIO_ADPCM:
        {
            mux_open_input.m_wFormatTag = 0x11;
            mux_open_input.m_wBitsPerSample = 4;
            switch(mux_open_input.m_nSampleRate)
            {
                case 8000:
                case 11025:
                case 12000:
                case 16000:
                    mux_open_input.m_nBlockAlign = 0x100;
                    break;
                case 22050:
                case 24000:
                case 32000:
                    mux_open_input.m_nBlockAlign = 0x200;
                    break;
                case 44100:
                case 48000:
                case 64000:
                    mux_open_input.m_nBlockAlign = 0x400;
                    break;
                default:
                    mux_open_input.m_nBlockAlign = 0x400;
                    break;
            }
            
            mux_open_input.m_ulSamplesPerPack =
                (mux_open_input.m_nBlockAlign-4)*8/4+1;
            
            mux_open_input.m_ulAudioBitrate = 
            mux_open_input.m_nAvgBytesPerSec = mux_open_input.m_nSampleRate
                        * mux_open_input.m_nBlockAlign
                        / mux_open_input.m_ulSamplesPerPack;

            mux_open_input.m_nBlockAlign *= mux_open_input.m_nChannels;
            mux_open_input.m_cbSize = 2;
            mux_open_input.m_pszData = (T_U8 *)&mux_open_input.m_ulSamplesPerPack;
            break;
        }
    	case MEDIALIB_AUDIO_AMR:
		{
			mux_open_input.m_ulAudioBitrate = 12200;
			mux_open_input.m_ulSamplesPerPack = 160;

			break;
		}
		default:
        goto err;
            
    }
    }
    mux_open_input.m_CBFunc.m_FunPrintf= (MEDIALIB_CALLBACK_FUN_PRINTF)anyka_print;
    mux_open_input.m_CBFunc.m_FunMalloc= (MEDIALIB_CALLBACK_FUN_MALLOC)malloc;
    mux_open_input.m_CBFunc.m_FunFree = (MEDIALIB_CALLBACK_FUN_FREE)free;
    mux_open_input.m_CBFunc.m_FunRead= (MEDIALIB_CALLBACK_FUN_READ)anyka_fs_read;
    mux_open_input.m_CBFunc.m_FunSeek= (MEDIALIB_CALLBACK_FUN_SEEK)anyka_fs_seek;
    mux_open_input.m_CBFunc.m_FunTell = (MEDIALIB_CALLBACK_FUN_TELL)anyka_fs_tell;
    mux_open_input.m_CBFunc.m_FunWrite = (MEDIALIB_CALLBACK_FUN_WRITE)anyka_fs_write;
    mux_open_input.m_CBFunc.m_FunFileHandleExist = anyka_fs_isexist;

    panyka_muxer->midea_handle = MediaLib_Mux_Open(&mux_open_input, &mux_open_output);
    if (AK_NULL == panyka_muxer->midea_handle)
    {
		anyka_print("[%s:%d] open mux handle failed!\n", __func__, __LINE__);
        goto err;
    }

    if (MediaLib_Mux_GetInfo(panyka_muxer->midea_handle, &MuxInfo) == AK_FALSE)
    {
		anyka_print("[%s:%d] mux get info failed!\n", __func__, __LINE__);
        goto err;
    }

    if (AK_FALSE == MediaLib_Mux_Start(panyka_muxer->midea_handle))
    {
		anyka_print("[%s:%d] mux start failed!\n", __func__, __LINE__);
        goto err;
    }

    anyka_fs_insert_file(record_fd);
    anyka_fs_insert_file(index_fd);


    return (void *)panyka_muxer;


err:
    anyka_print("[%s:%d] it fail to open the handle!\n", __func__, __LINE__);
    anyka_fs_flush(panyka_muxer->record_fd);
    anyka_fs_remove_file(panyka_muxer->record_fd);
    if(panyka_muxer->midea_handle != AK_NULL)
    {
        MediaLib_Mux_Close(panyka_muxer->midea_handle);
    }
    if(panyka_muxer)
    {
        anyka_pthread_mutex_destroy(&panyka_muxer->muxer_lock);
        free(panyka_muxer);
    }
    panyka_muxer = NULL;
    return NULL;
    
}

/**
 * NAME        mux_addAudio
 * @BRIEF	add audio to mux 
                  
 * @PARAM	void *handle,  handle when open 
 			void *pbuf, 	pointer to data
 			unsigned long size, data size
 			unsigned long timestamp, current time stamps
 * @RETURN	int
 * @RETVAL	if return 0 success, otherwise failed
 */

int mux_addAudio(void *handle, void *pbuf, unsigned long size, unsigned long timestamp)
{
    Panyka_muxer_handle panyka_muxer = (Panyka_muxer_handle)handle;
    
    if(NULL == panyka_muxer)
    {
        return -1;
    }
    
    T_MEDIALIB_MUX_PARAM mux_param;
    T_eMEDIALIB_MUX_STATUS mux_status;
    
    mux_param.m_pStreamBuf = pbuf;
    mux_param.m_ulStreamLen = size;
    mux_param.m_ulTimeStamp = timestamp;
    mux_param.m_bIsKeyframe = AK_TRUE;
	pthread_mutex_lock(&panyka_muxer->muxer_lock);
    //if (audio_tytes ! = 0)
    {
        if(!MediaLib_Mux_AddAudioData(panyka_muxer->midea_handle, &mux_param))
        {
            anyka_print("WARNING! Add Audio Failure\r\n");
            //Mutex_Unlock(&muxMutex);
            //return -1;
        }
        else
        {
            mux_status = MediaLib_Mux_Handle(panyka_muxer->midea_handle);
        }
    }
    
    mux_status = MediaLib_Mux_GetStatus(panyka_muxer->midea_handle);
	pthread_mutex_unlock(&panyka_muxer->muxer_lock);
    if ( MEDIALIB_MUX_SYSERR == mux_status || MEDIALIB_MUX_MEMFULL == mux_status )
    {
        anyka_print("mux_addAudio: it fail to muxer with err(%d)\n", mux_status);
        return -1;
    }
    return 0;
}



/**
 * NAME        mux_addAudio
 * @BRIEF	add video to mux 
                  
 * @PARAM	void *handle,  handle when open 
 			void *pbuf, 	pointer to data
 			unsigned long size, data size
 			unsigned long timestamp, current time stamps
 			int nIsIFrame, the number of I frame
 * @RETURN	int
 * @RETVAL	if return 0 success, otherwise failed
 */

int mux_addVideo(void *handle, void *pbuf, unsigned long size, unsigned long timestamp, int nIsIFrame)
{
    int ret=0;
    
    Panyka_muxer_handle panyka_muxer = (Panyka_muxer_handle)handle;
    if(NULL == panyka_muxer)
    {
        return -1;
    }
    T_eMEDIALIB_MUX_STATUS mux_status;
    
	T_MEDIALIB_MUX_PARAM mux_param;

	mux_param.m_pStreamBuf = pbuf;
	mux_param.m_ulStreamLen = size;
	mux_param.m_ulTimeStamp = timestamp;
	mux_param.m_bIsKeyframe = nIsIFrame;


	pthread_mutex_lock(&panyka_muxer->muxer_lock);

	
	if (!MediaLib_Mux_AddVideoData(panyka_muxer->midea_handle, &mux_param))
	{
		anyka_print("WARNING! Add Video Failure! ts: %lu IF: %d, sz:%lu\n", timestamp, nIsIFrame, size);
        ret = -1;
	}
	else
	{
		mux_status = MediaLib_Mux_Handle(panyka_muxer->midea_handle);
	}
	
	
	mux_status = MediaLib_Mux_GetStatus(panyka_muxer->midea_handle);
	pthread_mutex_unlock(&panyka_muxer->muxer_lock);
	if (MEDIALIB_MUX_SYSERR == mux_status || MEDIALIB_MUX_MEMFULL == mux_status)
	{
        anyka_print("mux_addVideo: it fail to muxer with err(%d)\n", mux_status);
		ret = -1;
	}
	return ret;
}

/**
 * NAME        mux_addAudio
 * @BRIEF	close mux 
                  
 * @PARAM	void *handle,  handle when open 

 * @RETURN	int
 * @RETVAL	if return 0 success, otherwise failed
 */

int mux_close(void *handle)
{
    
    Panyka_muxer_handle panyka_muxer = (Panyka_muxer_handle)handle;
    if(NULL == panyka_muxer)
    {
        return 0;
    }
	
	pthread_mutex_lock(&panyka_muxer->muxer_lock);
    anyka_fs_remove_file(panyka_muxer->record_fd);
    anyka_fs_remove_file(panyka_muxer->index_fd);
    anyka_fs_flush(panyka_muxer->record_fd);
    anyka_fs_flush(panyka_muxer->index_fd);
    MediaLib_Mux_Stop(panyka_muxer->midea_handle);

    MediaLib_Mux_Close(panyka_muxer->midea_handle);

    anyka_pthread_mutex_destroy(&panyka_muxer->muxer_lock);

    free(panyka_muxer);
    panyka_muxer = NULL;

	return 0;
}


