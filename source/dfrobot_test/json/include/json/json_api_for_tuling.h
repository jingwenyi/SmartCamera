/*
** filename: json_api_for_tuling.h
** author: jingwenyi
** date: 2016.06.27
**
*/

#ifndef  _JSON_API_FOR_TULING_H_
#define  _JSON_API_FOR_TULING_H_

/*
**  baidu tts  back access_token
**	{"access_token":"24.9e4308ad33f2599a974eab200fa9235a.2592000.1469584009.282335-5882776",
**	  "session_key":"9mzdAqcPJLqxmFfokub8nHn1JTT8\/9hUBI\/TkP8P2IHqq4Kf5o2R+mpXTLdKFbeO4lajC72KXznC7HV0VyIvb35B\/6lj",
**	  "scope":"public audio_voice_assistant_get audio_tts_post wise_adapt lebo_resource_base lightservice_public hetu_basic lightcms_map_poi kaidian_kaidian",
**	  "refresh_token":"25.8df2d5fc8dbf67248761ea7fe116b96a.315360000.1782352009.282335-5882776",
**	  "session_secret":"751c0945317d261d5c49a0746e510f42",
**	  "expires_in":2592000
**	 }
*/
char *parse_baidu_access_token(char *json_buffer, size_t *size);

/*
** 
**{"code":100000,"text":"我爱你 爱着你 就像老鼠爱大米"}
**
*/
char *parse_tuling_talk(char *json_buffer, size_t *size);

char *parse_baidu_asr(char* json_buffer, size_t *size);


#endif  //_JSON_API_FOR_TULING_H_


