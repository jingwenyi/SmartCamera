#ifndef _muxer_h_
#define _muxer_h_
#include "anyka_types.h"
#include "media_muxer_lib.h"

typedef struct {
	T_pSTR rec_path;
	T_eMEDIALIB_REC_TYPE	m_MediaRecType;
	T_BOOL					m_bCaptureAudio;

	//video
	T_eMEDIALIB_VIDEO_CODE	m_eVideoType;
	T_U16	m_nWidth;
	T_U16	m_nHeight;

	//audio
	T_eMEDIALIB_AUDIO_CODE	m_eAudioType;
	T_U32	m_nSampleRate;		//²ÉÑùÂÊ(8000)
	T_U32	abitsrate;
	
}T_MUX_INPUT;


/**
 * NAME        mux_open
 * @BRIEF	open mux lib
                  
 * @PARAM	T_MUX_INPUT *mux_input,  set mux lib attribute
 			int record_fd, save file descriptor
 			int index_fd, tmp file descriptor
 * @RETURN	void * 
 * @RETVAL	mux handle
 */

void* mux_open(T_MUX_INPUT *mux_input, int record_fd, int index_fd);

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

int mux_addAudio(void *handle,void *pbuf, unsigned long size, unsigned long timestamp);


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

int mux_addVideo(void *handle,void *pbuf, unsigned long size, unsigned long timestamp, int nIsIFrame);


/**
 * NAME        mux_addAudio
 * @BRIEF	close mux 
                  
 * @PARAM	void *handle,  handle when open 

 * @RETURN	int
 * @RETVAL	if return 0 success, otherwise failed
 */

int mux_close(void *handle);


#endif

