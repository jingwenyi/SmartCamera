#ifndef _VOICE_TIPS_H_
#define _VOICE_TIPS_H_

/*
* @PATH:传入歌曲路径
* @enc_type:传入歌曲解码类型	
*/

/**
* @brief	voice_tips_add_music
			外部通过调用该接口添加要播放的文件
* @date 	2015/3
* @param:	void *file_name，音频文件文件名，绝对路径
* @return	int
* @retval	success --> 0; failed --> -1
*/
int voice_tips_add_music(void *file_name);

/**
* @brief 	voice_tips_init
			提示音模块初始化函数
* @date 	2015/3
* @param:	void 
* @return 	int
* @retval 	success --> 0; failed --> -1
*/

int voice_tips_init();

/**
* @brief 	voice_tips_stop
			提示音模块停止接口
* @date 	2015/3
* @param:	void 
* @return 	void
* @retval 	
*/

void voice_tips_stop();

/**
* @brief 	voice_tips_destroy
			提示音模块真正停止接口，通过调用它释放资源
* @date 	2015/3
* @param:	void 
* @return 	void
* @retval 	
*/
void voice_tips_destroy();


/**
* @brief 	voice_tips_get_file
			提示音模块获取要播放的文件名，主要是和脚本配合使用
* @date 	2015/3
* @param:	void 
* @return 	void
* @retval 	
*/
void voice_tips_get_file();

#endif


