#include "common.h"
#include "sdcodec.h"
#include "audio_enc.h"

#define BITS_RATE_128Kbps		128000	//for mp3 bits out 128kbps
#define BITS_RATE_12Dot2Kbps	12200	//for amr bits out 12.2kbps
#define BITS_RATE_10Dot2Kbps	10200	//for amr bits out 10.2kbps
#define BITS_RATE_7Dot95Kbps	7950	//for amr bits out 7.95kbps
#define BITS_RATE_7Dot4Kbps		7400	//for amr bits out 7.4kbps
#define BITS_RATE_6Dot7Kbps		6700	//for amr bits out 6.7kbps
#define BITS_RATE_5Dot9Kbps		5900	//for amr bits out 5.9kbps
#define BITS_RATE_5Dot15Kbps	5150	//for amr bits out 5.15kbps
#define BITS_RATE_4Dot75Kbps	4750	//for amr bits out 4.75kbps

//static T_VOID * 			g_EncLib 	  = NULL;
typedef struct _anyka_audio_encode_info
{
    T_U8 *				EncBuffer;
    T_U32				nEncBufSize;
    T_U32				nBufUse;
    T_U32				nChannels;
    AUDIO_ENCODE_TYPE_CC	nEncType;
    T_VOID  *encode_handle;
}anyka_audio_encode, *Panyka_audio_encode;

#define MP3_MAX_INPACKET_SIZE		8192	// mp3 encoder max input is 8k
#define AAC_SINGLE_INPACKET_SIZE 	2048	// aac one channel input is 2k
#define AAC_STEREO_INPACKET_SIZE	4096	// aac two channels input is 4k
T_S32 audio_encode( T_VOID* p_enc, T_U8 * pRawData, T_U32 nRawDataSize, T_U8 * pEncBuf, T_U32 nEncBufSize );


/*
 * @brief		used for media lib to print 
 * @param	format [in] ,... [in]
 * @return	T_VOID
 * @retval	NONE
 */
T_VOID ak_rec_cb_printf( T_pCSTR format, ... )
{
	//REC_TAG, used to identify print informations of media lib
}

/*
 * @brief		used to alloc memory
 * @param	size [in] 
 * @return	T_pVOID
 * @retval	NULL for error,otherwise the handle of memory allocated.
 */
T_pVOID ak_rec_cb_malloc( T_U32 size )
{
	//anyka_print("malloc size 0x%lx\n",size);
	return malloc( size );
}

/*
 * @brief		free memory
 * @param	mem [in] 
 * @return	T_VOID
 * @retval	NONE
 */
T_VOID ak_rec_cb_free( T_pVOID mem )
{
	//anyka_print("free buffer %p",mem);
	return free( mem );
}



/*
 * @brief		media lib will call this while idle.
 * @param	ticks[in]
 * @return	T_BOOL
 * @retval	AK_TRUE for success,other for error.
 */
T_BOOL ak_rec_cb_lnx_delay(T_U32 ticks)
{
	anyka_print("delay 0x%lx ticks",ticks);
	
#ifdef ANDROID
	//usleep (ticks*1000);
	akuio_wait_irq();
	return true;
#else
	return (usleep(ticks*1000) == 0);
#endif
}



/**
* @brief  open the AK audio encode lib
* @author hankejia
* @date 2012-07-05
* @param[in] pstEncIn		the audio pcm data is params.
* @param[in] pstEncOut		the audio encode data is params.
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
T_VOID *audio_enc_open( AudioEncIn * pstEncIn, AudioEncOut * pstEncOut )
{
	T_AUDIO_REC_INPUT 		inConfig;
	T_AUDIO_ENC_OUT_INFO	outConfig;
    Panyka_audio_encode audio_encode_handle;
	bzero( &inConfig, sizeof( T_AUDIO_REC_INPUT ) );
	bzero( &outConfig, sizeof( T_AUDIO_ENC_OUT_INFO ) );
#if 0	
	if ( g_EncLib ) {
		anyka_print( "already open encode lib, can't open again! please close first!\n" );
		return -1;
	}
#endif
	if ( ( NULL == pstEncIn ) || ( NULL == pstEncOut ) ) {
		anyka_print( "openSdEncodeLib::Invalid parameter!\n" );
		return NULL;
	}
#if 0
	if ( NULL != g_EncBuffer ) {
		free( g_EncBuffer );
		g_EncBuffer = NULL;
	}
#endif
    audio_encode_handle = (Panyka_audio_encode)malloc(sizeof(anyka_audio_encode));
    if(NULL == audio_encode_handle)
    {
        return NULL;
    }
    memset(audio_encode_handle, 0 , sizeof(anyka_audio_encode));
	//config the call back function
	inConfig.cb_fun.Malloc = ak_rec_cb_malloc;
	inConfig.cb_fun.Free = ak_rec_cb_free;
	inConfig.cb_fun.printf = (MEDIALIB_CALLBACK_FUN_PRINTF)anyka_print;
	inConfig.cb_fun.delay = ak_rec_cb_lnx_delay;

	//config the encode input pcm data is params
	inConfig.enc_in_info.m_nChannel = pstEncIn->nChannels;
	inConfig.enc_in_info.m_nSampleRate = pstEncIn->nSampleRate;
	inConfig.enc_in_info.m_BitsPerSample = pstEncIn->nBitsPerSample;

	audio_encode_handle->nChannels = pstEncIn->nChannels;
//	g_nEncType = pstEncOut->enc_type;
	audio_encode_handle->nEncType = pstEncOut->enc_type;
	switch ( pstEncOut->enc_type ) 
	{
		
	case ENC_TYPE_AAC:
		inConfig.enc_in_info.m_Type = _SD_MEDIA_TYPE_AAC;
		if ( 1 == pstEncIn->nChannels ) {
			audio_encode_handle->EncBuffer = (T_U8 *)malloc( AAC_SINGLE_INPACKET_SIZE );
			audio_encode_handle->nEncBufSize = AAC_SINGLE_INPACKET_SIZE;
		} else if ( 2 == pstEncIn->nChannels ) {
			audio_encode_handle->EncBuffer = (T_U8 *)malloc( AAC_STEREO_INPACKET_SIZE );
			audio_encode_handle->nEncBufSize = AAC_STEREO_INPACKET_SIZE;
		}else {
			anyka_print( "openSdEncodeLib::no support %ld channels!\n", pstEncIn->nChannels );
			return NULL;
		}

		if ( NULL == audio_encode_handle->EncBuffer ) {
			anyka_print( "openSdEncodeLib::out of memory!\n"  );
			return NULL;
		}

		bzero( audio_encode_handle->EncBuffer, audio_encode_handle->nEncBufSize );
		
		break;
    case ENC_TYPE_AMR:
		inConfig.enc_in_info.m_Type = _SD_MEDIA_TYPE_AMR;
        inConfig.enc_in_info.m_private.m_amr_enc.mode = AMR_ENC_MR122;
        break;
        
	case ENC_TYPE_ADPCM_IMA:
		inConfig.enc_in_info.m_Type = _SD_MEDIA_TYPE_ADPCM_IMA;
		inConfig.enc_in_info.m_private.m_adpcm.enc_bits = 4; //̶Ϊ4
		break;
	case ENC_TYPE_G711:
		inConfig.enc_in_info.m_Type = _SD_MEDIA_TYPE_PCM_ALAW;
		break;
		
	default:
		anyka_print( "openSdEncodeLib::Unknow encode type!\n" );
		return NULL;
	}
    inConfig.chip = AUDIOLIB_CHIP_AK39XXE;
	//open the ak audio encode lib
	audio_encode_handle->encode_handle = _SD_Encode_Open(&inConfig, &outConfig);
	if ( NULL == audio_encode_handle->encode_handle ) {
		anyka_print( "openSdEncodeLib::can't open SdCodec!\n" );
		return NULL;
	}
    if(pstEncOut->enc_type == ENC_TYPE_AAC)
        _SD_Encode_SetFramHeadFlag(audio_encode_handle->encode_handle ,1);

	return audio_encode_handle;
}


/**
* @brief  close the AK audio encode lib
* @author hankejia
* @date 2012-07-05
* @param[in] NONE
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
T_S32 audio_enc_close(void *handle)
{
    Panyka_audio_encode pencode_handle = (Panyka_audio_encode)handle;
    
	if ( NULL == pencode_handle) {
		return 0;
	}
    
	if ( NULL == pencode_handle->encode_handle) {
        free(pencode_handle);
		return 0;
	}

	if( _SD_Encode_Close( pencode_handle->encode_handle) != AK_TRUE ){
		anyka_print("unable close encode lib!\n");
		return -1;
	}
	

	if ( pencode_handle->EncBuffer ) {
		free( pencode_handle->EncBuffer );
		pencode_handle->EncBuffer = NULL;
	}
    free(pencode_handle);        
	return 0;
}



/**
* @brief  encode the pcm data to the aac data
* @author hankejia
* @date 2012-07-05
* @param[in] pRawData	the audio pcm raw data
* @param[in] nRawDataSize	raw data buffer size
* @param[out] pEncBuf		encode out buffer
* @param[in] nEncBufSize	encode out buffer size
* @return T_S32
* @retval if >=0 encode return size and success, otherwise failed 
*/
static T_S32 audio_pcm2aac( T_VOID * p_enc, T_U8 * pRawData, T_U32 nRawDataSize, T_U8 * pEncBuf, T_U32 nEncBufSize )
{
	T_AUDIO_ENC_BUF_STRC encBuf	= { NULL, NULL, 0, 0 };
	T_S32 ret = 0;
	T_S32 nEncSize = 0;    
    Panyka_audio_encode pencode_handle = (Panyka_audio_encode)p_enc;
    
	encBuf.buf_out	= pEncBuf;
	encBuf.len_out	= nEncBufSize;

	while( nRawDataSize > 0 ) {
		if ( nRawDataSize < ( pencode_handle->nEncBufSize - pencode_handle->nBufUse ) ) {
			memcpy( pencode_handle->EncBuffer + pencode_handle->nBufUse, pRawData, nRawDataSize );
			pencode_handle->nBufUse += nRawDataSize;
			return nEncSize;
		} else if ( nRawDataSize >= ( pencode_handle->nEncBufSize - pencode_handle->nBufUse ) ) {
			memcpy( pencode_handle->EncBuffer + pencode_handle->nBufUse, pRawData, pencode_handle->nEncBufSize - pencode_handle->nBufUse );
			pRawData += (pencode_handle->nEncBufSize - pencode_handle->nBufUse);
			nRawDataSize -= (pencode_handle->nEncBufSize - pencode_handle->nBufUse);
			pencode_handle->nBufUse = pencode_handle->nEncBufSize;
		}else {
			anyka_print( "audio_pcm2aac::how could this happend?\n" );
			ret = -1;
			break;
		}
		
		encBuf.buf_in	= pencode_handle->EncBuffer;
		encBuf.len_in 	= pencode_handle->nEncBufSize;
		encBuf.buf_out	= pEncBuf + nEncSize;
		encBuf.len_out	= nEncBufSize - nEncSize;

		if ( encBuf.len_out <= 0 ) {
			anyka_print( "audio_pcm2aac::the encode out buffer too small!\n" );
			ret = -1;
			break;
		}
		
		ret = _SD_Encode( pencode_handle->encode_handle, &encBuf );
		if ( ret < 0 ) {
			break;
		}

		nEncSize += ret;

		pencode_handle->nBufUse = 0;
	}

	if ( ret < 0 )
		pencode_handle->nBufUse = 0;
	
	return ret;
}


/**
* @brief  encode the pcm data
* @author hankejia
* @date 2012-07-05
* @param[in] pRawData	the audio pcm raw data
* @param[in] nRawDataSize	raw data buffer size
* @param[out] pEncBuf		encode out buffer
* @param[in] nEncBufSize	encode out buffer size
* @return T_S32
* @retval if >=0 encode return size and success, otherwise failed 
*/
T_S32 audio_encode( T_VOID* p_enc, T_U8 * pRawData, T_U32 nRawDataSize, T_U8 * pEncBuf, T_U32 nEncBufSize )
{
    Panyka_audio_encode pencode_handle = (Panyka_audio_encode)p_enc;
    AUDIO_ENCODE_TYPE_CC type;
    
    if(pencode_handle == NULL)
    {
	    anyka_print("[%s:%d] the handle don't been openned!\n", __func__, __LINE__);
        return -1;
    }

    type = pencode_handle->nEncType;
#if 0
	if ( NULL == g_EncLib ) {
		anyka_print( "audio_pcm_encode::can't encode pcm data before open SdCodec! please call the openSdEncodeLib\n" );
		return -1;
	}
#endif
	//open encode lib with mp3 encode type
	 if ( ENC_TYPE_AAC == type ) {
		return audio_pcm2aac( pencode_handle ,pRawData, nRawDataSize, pEncBuf, nEncBufSize );
	}else {
	
		T_AUDIO_ENC_BUF_STRC encBuf	= { NULL, NULL, 0, 0 };
		
		encBuf.buf_in	= pRawData;
		encBuf.len_in 	= nRawDataSize;
		encBuf.buf_out	= pEncBuf;
		encBuf.len_out	= nEncBufSize;
		return _SD_Encode( pencode_handle->encode_handle, &encBuf );
	}

	return -1;
}

