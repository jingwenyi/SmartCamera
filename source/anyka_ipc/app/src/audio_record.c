#include "common.h"

/**************************************************************
此模块只完成录音，目前以AMR或WAV文件格式为输出，但是编码可以支持AMR-NB,MP3,WAV这几种格式，
如果需要其它格式，请向安凯申请相应的音频库

***************************************************************/
typedef T_U32	FOURCC;    /* a four character code */

#define WAVE_FORMAT_PCM 	1

/* MMIO macros */
#define mmioFOURCC(ch0, ch1, ch2, ch3) \
	((T_U32)(T_U8)(ch0) | ((T_U32)(T_U8)(ch1) << 8) | \
	((T_U32)(T_U8)(ch2) << 16) | ((T_U32)(T_U8)(ch3) << 24))

typedef struct CHUNKHDR {	
	FOURCC ckid;        /* chunk ID */	
	DWORD dwSize;       /* chunk size */
} CHUNKHDR;

#define FOURCC_RIFF		mmioFOURCC ('R', 'I', 'F', 'F')		//RIFF
#define FOURCC_LIST		mmioFOURCC ('L', 'I', 'S', 'T')		//LIST
#define FOURCC_WAVE		mmioFOURCC ('W', 'A', 'V', 'E')		//WAVE
#define FOURCC_FMT		mmioFOURCC ('f', 'm', 't', ' ')		//fmt
#define FOURCC_DATA		mmioFOURCC ('d', 'a', 't', 'a')		//data

#define cpu_to_le32(x) (x)
#define cpu_to_le16(x) (x)
#define le32_to_cpu(x) (x)
#define le16_to_cpu(x) (x)

/* 
 * simplified Header for standard WAV files 
 */
typedef struct WAVEHDR {	
	CHUNKHDR chkRiff;
	FOURCC fccWave;
	CHUNKHDR chkFmt;
    
    /*
     * format type 
     */
	WORD wFormatTag; 
    
    /* 
     * number of channels (i.e. mono, stereo, etc.) 
     */    
	WORD nChannels; 
    
    /* 
     * sample rate 
     */    
	DWORD nSamplesPerSec;  
    
    /* 
     * for buffer estimation 
     */
	DWORD nAvgBytesPerSec; 
    
    /* 
     * block size of data 
     */
	WORD nBlockAlign;       
	WORD wBitsPerSample;
	CHUNKHDR chkData;
} WAVEHDR;

typedef struct _audio_record_ctrl_
{
    int fd;
    int record_type;
    int record_size;
    char file_name[200];
}audio_record_ctrl_info, *Paudio_record_ctrl_info;

static Paudio_record_ctrl_info paudio_record_ctrl = NULL;

void audio_encode_save(void *param, T_STM_STRUCT *pstream)
{
    paudio_record_ctrl->record_size += pstream->size;
    write(paudio_record_ctrl->fd, pstream->buf, pstream->size);
}

/**
* @brief   write the wav file header to file
* @author hankejia
* @date 2012-07-05
* @param[in] fd  			file fd
* @param[in] nChannels  	channels.
* @param[in] nSampleRate	Sample rate.
* @param[in] nDataSize		current file len.
* @return T_S32
* @retval if return 0 success, otherwise failed 
*/
static T_S32 write_wav_head( T_S32 fd, T_U32 nChannels, T_U32 nSampleRate, 
								 T_U32 nBitsPerSample, T_U32 nDataSize )
{
	WAVEHDR stFileheader;
	T_U32 nAvgBytesPerSec = 0, nBlockAlign = 0, nFileSize = 0;
	
	if ( fd <= 0 ) {
		anyka_print( "can't write the wav header, because the fd is invalid!\n" );
		return -1;
	}

	nBlockAlign = nChannels * ( ( nBitsPerSample + 7 ) / 8 );
	nAvgBytesPerSec = nBlockAlign * nSampleRate;
	nFileSize = nDataSize + sizeof(WAVEHDR) - sizeof(CHUNKHDR);
	
	bzero( &stFileheader, sizeof( WAVEHDR ) );

	stFileheader.chkRiff.ckid    = cpu_to_le32(FOURCC_RIFF);	
	stFileheader.fccWave         = cpu_to_le32(FOURCC_WAVE);	
	stFileheader.chkFmt.ckid     = cpu_to_le32(FOURCC_FMT);	
	stFileheader.chkFmt.dwSize   = cpu_to_le32(16);	
	stFileheader.wFormatTag      = cpu_to_le16(WAVE_FORMAT_PCM);	
	stFileheader.nChannels       = cpu_to_le16(nChannels);	
	stFileheader.nSamplesPerSec  = cpu_to_le32(nSampleRate);	
	stFileheader.nAvgBytesPerSec = cpu_to_le32(nAvgBytesPerSec);	
	stFileheader.nBlockAlign     = cpu_to_le16(nBlockAlign);	
	stFileheader.wBitsPerSample  = cpu_to_le16(nBitsPerSample);	
	stFileheader.chkData.ckid    = cpu_to_le32(FOURCC_DATA);	
	stFileheader.chkRiff.dwSize  = cpu_to_le32(nFileSize);	
	stFileheader.chkData.dwSize  = cpu_to_le32(nDataSize /* data length */);

	lseek( fd, 0, SEEK_SET );

	return write( fd, (T_U8*)(&stFileheader), sizeof(WAVEHDR) );
}

/**
 * NAME         audio_record_start
 * @BRIEF     开始启动录音功能，目前录像只保存 一个文件，并不做文件分离，
                   
 * @PARAM   audio_path  :录音文件的路径
            ext_name    :录音文件后缀名(".wav" or ".amr")   
 * @RETURN  void 
 * @RETVAL  
 */

void audio_record_start(char *audio_path, char *ext_name)
{
    char file_name[200];
    const T_U8 amrHeader[]= "#!AMR\n";
#if 0
    //先检查SD卡是否存在
    if(check_sdcard() < 0)
    {
        anyka_print("[%s:%d] fails to check sd\n", __func__, __LINE__);
        return ;
    }
#endif

    if(paudio_record_ctrl )
    {
        anyka_print("[%s:%d] audio record have started\n", __func__, __LINE__);
        return ;
    }
    paudio_record_ctrl = malloc(sizeof(audio_record_ctrl_info));
    if(paudio_record_ctrl == NULL)
    {
        anyka_print("[%s:%d] fails to malloc ram!\n", __func__, __LINE__);
        return ;
    }
    paudio_record_ctrl->record_size = 0;
    video_fs_create_dir(audio_path);
    video_fs_get_audio_record_name(audio_path, file_name, ext_name);
    paudio_record_ctrl->fd = open( file_name ,  O_RDWR | O_CREAT | O_TRUNC );
    if(paudio_record_ctrl->fd < 0)
    {        
        anyka_print("[%s:%d] fails to create the record file!\n", __func__, __LINE__);
        free(paudio_record_ctrl);
        paudio_record_ctrl = NULL;
        return ;
    }
    strcpy(paudio_record_ctrl->file_name, file_name);
    if(strcmp(ext_name, ".amr") == 0)
    {
        paudio_record_ctrl->record_type = SYS_AUDIO_ENCODE_AMR;
        write(paudio_record_ctrl->fd, amrHeader, sizeof(amrHeader) - 1);
        audio_add(SYS_AUDIO_ENCODE_AMR, audio_encode_save, paudio_record_ctrl);
    }
    else if(strcmp(ext_name, ".wav") == 0)
    {
        paudio_record_ctrl->record_type = SYS_AUDIO_RAW_PCM;
        write_wav_head(paudio_record_ctrl->fd, 1, 8000, 16, 0);
        audio_add(SYS_AUDIO_RAW_PCM, audio_encode_save, paudio_record_ctrl);
    }
}


/**
 * NAME         audio_record_stop
 * @BRIEF     停止录音功能
                   
 * @PARAM   file_name   录音文件的全路径名
 * @RETURN  void 
 * @RETVAL  
 */

void audio_record_stop(char *file_name)
{
    if(paudio_record_ctrl == NULL)
    {
        anyka_print("[%s:%d] it had stopped\n", __func__, __LINE__);
        return ;
    }
    audio_del(paudio_record_ctrl->record_type, paudio_record_ctrl);
    if(paudio_record_ctrl->record_type == SYS_AUDIO_RAW_PCM)
    {
        write_wav_head(paudio_record_ctrl->fd, 1, 8000, 16, paudio_record_ctrl->record_size);
    }
    close(paudio_record_ctrl->fd);
    
    if(file_name)
    {
        strcpy(file_name, paudio_record_ctrl->file_name);
    }
    free(paudio_record_ctrl);
    paudio_record_ctrl = NULL;
    //anyka_print("[%s:%d] the file is %s\n", __func__, __LINE__, file_name);
}



