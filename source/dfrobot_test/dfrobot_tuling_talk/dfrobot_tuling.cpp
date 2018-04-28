/*
** filename: dfrobot_tuling.c
** author: jingwenyi
** date: 2016.06.24
**
*/


#include "dfrobot_tuling.h"
#include "json/json_api_for_tuling.h"


#define	BAIDU_TTS_TEXT_MAX  1024
#define AUDIO_TYPE			"pcm"
#define AUDIO_RATE			"16000"

static char *access_tocken = NULL;


static char *baiduApiId = NULL;
static char *baiduApiKey = NULL;
static char *baiduApiSecrtkey = NULL;
static char *tulingApiKey = NULL;
static char *tulingUserKey = NULL;

static bool user_config_baidu = false;
static bool user_config_tuling = false;

bool set_baidu_api(const char *ApiId, const char *ApiKey, const char *ApiSecrtkey)
{
	int len = 0;
	if((ApiId == NULL) || (ApiKey == NULL) || (ApiSecrtkey == NULL))
		return false;
	len = strlen(ApiId) + 1;
	baiduApiId = (char *)malloc(len);
	if(baiduApiId == NULL)
	{
		printf("set baidu api malloc failed\r\n");
		return false;
	}
	memcpy(baiduApiId, ApiId, len);

	len = strlen(ApiKey) + 1;
	baiduApiKey = (char *)malloc(len);
	if(baiduApiKey == NULL)
	{
		free(baiduApiId);
		baiduApiId = NULL;
		printf("set baidu api malloc failed\r\n");
		return false;
	}
	memcpy(baiduApiKey, ApiKey, len);

	len = strlen(ApiSecrtkey) + 1;
	baiduApiSecrtkey = (char *)malloc(len);
	if(baiduApiSecrtkey == NULL)
	{
		free(baiduApiId);
		baiduApiId = NULL;
		free(baiduApiKey);
		baiduApiKey = NULL;
		printf("set baidu api malloc failed\r\n");
		return false;
	}
	memcpy(baiduApiSecrtkey, ApiSecrtkey, len);

	user_config_baidu = true;
	return true;
}


bool set_tuling_api(const char *apikey, const char *userkey)
{
	int len = 0;
	if((apikey == NULL) || (userkey == NULL))
		return false;

	len = strlen(apikey)+1;
	tulingApiKey = (char *)malloc(len);
	if(tulingApiKey == NULL)
	{
		printf("set tuling api malloc error\r\n");
		return false;
	}
	memcpy(tulingApiKey, apikey, len);

	len = strlen(userkey) + 1;
	tulingUserKey = (char *)malloc(len);
	if(tulingUserKey == NULL)
	{
		free(tulingApiKey);
		tulingApiKey = NULL;
		printf("set tuling api malloc err\r\n");
		return false;
	}
	memcpy(tulingUserKey, userkey, len);

	user_config_tuling = true;

	return true;
	
}


void tuling_exit()
{
	if(baiduApiId != NULL)
	{
		free(baiduApiId);
		baiduApiId = NULL;
	}

	if(baiduApiKey != NULL)
	{
		free(baiduApiKey);
		baiduApiKey = NULL;
	}

	if(baiduApiSecrtkey != NULL)
	{
		free(baiduApiSecrtkey);
		baiduApiSecrtkey = NULL;
	}

	if(tulingApiKey != NULL)
	{
		free(tulingApiKey);
		tulingApiKey = NULL;
	}

	if(tulingUserKey != NULL)
	{
		free(tulingUserKey);
		tulingUserKey = NULL;
	}

	if(access_tocken != NULL)
	{
		free(access_tocken);
		access_tocken = NULL;
	}

	user_config_tuling = false;
	user_config_baidu = false;
}


#define  WIRTE_FUNC(name,json_parse)															\
	static  size_t name##_write_func(void *ptr, size_t size, size_t nmemb, char **result) 		\
	{ 																							\
		char *tmp = NULL;																		\
		size_t len = 0;																			\
		size_t result_len = size*nmemb;     													\
		tmp = (char*)malloc(result_len+1);     													\
		if(tmp == NULL)     																	\
		{																						\
			printf("*tmp malloc error\r\n");      												\
			return 0;                                 	 										\
		}                                           											\
		memcpy(tmp, ptr, result_len);   														\
		tmp[result_len] = '\0';   																\
		if(*result != NULL)																		\
		{																						\
			free(*result);																		\
		}																						\
		(*result) = json_parse(tmp, &len);														\
		free(tmp);																				\
		return len;    																			\
	}
		


WIRTE_FUNC(access_tocken, parse_baidu_access_token)
WIRTE_FUNC(baidu_asr, parse_baidu_asr)
WIRTE_FUNC(talk_to_tuling, parse_tuling_talk)

#undef WIRTE_FUNC





char* get_baidu_api_access_tocken()
{
	CURL *curl;
	CURLcode res;
	
	

	if(access_tocken != NULL)
	{
		return access_tocken;
	}

	curl = curl_easy_init();

	if(curl)
	{

		if(user_config_baidu)
		{
			char tmp[500] = {0};
			sprintf(tmp,"https://openapi.baidu.com/oauth/2.0/token?grant_type=client_credentials&client_id=%s"
				"&client_secret=%s", baiduApiKey, baiduApiSecrtkey);
			curl_easy_setopt(curl, CURLOPT_URL, tmp);
		}
		else
		{
	
			curl_easy_setopt(curl, CURLOPT_URL,
				"https://openapi.baidu.com/oauth/2.0/token?grant_type=client_credentials&client_id="DFROBOT_BAIDU_APIKEY
				"&client_secret="DFROBOT_BAIDU_APISECRTKEY);
		}
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		//Ö¸Ã÷stream µØÖ·
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &access_tocken);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, access_tocken_write_func);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	

	return access_tocken;

}



void talk_to_talk_with_tuling(uint8_t *pcm_data, int pcm_data_len, curl_callback_func *func)
{
	char *say_to_tuling = NULL;
	char *tuling_say = NULL;

	if(pcm_data == NULL)
	{
		printf("no audio data\r\n");
		goto out;
	}
	
	say_to_tuling = baidu_asr_pcm2text(pcm_data, pcm_data_len);
	
	
	if(say_to_tuling == NULL)
	{
		printf("baidu asr has no respond data\r\n");
		goto out;
	}
	
	printf("say_to_tuling:%s\r\n",say_to_tuling);

	
	tuling_say = talk_to_tuling(say_to_tuling);

	if(tuling_say == NULL)
	{
		printf("tuling has no say anything \r\n");
		goto out;
	}
	printf("tuling_say:%s\r\n",tuling_say);

	baidu_tts_text2mp3(tuling_say, func);
	

out:
	if(say_to_tuling != NULL)
	{
		free(say_to_tuling);
		say_to_tuling = NULL;
	}
	if(tuling_say != NULL)
	{
		free(tuling_say);
		tuling_say = NULL;
	}
	
	
}


char *baidu_asr_pcm2text(uint8_t *pcm_data, int pcm_data_len)
{
	CURL *curl;
	CURLcode res;
	char *request = NULL;
	char *result_buffer = NULL;

	if(pcm_data == NULL)
	{
		printf("pcm data = null \r\n");
		return NULL;
	}
	get_baidu_api_access_tocken();

	int i=1000;
	while((access_tocken == NULL) && (i>0)) i--;

	if(access_tocken == NULL)
	{
		printf(" no access tocken \r\n");
		return NULL;
	}

	request = (char*)malloc(200);
	if(request == NULL)
	{
		printf("request malloc failed \r\n");
		return NULL;
	}

	sprintf(request, "http://vop.baidu.com/server_api?lan=zh&cuid=xxx&token=%s",access_tocken);

	curl = curl_easy_init();

	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, request);
		curl_easy_setopt(curl, CURLOPT_POST, 1);
	
		//add data type to http header
		char type_buffer[200] = {0};
		sprintf(type_buffer,"Content-Type: audio/%s; rate=%s", AUDIO_TYPE, AUDIO_RATE);
		//struct curl_slist *headerlist = curl_slist_append(NULL,"Content-Type: audio/pcm; rate=8000");
		struct curl_slist *headerlist = curl_slist_append(NULL,type_buffer);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
		
		//add data length to http header
		char len_buffer[50]= {0};
		sprintf(len_buffer, "Content-Length: %d", pcm_data_len);
		headerlist = curl_slist_append(headerlist,len_buffer);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

		//add data 
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, pcm_data);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, pcm_data_len);

		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result_buffer);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, baidu_asr_write_func);

		res = curl_easy_perform(curl);

		curl_slist_free_all(headerlist);
		
		curl_easy_cleanup(curl);
		
		
		
	}
	else
	{
		printf("culr init error\r\n");
		free(request);
		return NULL;
	}
	
	free(request);

	
	int count = 10000;
	while((result_buffer == NULL) && (count > 0)) count--;
		
	return result_buffer;
	
}


char *talk_to_tuling(char *text)
{
	char *request = NULL;
	char *result_buffer = NULL;

	CURL *curl;
	CURLcode res;
	int len;
	
	if(text == NULL)
	{
		printf("say to tuling = NULL\r\n");
		return NULL;
	}
	
	len = strlen(text);
	
	request = (char*) malloc((100+len));
	if(request == NULL)
	{
		printf("request = NULL\r\n");
		return NULL;
	}

	if(user_config_tuling)
	{
		sprintf(request,"http://www.tuling123.com/openapi/api?key=%s&info=%s&userid=%s",
						tulingApiKey, text, tulingUserKey);
	}
	else
	{
		sprintf(request,"http://www.tuling123.com/openapi/api?key=%s&info=%s&userid=%s",
						DFROBOT_TULING_APIKEY, text, DFROBOT_TULING_USERKEY);
	}
	
	curl = curl_easy_init();

	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, request);
		//Ö¸Ã÷stream µØÖ·
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result_buffer);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, talk_to_tuling_write_func);
		
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	else
	{
		printf("curl init error\r\n");
		free(request);
		return  NULL;
	}
	free(request);

	
	int count = 10000;
	while((result_buffer == NULL) && (count > 0)) count--;
		
	return result_buffer;
	
	
	
}


void baidu_tts_text2mp3(char *text, curl_callback_func *func)
{
	
	CURL *curl;
	CURLcode res;
	char *request = NULL;
	int i;
		
	if(text == NULL)
	{
		printf("text data = NULL\r\n");
		return ;
	}
	if(strlen(text) > BAIDU_TTS_TEXT_MAX)
	{
		printf("you input text too many, bai du tts only accept 1024 bytes\r\n");
		return ;
	}
		
	get_baidu_api_access_tocken();
		
	i=1000;
	while((access_tocken == NULL) && (i>0)) i--; 
	//printf("i=%d,tocken=%s\r\n",i,access_tocken);
		
	if(access_tocken == NULL)
	{
		printf("access tocken = NULL\r\n");
		return ;
	}
	
	request = (char*) malloc(BAIDU_TTS_TEXT_MAX+200);
	if(request == NULL)
	{
		printf("request buffer malloc error\r\n");
		return ;
	}
		
	sprintf(request, "http://tsn.baidu.com/text2audio?tex=%s&lan=zh&cuid=xxx&ctp=1&tok=%s&spd=5&pit=5&vol=9&per=0",
							text,access_tocken);
	
	//printf("%s\r\n",request);
	
	curl = curl_easy_init();
	
	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, request);
		//Ö¸Ã÷stream µØÖ·
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, *func);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	else
	{
		printf("curl init error\r\n");
	}
		
	free(request);
}



