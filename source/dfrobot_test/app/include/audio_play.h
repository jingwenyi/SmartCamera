#ifndef _AUDIO_PLAY_H_
#define _AUDIO_PLAY_H_

/**
 * NAME         audio_play_start
 * @BRIEF	开始AMR的音乐播放，目前只做了AMR文件的播放
 * @PARAM	audio_path   AMR音乐路径
                    del_flag 是否删除文件
 * @RETURN	NONE
 * @RETVAL	
 */


void audio_play_start(char *audio_path, int del_flag);


/**
 * NAME         audio_play_stop
 * @BRIEF	停止当前音乐的播放
 * @PARAM	void
 * @RETURN	NONE
 * @RETVAL	
 */

void audio_play_stop(void);

#endif

