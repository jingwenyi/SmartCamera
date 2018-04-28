#ifndef _audio_decode_h_
#define _audio_decode_h_

#define 	AUDIO_DECODE_MAX_SZ 2048

typedef struct media_info
{
	int m_AudioType;
	int m_nSamplesPerSec;
	int m_nChannels;
	int m_wBitsPerSample;
}media_info;

/**
* @brief  audio_decode_get_info
* 		 获取对应句柄的信息
* @author 	
* @date 	2015/3
* @param:	void *handle,	指定要获取数据的句柄 
			T_AUDIO_INPUT *out_info	， 指向存放输出结果的buf 的指针
* @return 	int
* @retval 	0 --> failed, 1--> success
*/

int audio_decode_get_info(void *handle, T_AUDIO_INPUT *out_info);

/**
* @brief  audio_decode_get_free
* 		 获取句柄所指向的解码缓冲区剩余空间
* @author 	
* @date 	2015/3
* @param:	void *handle,	指定要获取数据的句柄 
* @return 	int
* @retval 	0 --> failed, 1--> success
*/

int audio_decode_get_free(void * handle);


/**
* @brief  audio_decode_get_info
* 		 获取对应句柄的信息
* @author 	
* @date 	2015/3
* @param:	void *handle,	指定要获取数据的句柄 
			T_AUDIO_INPUT *out_info	， 指向存放输出结果的buf 的指针
* @return 	int
* @retval 	0 --> failed, 1--> success
*/

void * audio_decode_open(media_info *pmedia);

/**
* @brief  audio_decode_add_data
* 		 往句柄所指向的解码缓冲区添加待解码数据
* @author 	
* @date 	2015/3
* @param:	void *handle，	open时返回的句柄 
			uint8 * in_buf，指向输入数据的指针
			int in_sz， 添加的大小
* @return 	int
* @retval 	0 --> failed, 1--> success
*/

int audio_decode_add_data(void *handle, uint8 *in_buf, int in_sz);

/**
* @brief  audio_decode_decode
* 		解码函数
* @date 	2015/3
* @param:	void *handle，	open时返回的句柄 
			uint8 *out_buf，指向存放解码输出结果的指针
			uint8 channel，数据通道数，1-->单声道，2-->双声道
* @return 	int
* @retval 	0 --> failed, success-->返回成功解码的数据大小
*/

int audio_decode_decode(void *handle, uint8 *out_buf, uint8 channel);

/**
* @brief  audio_decode_close
* 		关闭解码
* @date 	2015/3
* @param:	void *handle，	open时返回的句柄 
* @return 	void
* @retval 	
*/

void audio_decode_close(void *handle);

/**
* @brief  audio_decode_change_mode
* 		 设置句柄的解码属性，当前设置其无需等待数据满4K，只要有数据就能够解码
* @date 	2015/3
* @param:	void *handle，	open时返回的句柄 
* @return 	void
* @retval 	
*/

void audio_decode_change_mode(void * handle);


/**
* @brief  audio_decode_set_buf
* 		 设置句柄所指向的时解码缓冲最小延迟长度.
* @date 	2015/3
* @param:	void *handle，	open时返回的句柄 
			int buf_lenf，要设置的长度
* @return 	void
* @retval 	
*/

void audio_decode_set_buf(void * handle, int buf_len);



#endif
