#ifndef _AUDIO_RECORD_H_
#define _AUDIO_RECORD_H_
/**
 * NAME         audio_record_start
 * @BRIEF     开始启动录音功能，目前录像只保存 一个文件，并不做文件分离，
                   
 * @PARAM   audio_path  :录音文件的路径
            ext_name    :录音文件后缀名(".wav" or ".amr")   
 * @RETURN  void 
 * @RETVAL  
 */

void audio_record_start(char *audio_path, char *ext_name);



/**
 * NAME         audio_record_stop
 * @BRIEF     停止录音功能
                   
 * @PARAM   file_name   录音文件的全路径名
 * @RETURN  void 
 * @RETVAL  
 */

void audio_record_stop(char *file_name);

#endif

