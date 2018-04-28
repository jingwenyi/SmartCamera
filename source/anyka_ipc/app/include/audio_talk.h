#ifndef _audio_talk_h_
#define _audio_talk_h_
typedef void * Pnet_get_talk_data(void * user );

/**
 * NAME         audio_speak_start
 * @BRIEF	开启对讲功能,这个函数完成将AD数据采集，并编码送给网络
 * @PARAM	mydata    用户数据
                    pget_callback       得到网络数据的回调
                    encode_type     编码类型
 * @RETURN	void
 * @RETVAL	
 */

void audio_speak_start(void *mydata, AUDIO_SEND_CALLBACK *pcallback, int encode_type);


/**
 * NAME         audio_speak_stop
 * @BRIEF	停止对讲功能,关闭AD数据编码
 * @PARAM	encode_type     编码类型
 * @RETURN	void
 * @RETVAL	
 */

void audio_speak_stop(void *mydata, int encode_type);

/**
 * NAME         audio_talk_start
 * @BRIEF	开启对讲功能，这个函数完成接受网络数据，并解码，送给DA
 * @PARAM	mydata    用户数据
                    pget_callback       得到网络数据的回调
                    encode_type     编码类型
 * @RETURN	0--fail; 1-->ok
 * @RETVAL	
 */

uint8 audio_talk_start(void *mydata, Pnet_get_talk_data pget_callback, int encode_type);


/**
 * NAME         audio_talk_status
 * @BRIEF	检查编码缓冲区是不是为空的状态
 * @PARAM	
 * @RETURN	0-->为空，1-->不为空 
 * @RETVAL	
 */

int audio_talk_status(void);



/**
 * NAME         audio_talk_stop
 * @BRIEF	停止对讲功能,将不再接受网络数据
 * @PARAM	
 * @RETURN	void
 * @RETVAL	
 */
void audio_talk_stop(void *mydata);

#endif


