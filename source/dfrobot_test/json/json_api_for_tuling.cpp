/*
** filename: json_api_for_tuling.cpp
** author: jingwenyi
** date: 2016.06.27
*/

#include <stdio.h>
#include <string.h>
#include <json/json.h>
#include <sys/file.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <stdint.h>
#include "json/json_api_for_tuling.h"

using namespace std;

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

char *parse_baidu_access_token(char *json_buffer, size_t *size)
{
	Json::Reader  reader;
	Json::Value value;
	char *token = NULL;

	bool flag = false;


	if(json_buffer == NULL) return NULL;

	reader.parse(json_buffer, value);

	flag = value.isMember("error");

	if(flag)
	{
		printf("json_buffer:%s\r\n",json_buffer);
		return NULL;
	}
	
	std::string c_value = value["access_token"].asCString();
	//printf("vaule:%s,%d\r\n",c_value.c_str(), c_value.size());
	*size = c_value.size();
	
	token = (char*) malloc(c_value.size()+1);
	if(token == NULL)
	{
		printf("token malloc failed\r\n");
		return NULL;
	}
	memset(token, 0, c_value.size());
	sprintf(token,"%s",c_value.c_str());
	
	return token;
}



//{"code":100000,"text":"我爱你 爱着你 就像老鼠爱大米"}
//{"code":200000,"text":"�ף��Ѱ����ҵ��г���Ϣ","url":"www.bing.com"}
char *parse_tuling_talk(char *json_buffer, size_t *size)
{
	Json::Reader  reader;
	Json::Value value;
	char *tuling_talk = NULL;

	if(json_buffer == NULL) return NULL;

	reader.parse(json_buffer, value);

	int code = value["code"].asInt();
	//printf("-----------code:%d----------\r\n",code);
	
	std::string c_value;
	switch(code)
	{
		case 100000:  //text data
			c_value = value["text"].asCString();
			break;
			
		case 200000:  //url data
			c_value = value["text"].asCString();
			break;
			
		default:
			;
	}

	*size = c_value.size();
	if((*size)>0)
	{
		tuling_talk = (char *)malloc(c_value.size()+1);
		if(tuling_talk == NULL)
		{
			printf("tuling_talk malloc failed\r\n");
			return NULL;
		}
		memset(tuling_talk, 0, c_value.size());
		sprintf(tuling_talk,"%s",c_value.c_str());
		//change 0x20 to "%20"
		char* tmp  = (char*)malloc(2*strlen(tuling_talk));
		if(tmp == NULL)
		{
			printf("tmp maloc error\r\n");
			free(tuling_talk);
			return NULL;
		}
		memset(tmp, 0, 2*strlen(tuling_talk));
		
		int i,j;
		for(i=0,j=0; i<strlen(tuling_talk); i++)
		{
			if(tuling_talk[i] == 0x20)
			{
				tmp[j++] = '%';
				tmp[j++] = '2';
				tmp[j++] = '0';
			}
			else
			{
				tmp[j++] = tuling_talk[i];
			}
		}
		//printf("j=%d,i=%d,%s,strlen(tmp):%d\r\n", j, i, tmp,strlen(tmp));

		if(j>i)
		{
			free(tuling_talk);
			tuling_talk = (char*)malloc(strlen(tmp)+1);
			if(tuling_talk == NULL)
			{
				printf("tuling talk-- malloc error\r\n");
				free(tmp);
				return NULL;
			}
			memset(tuling_talk, 0, strlen(tmp)+1);
			sprintf(tuling_talk, "%s", tmp);
			
		}
		
		free(tmp);
		tmp = NULL;
		
	}
	//printf("--talk:%s\r\n",tuling_talk);

	return tuling_talk;
	
}


/*
**	{"corpus_no":"6301423799474541215",
**	"err_msg":"success.","err_no":0,
**	"result":["百度语音提供技术支持，"],
**	"sn":"133482666401467164559"}
*/


char *parse_baidu_asr(char* json_buffer, size_t *size)
{
	Json::Reader  reader;
	Json::Value value;

	char *asr_res = NULL;

	if(json_buffer == NULL) return NULL;
	
	reader.parse(json_buffer, value);

	int err_no = value["err_no"].asInt();
	//printf("err_no=%d\r\n",err_no);
	if(err_no != 0)
	{
		printf("baidu asr respond error:%d\r\n",err_no);
		return NULL;
	}
#if 0
	std::string c_value = value["result"][(unsigned int)0].asCString();
	*size = c_value.size();
	asr_res = (char*)malloc(c_value.size()+1);
	memset(asr_res, 0, c_value.size());
	sprintf(asr_res, c_value.c_str());
	//printf("asr_res:%s, strlen(asr_res)=%d,--size=%d,*size=%d\r\n", asr_res, strlen(asr_res), c_value.size(), *size);
#endif
	int count = value["result"].size();
	//printf("size=%d\r\n",count);
	std::string c_value;
	int i, data_size=0;
	char *tmp = (char*)malloc(1*1024*1024);
	if(tmp == NULL)
	{
		printf("tmp -- malloc error\r\n");
		return NULL;
	}
	memset(tmp, 0, 1*1024*1024);
	for(i=0; i<count; i++)
	{
		c_value = value["result"][(unsigned int )i].asCString();
		sprintf(&tmp[data_size],"%s",c_value.c_str());
		data_size += c_value.size();
	}
	*size = data_size;

	asr_res = (char*)malloc(data_size+1);
	if(asr_res == NULL)
	{
		printf("asr res malloc error\r\n");
		free(tmp);
		return NULL;
	}
	memset(asr_res, 0, (data_size+1));
	memcpy(asr_res, tmp, data_size);

	free(tmp);

	return asr_res;
	
}





