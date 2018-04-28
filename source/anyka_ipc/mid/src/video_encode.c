#include "common.h"
#include "video_stream_lib.h"
#include "akuio.h"
#include "video_encode.h"

//stream buffer size for encoder. if need better video quality,this buffer should be enlarged.
#define ENCMEM (320*1024) 

typedef struct _anyka_encode_handle
{
    int IPFrame_ctrl;
	unsigned int video_mode;
	unsigned int qp;
    T_pVOID encode_handle;
    T_pVOID poutbuf;
    T_pVOID pencbuf;
    T_VIDEOLIB_ENC_RC video_rc;
    T_eVIDEO_DRV_TYPE encode_type;
	pthread_mutex_t     lock;
	T_U32				width;
	T_U32				fps;
}anyka_encode_handle, *Panyka_encode_handle;

static T_pVOID enc_mutex_create(T_VOID)
{
	pthread_mutex_t *pMutex;
	pMutex = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(pMutex, NULL); 
	return pMutex;
}

static T_S32 enc_mutex_lock(T_pVOID pMutex, T_S32 nTimeOut)
{
	pthread_mutex_lock(pMutex);
	return 1;
}

static T_S32 enc_mutex_unlock(T_pVOID pMutex)
{
	pthread_mutex_unlock(pMutex);
	return 1;
}

static T_VOID enc_mutex_release(T_pVOID pMutex)
{
	int rc = pthread_mutex_destroy( pMutex ); 
    if ( rc == EBUSY ) {                      
        pthread_mutex_unlock( pMutex );             
        pthread_mutex_destroy( pMutex );    
    } 
	free(pMutex);
}
/*
 * @brief	media lib will call this while idle.
 * @param	ticks[in]
 * @return	T_BOOL
 * @retval	AK_TRUE for success,other for error.
 */
T_BOOL ak_enc_delay(T_U32 ticks)
{
	akuio_wait_irq();
	return AK_TRUE;
}

/**
* @brief  init video encoder
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
int video_encode_init(void)
{
	T_VIDEOLIB_CB	init_cb_fun;
	int ret;

	memset(&init_cb_fun, 0, sizeof(T_VIDEOLIB_CB));
	/* set callback */
	init_cb_fun.m_FunPrintf			= (MEDIALIB_CALLBACK_FUN_PRINTF)anyka_print;
	init_cb_fun.m_FunMalloc  		= (MEDIALIB_CALLBACK_FUN_MALLOC)malloc;
	init_cb_fun.m_FunFree    		= free;
	init_cb_fun.m_FunRtcDelay       = ak_enc_delay;
				
	init_cb_fun.m_FunDMAMalloc		= (MEDIALIB_CALLBACK_FUN_DMA_MALLOC)akuio_alloc_pmem;
  	init_cb_fun.m_FunDMAFree		= (MEDIALIB_CALLBACK_FUN_DMA_FREE)akuio_free_pmem;
  	init_cb_fun.m_FunVaddrToPaddr	= (MEDIALIB_CALLBACK_FUN_VADDR_TO_PADDR)akuio_vaddr2paddr;
  	init_cb_fun.m_FunMapAddr		= (MEDIALIB_CALLBACK_FUN_MAP_ADDR)akuio_map_regs;
  	init_cb_fun.m_FunUnmapAddr		= (MEDIALIB_CALLBACK_FUN_UNMAP_ADDR)akuio_unmap_regs;
  	init_cb_fun.m_FunRegBitsWrite	= (MEDIALIB_CALLBACK_FUN_REG_BITS_WRITE)akuio_sysreg_write;
	init_cb_fun.m_FunVideoHWLock	= (MEDIALIB_CALLBACK_FUN_VIDEO_HW_LOCK)akuio_lock_timewait;
	init_cb_fun.m_FunVideoHWUnlock	= (MEDIALIB_CALLBACK_FUN_VIDEO_HW_UNLOCK)akuio_unlock;

	//add for using api about fifo in multithread
	init_cb_fun.m_FunMutexCreate	= enc_mutex_create;
	init_cb_fun.m_FunMutexLock		= enc_mutex_lock;
	init_cb_fun.m_FunMutexUnlock	= enc_mutex_unlock;
	init_cb_fun.m_FunMutexRelease	= enc_mutex_release;

	ret = VideoStream_Enc_Init(&init_cb_fun);
	
	return 0;
}

/**
* @brief  destroy vedio encoder
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
int video_encode_destroy(void)
{
	VideoStream_Enc_Destroy();
	return 0;
}


/**
* @brief  open video encoder
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
T_VOID *video_encode_open(T_ENC_INPUT *pencInput)
{
	T_VIDEOLIB_ENC_OPEN_INPUT open_input;
    Panyka_encode_handle pencode_handle;
	T_U32 temp;

    pencode_handle = (Panyka_encode_handle)malloc(sizeof(anyka_encode_handle));
    if(NULL == pencode_handle)
    {
        anyka_print("[%s] malloc, %s\n", __func__, strerror(errno));
        return NULL;
    }
	memset(&open_input, 0, sizeof(T_VIDEOLIB_ENC_OPEN_INPUT));
	
	pencode_handle->poutbuf = akuio_alloc_pmem(ENCMEM);
	if (AK_NULL == pencode_handle->poutbuf)
	{
        anyka_print("[%s] failed, akuio_alloc_pmem!\n", __func__);
		return NULL;
	}	
	temp = akuio_vaddr2paddr(pencode_handle->poutbuf) & 7;
	//编码buffer 起始地址必须8字节对齐
	pencode_handle->pencbuf = ((T_U8 *)pencode_handle->poutbuf) + ((8-temp)&7);
    pencode_handle->IPFrame_ctrl = 0;
	pencode_handle->width = pencInput->width;
	pencode_handle->fps = pencInput->framePerSecond;
    
    if (H264_ENC_TYPE == pencInput->encode_type)
    {
        pencode_handle->encode_type = VIDEO_DRV_H264;
	    //H264 encoder
        open_input.encFlag = VIDEO_DRV_H264;
    	open_input.encH264Par.width = pencInput->width;			//实际编码图像的宽度，能被4整除
    	open_input.encH264Par.height = pencInput->height;			//实际编码图像的长度，能被2整除 
    	open_input.encH264Par.lumWidthSrc = pencInput->lumWidthSrc;
    	open_input.encH264Par.lumHeightSrc = pencInput->lumHeightSrc;
    	open_input.encH264Par.horOffsetSrc = (pencInput->lumHeightSrc - pencInput->height)/2;
    	open_input.encH264Par.verOffsetSrc = (pencInput->lumHeightSrc - pencInput->height)/2;

    	/*
    	    码率调试技巧:
    	    初始frameRateNum要设置的比后面实际帧率要大(比如大一倍)，可以有效降低码率。
    	    此时可以把qpMax设置的小一点，保证画面质量无马赛克。
    	*/
    	open_input.encH264Par.rotation = 0;		//编码前yuv图像的旋转
    	open_input.encH264Par.frameRateDenom = 1;	//帧率的分母
    	open_input.encH264Par.frameRateNum = pencInput->framePerSecond;	//帧率的分子
    	
    	open_input.encH264Par.qpHdr = 28;//-1;			//初始的QP的值
      	open_input.encH264Par.streamType = 0;		//有startcode和没startcode两种
#if 0
		if (2 == pvideo_default_config->profile_mode)	//high profile
		{
			open_input.encH264Par.enableCabac = 1;
    		open_input.encH264Par.transform8x8Mode = 2;
		}
		else if (0 == pvideo_default_config->profile_mode)	//base line
		{
			open_input.encH264Par.enableCabac = 0;
    		open_input.encH264Par.transform8x8Mode = 0;
		}
		else	//main profile
		{
			open_input.encH264Par.enableCabac = 1;
    		open_input.encH264Par.transform8x8Mode = 0;
		}
#else
		open_input.encH264Par.enableCabac = 1;
		open_input.encH264Par.transform8x8Mode = 0;
#endif

    	
    	open_input.encH264Par.qpMin = pencInput->minQp;     //the minimize is 10      
    	open_input.encH264Par.qpMax = pencInput->maxQp; //the max is 51

		//if(!pencInput->video_mode) //video mode = 0, CBR
	    	open_input.encH264Par.fixedIntraQp = 0;
		//else  //VBR
			//open_input.encH264Par.fixedIntraQp = (pencInput->maxQp + pencInput->minQp)/2; //25;
			
		pencode_handle->video_mode = pencInput->video_mode;
		pencode_handle->qp = open_input.encH264Par.fixedIntraQp; //VBR mode
				
        open_input.encH264Par.bitPerSecond = pencInput->bitPerSecond*1024;	//目标bps
        open_input.encH264Par.gopLen = pencInput->gopLen;  

        //开启宏块QP控制以及丢帧功能
		open_input.encH264Par.mbRc = 0;
		open_input.encH264Par.hrd = 0;    
		open_input.encH264Par.pictureSkip = 0;

		if (0 == pencode_handle->video_mode) { //CBR
			open_input.encH264Par.hrdCpbSize = pencInput->bitPerSecond*50;
		} else { //VBR
			open_input.encH264Par.hrdCpbSize = pencInput->bitPerSecond*50;
		}
		open_input.encH264Par.intraQpDelta = -3;
		open_input.encH264Par.mbQpAdjustment = -2;
        
    	pencode_handle->video_rc.qpHdr = 28;//-1;
    	pencode_handle->video_rc.qpMin = open_input.encH264Par.qpMin;
    	pencode_handle->video_rc.qpMax = open_input.encH264Par.qpMax;
    	pencode_handle->video_rc.fixedIntraQp = open_input.encH264Par.fixedIntraQp;
    	pencode_handle->video_rc.bitPerSecond =  open_input.encH264Par.bitPerSecond;
    	pencode_handle->video_rc.gopLen = pencInput->gopLen; //gop is twice as framerate
		if(0 == pencode_handle->video_mode){ //CBR
			pencode_handle->video_rc.hrdCpbSize = pencInput->bitPerSecond*50;
		} else { //VBR
			pencode_handle->video_rc.hrdCpbSize = pencInput->bitPerSecond*50;
		}
		pencode_handle->video_rc.intraQpDelta = -3;
		pencode_handle->video_rc.mbQpAdjustment = -2;

	    anyka_print("[%s] H.264 params:\nw=%ld, h=%ld, qpMin=%ld, qpMax=%ld, bps=%ld, gop=%ld, fps=%ld\n", 
					__func__,
                    open_input.encH264Par.width,
                    open_input.encH264Par.height,
                    open_input.encH264Par.qpMin,
                    open_input.encH264Par.qpMax,
                    open_input.encH264Par.bitPerSecond,
                    open_input.encH264Par.gopLen,
                    open_input.encH264Par.frameRateNum);
    } else {
	    //MJPEG encoder
    	open_input.encFlag = VIDEO_DRV_MJPEG;
    	open_input.encMJPEGPar.frameType = ENC_YUV420_PLANAR;//JPEGENC_YUV420_PLANAR;
    	open_input.encMJPEGPar.format = ENC_THUMB_JPEG;
    	open_input.encMJPEGPar.thumbWidth = 0;
    	open_input.encMJPEGPar.thumbHeight = 0;
    	open_input.encMJPEGPar.thumbData = NULL;
    	open_input.encMJPEGPar.thumbDataLen = 0;
    	open_input.encMJPEGPar.qLevel = 7;
    	open_input.encMJPEGPar.width = pencInput->width;
    	open_input.encMJPEGPar.height = pencInput->height;
    	open_input.encMJPEGPar.lumWidthSrc = pencInput->lumWidthSrc;
    	open_input.encMJPEGPar.lumHeightSrc = pencInput->lumHeightSrc;
    	open_input.encMJPEGPar.horOffsetSrc = (pencInput->lumWidthSrc - pencInput->width)/2;
    	open_input.encMJPEGPar.verOffsetSrc = (pencInput->lumHeightSrc - pencInput->height)/2;
    }
    
	pencode_handle->encode_handle = VideoStream_Enc_Open(&open_input);
	if (!pencode_handle->encode_handle){		
		akuio_free_pmem(pencode_handle->poutbuf);
        free(pencode_handle);
        anyka_print("[%s] failed, VideoStream_Enc_Open!\n", __func__);
		return NULL;
	}
    pthread_mutex_init(&pencode_handle->lock, NULL);
    
	{
   		unsigned int temp;
		T_VIDEOLIB_ENC_PARA video_enc_para;
	  
		video_enc_para.skipPenalty = 255;	       
		video_enc_para.interFavor = 420; //-1 for hantro drive code value
		video_enc_para.intra16x16Favor = 1272; //-1 for hantro drive code value
		video_enc_para.intra4x4Favor = -1; //-1 for hantro drive code value
		video_enc_para.chromaQPOffset = 127; //127 for hantro drive code value
		video_enc_para.diffMVPenalty4p = -1; //-1 for hantro drive code value
		video_enc_para.diffMVPenalty1p = -1; //-1 for hantro drive code value
		video_enc_para.minIQP = pencInput->minQp;
		video_enc_para.maxIQP = pencInput->maxQp;
		video_enc_para.adjustment_area_pencent = 30;

		temp = (pencInput->bitPerSecond*1024) / (pencInput->framePerSecond);
	
		video_enc_para.qp_up_bitsize_threshold1 = temp*6/5;
		video_enc_para.qp_up_delta1 = 1;
		video_enc_para.qp_up_bitsize_threshold2 = temp*3/2;
		video_enc_para.qp_up_delta2 = 2;
		video_enc_para.qp_up_bitsize_threshold3 = temp*2;
		video_enc_para.qp_up_delta3 = 3;
		video_enc_para.qp_down_bitsize_threshold1 = temp*9/10;
		video_enc_para.qp_down_delta1 = 1;
		video_enc_para.qp_down_bitsize_threshold2 = temp*7/10;
		video_enc_para.qp_down_delta2 = 2;
		video_enc_para.qp_down_bitsize_threshold3 = temp/2;
		video_enc_para.qp_down_delta3 = 3;
		video_enc_para.mbRows_threshold = 20;
		
		video_enc_para.debug = 0;
		video_enc_para.quarterPixelMv = 0; // 1 for enable, other value for disable
		video_enc_para.qp_filter_k = 32; //0~64

		if (1280 == pencInput->width) 
			VideoStream_Enc_setEncPara_mainch(&video_enc_para);
		else
			VideoStream_Enc_setEncPara_subch(&video_enc_para);

	}


	return pencode_handle;
}

/**
* @brief  video encode one frame in channel2
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return size success, otherwise failed 
*/
int video_encode_frame(T_VOID * pencode_handle, void *pinbuf2, void **poutbuf2, int* nIsIFrame)
{
	T_VIDEOLIB_ENC_IO_PAR video_enc_io_param2;   
    Panyka_encode_handle handle = (Panyka_encode_handle)pencode_handle;
	
	//the second channel setting, CBR mode
	//if(!handle->video_mode)		//CBR
	    video_enc_io_param2.QP = 0;		        //编码器自行决定		
	//else	//VBR
    	//video_enc_io_param2.QP = handle->qp;    //编码器自行决定		

    if(NULL == pencode_handle)
    {
        anyka_print("[%s:%d] it fails because of null!\n", __func__, __LINE__);
        return -1;
    }

	if ((handle->encode_type == VIDEO_DRV_H264 && handle->IPFrame_ctrl == 0) || handle->encode_type == VIDEO_DRV_MJPEG) //I frame
	{
		*nIsIFrame = 1;					
		video_enc_io_param2.mode = 0;   //编码类型I/P帧,0，i，1，p
	}
	else //P frame
	{
		*nIsIFrame = 0;
		video_enc_io_param2.mode = 1;
	}

	video_enc_io_param2.p_curr_data = pinbuf2;		//yuv输入地址
	video_enc_io_param2.p_vlc_data = handle->pencbuf;			//码流输出地址
	video_enc_io_param2.out_stream_size = ENCMEM;	//输出码流的大小

    pthread_mutex_lock(&handle->lock);    
	VideoStream_Enc_Encode(handle->encode_handle, NULL, &video_enc_io_param2, NULL);
    pthread_mutex_unlock(&handle->lock);    

	*poutbuf2 = video_enc_io_param2.p_vlc_data;

	if (++handle->IPFrame_ctrl >= handle->video_rc.gopLen)
		handle->IPFrame_ctrl = 0;

    if(!video_enc_io_param2.out_stream_size)
    { 
		anyka_debug("encode size 0\n");
	}
	
	return video_enc_io_param2.out_stream_size;
}

void video_encode_set_Iframe(T_VOID * pencode_handle)
{
    Panyka_encode_handle handle = (Panyka_encode_handle)pencode_handle;
    
    if(handle)
    {
        handle->IPFrame_ctrl = 0;
    }
}

/**
* @brief  close vedio encoder
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
int video_encode_close(T_VOID * pencode_handle)
{
    
    Panyka_encode_handle handle = (Panyka_encode_handle)pencode_handle;
    
	VideoStream_Enc_Close(handle->encode_handle);
	akuio_free_pmem(handle->poutbuf);
    pthread_mutex_destroy(&handle->lock);    

    free(pencode_handle);
	return 0;
}

void video_encode_set_subchn_rc(void *penc_handle, int bps)
{
    Panyka_encode_handle handle = (Panyka_encode_handle)penc_handle;
	T_VIDEOLIB_ENC_PARA video_enc_para;
	int temp;

	video_enc_para.skipPenalty = 255;
	video_enc_para.interFavor = 420; //-1 for hantro drive code value
	video_enc_para.intra16x16Favor = 1272; //-1 for hantro drive code value
	video_enc_para.intra4x4Favor = -1; //-1 for hantro drive code value
	video_enc_para.chromaQPOffset = 127; //127 for hantro drive code value
	video_enc_para.diffMVPenalty4p = -1; //-1 for hantro drive code value
	video_enc_para.diffMVPenalty1p = -1; //-1 for hantro drive code value
	video_enc_para.minIQP = handle->video_rc.qpMin;
	video_enc_para.maxIQP = handle->video_rc.qpMax;
	video_enc_para.adjustment_area_pencent = 30;

	temp = (handle->video_rc.bitPerSecond) / (handle->fps);

	/* vga VBR */
	if (handle->video_mode) {
		video_enc_para.qp_up_bitsize_threshold1 = temp*6/5;
		video_enc_para.qp_up_delta1 = 1;
		video_enc_para.qp_up_bitsize_threshold2 = temp*3/2;
		video_enc_para.qp_up_delta2 = 2;
		video_enc_para.qp_up_bitsize_threshold3 = temp*2;
		video_enc_para.qp_up_delta3 = 3;

		video_enc_para.qp_down_bitsize_threshold1 = temp*4/5;
		video_enc_para.qp_down_delta1 = 1;
		video_enc_para.qp_down_bitsize_threshold2 = temp*3/5;
		video_enc_para.qp_down_delta2 = 2;
		video_enc_para.qp_down_bitsize_threshold3 = temp/2;
		video_enc_para.qp_down_delta3 = 3;
		
		if (bps <= 200) {
			handle->video_rc.qpMin = 28;
			handle->video_rc.qpMax = 40;	//incase the ini file's qpmax less than qpmin here we set
			video_enc_para.qp_up_delta3 = 2;
			video_enc_para.qp_down_delta3 = 3;
		} else if (bps <= 300) {
			handle->video_rc.qpMin = 28;
			handle->video_rc.qpMax = 38;	//incase the ini file's qpmax less than qpmin here we set
		} else if (bps <= 500) {
			handle->video_rc.qpMin = 25;
			handle->video_rc.qpMax = 35;	//incase the ini file's qpmax less than qpmin here we set
		} else {
			handle->video_rc.qpMin = 24;
			handle->video_rc.qpMax = 35;	//incase the ini file's qpmax less than qpmin here we set
		}

		anyka_print("[%s:%d] VGA VBR #####\n", __func__, __LINE__);
	} else {
		/* vga CBR */	
		if (bps <= 200) {
			handle->video_rc.qpMin = 27;
			handle->video_rc.qpMax = 41;	//incase the ini file's qpmax less than qpmin here we set
		} else if (bps <= 300) {
			handle->video_rc.qpMin = 26;
			handle->video_rc.qpMax = 38;	//incase the ini file's qpmax less than qpmin here we set
		} else if (bps <= 500) {
			handle->video_rc.qpMin = 24;
			handle->video_rc.qpMax = 36;	//incase the ini file's qpmax less than qpmin here we set
		} else {
			handle->video_rc.qpMin = 22;
			handle->video_rc.qpMax = 36;	//incase the ini file's qpmax less than qpmin here we set
		}
		video_enc_para.qp_up_bitsize_threshold1 = temp*23/20;
		video_enc_para.qp_up_delta1 = 1;
		video_enc_para.qp_up_bitsize_threshold2 = temp*7/5;
		video_enc_para.qp_up_delta2 = 2;
		video_enc_para.qp_up_bitsize_threshold3 = temp*9/5;
		video_enc_para.qp_up_delta3 = 3;

		video_enc_para.qp_down_bitsize_threshold1 = temp*9/10;
		video_enc_para.qp_down_delta1 = 1;
		video_enc_para.qp_down_bitsize_threshold2 = temp*7/10;
		video_enc_para.qp_down_delta2 = 2;
		video_enc_para.qp_down_bitsize_threshold3 = temp/2;
		video_enc_para.qp_down_delta3 = 3;

		anyka_print("[%s:%d] VGA CBR #####\n", __func__, __LINE__);
	}
	video_enc_para.debug = 0;
	video_enc_para.mbRows_threshold = 20;
	video_enc_para.quarterPixelMv = 1; // 1 for enable, other value for disable
	video_enc_para.qp_filter_k = 32; //0~64

	VideoStream_Enc_setEncPara_subch(&video_enc_para);
}

void video_encode_set_mainchn_rc(void *penc_handle, int bps)
{
    Panyka_encode_handle handle = (Panyka_encode_handle)penc_handle;
	T_VIDEOLIB_ENC_PARA video_enc_para;
	int temp;

	temp = (handle->video_rc.bitPerSecond) / (handle->fps);

	/* 720P VBR */
	if (handle->video_mode) {
		video_enc_para.qp_up_bitsize_threshold1 = temp*6/5;
		video_enc_para.qp_up_delta1 = 1;
		video_enc_para.qp_up_bitsize_threshold2 = temp*3/2;
		video_enc_para.qp_up_delta2 = 2;
		video_enc_para.qp_up_bitsize_threshold3 = temp*2;
		video_enc_para.qp_up_delta3 = 3;

		video_enc_para.qp_down_bitsize_threshold1 = temp*8/10;
		video_enc_para.qp_down_delta1 = 1;
		video_enc_para.qp_down_bitsize_threshold2 = temp*6/10;
		video_enc_para.qp_down_delta2 = 2;
		video_enc_para.qp_down_bitsize_threshold3 = temp/2;
		video_enc_para.qp_down_delta3 = 3;

		if (bps <= 300) {
			handle->video_rc.qpMin = 28;
			handle->video_rc.qpMax = 42;	//incase the ini file's qpmax less than qpmin here we set
			video_enc_para.qp_up_delta3 = 2;
			video_enc_para.qp_down_delta3 = 2;
		} else if (bps <= 500) {
			handle->video_rc.qpMin = 28;
			handle->video_rc.qpMax = 40;	//incase the ini file's qpmax less than qpmin here we set
		} else if (bps <= 800) {
			handle->video_rc.qpMin = 27;
			handle->video_rc.qpMax = 38;	//incase the ini file's qpmax less than qpmin here we set	
		} else {
			handle->video_rc.qpMin = 27;
			handle->video_rc.qpMax = 37;	//incase the ini file's qpmax less than qpmin here we set
		}
		anyka_print("[%s:%d] VBR #####\n", __func__, __LINE__);
	} else {
		/* 720P CBR */
		if (bps <= 300) {
			handle->video_rc.qpMin = 28;
			handle->video_rc.qpMax = 43;	//incase the ini file's qpmax less than qpmin here we set
		} else if (bps <= 500) {
			handle->video_rc.qpMin = 28;
			handle->video_rc.qpMax = 41;	//incase the ini file's qpmax less than qpmin here we set
		} else if (bps <= 800) {
			handle->video_rc.qpMin = 27;
			handle->video_rc.qpMax = 39;	//incase the ini file's qpmax less than qpmin here we set
		} else {
			handle->video_rc.qpMin = 26;
			handle->video_rc.qpMax = 38;	//incase the ini file's qpmax less than qpmin here we set
		}
		video_enc_para.qp_up_bitsize_threshold1 = temp*23/20;
		video_enc_para.qp_up_delta1 = 1;
		video_enc_para.qp_up_bitsize_threshold2 = temp*7/5;
		video_enc_para.qp_up_delta2 = 2;
		video_enc_para.qp_up_bitsize_threshold3 = temp*9/5;
		video_enc_para.qp_up_delta3 = 3;

		video_enc_para.qp_down_bitsize_threshold1 = temp*9/10;
		video_enc_para.qp_down_delta1 = 1;
		video_enc_para.qp_down_bitsize_threshold2 = temp*7/10;
		video_enc_para.qp_down_delta2 = 2;
		video_enc_para.qp_down_bitsize_threshold3 = temp/2;
		video_enc_para.qp_down_delta3 = 3;

		anyka_print("[%s:%d] CBR #####\n", __func__, __LINE__);
	}
	if (960 == camera_get_ch1_height()) {
		handle->video_rc.qpMax += 2;
	}

	video_enc_para.skipPenalty = 255;
	video_enc_para.interFavor = 420; //-1 for hantro drive code value
	video_enc_para.intra16x16Favor = 1272; //-1 for hantro drive code value
	video_enc_para.intra4x4Favor = -1; //-1 for hantro drive code value
	video_enc_para.chromaQPOffset = 127; //127 for hantro drive code value
	video_enc_para.diffMVPenalty4p = -1; //-1 for hantro drive code value
	video_enc_para.diffMVPenalty1p = -1; //-1 for hantro drive code value
	video_enc_para.minIQP = handle->video_rc.qpMin;
	video_enc_para.maxIQP = handle->video_rc.qpMax;
	video_enc_para.adjustment_area_pencent = 30;
	video_enc_para.debug = 0;

	video_enc_para.mbRows_threshold = 20;
	video_enc_para.quarterPixelMv = 1; // 1 for enable, other value for disable
	video_enc_para.qp_filter_k = 32; //0~64

	VideoStream_Enc_setEncPara_mainch(&video_enc_para);
}
/**
* @brief  reset encode bitpersecond
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/

int video_encode_reSetRc(T_VOID * pencode_handle, int bps)
{
    int ret;
    Panyka_encode_handle handle = (Panyka_encode_handle)pencode_handle;

	/** for motion jpeg enconde type **/
	if(handle->encode_type == VIDEO_DRV_MJPEG) {		
		return 0;
	} 

	/** set qpmin according to bps **/
	if (1280 == handle->width) {
		/* 720p */
		anyka_print("[%s] %d(kbps)\n", __func__, bps);
		bps = bps * 85 / 100;
		/** set main channel  bps **/
	    handle->video_rc.bitPerSecond = bps * 1024;
		video_encode_set_mainchn_rc(handle, bps);
	} else {
		/** set sub channel bps **/
		anyka_print("[%s] %d(kbps)\n", __func__, bps);
		bps = bps * 9/10;
	    handle->video_rc.bitPerSecond = bps * 1024;
		video_encode_set_subchn_rc(handle, bps);
	}

#if 0
	if (1 == handle->video_mode)	//VBR
	{
		handle->video_rc.qpMin += 2;
		handle->video_rc.qpMax -= 2;
	}
#endif

	anyka_print("[%s:%d] set qp to [%ld,%ld]\n",
		   	__func__, __LINE__, handle->video_rc.qpMin, handle->video_rc.qpMax);
		   	
    pthread_mutex_lock(&handle->lock);
	ret = VideoStream_Enc_setRC(handle->encode_handle, &handle->video_rc);
    pthread_mutex_unlock(&handle->lock);

    return ret;
}


/**
* @brief  get encode kbps
* 
* @param[in] 
* @return T_S32
* @retval >=0 kbps ;-1:failed
*/

int video_encode_Getkbps(T_VOID * pencode_handle)
{
	int kbps = 0;
	Panyka_encode_handle handle = (Panyka_encode_handle)pencode_handle;

	/** for motion jpeg enconde type **/
	if(handle->encode_type == VIDEO_DRV_MJPEG) {		
		return -1;
	} 

	/** get bps **/
    kbps = handle->video_rc.bitPerSecond >> 10;
	return kbps;
}


