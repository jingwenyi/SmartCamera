/*
** author : 	wenyi.jing 
** date: 	2016.05.11
** company :dfrobot
*/
#include <stdio.h>
#include <string.h>
#include <json/json.h>
#include <sys/file.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <stdint.h>
#include "json/json_api.h"
//#include "../colorlut.h"



using namespace std;

#define CL_NUM_SIGNATURES               7





enum return_type
{
	ME_OK,
	OPEN_FILE_ERROR,
	READ_FILE_ERROR,
	WRITE_FILE_ERROR
	
};



ColorSignature *json_load_file(const char *file_name)
{
	ColorSignature *json_signatures_array;
	char signatures[4096];
	Json::Reader reader;
	Json::Value value;
	int i, signum;
	
	json_signatures_array = (ColorSignature *)malloc(sizeof(ColorSignature)*CL_NUM_SIGNATURES);
	 memset((void *)json_signatures_array, 0, sizeof(ColorSignature)*CL_NUM_SIGNATURES);
	
	long fd = open(file_name, O_RDWR | O_CREAT, 0777);
	if(fd <= 0)
	{
		printf("open file failed\r\n");
		return NULL;
	}
	read(fd, signatures, 4096);
	close(fd);

	reader.parse(signatures, value);

	for(i=0; i<value["signatures"].size(); i++)
	{
		signum = value["signatures"][(unsigned int)i]["lable"].asInt();
		json_signatures_array[signum-1].m_uMin = value["signatures"][(unsigned int)i]["m_uMin"].asInt();
		json_signatures_array[signum-1].m_uMax = value["signatures"][(unsigned int)i]["m_uMax"].asInt();
		json_signatures_array[signum-1].m_uMean = value["signatures"][(unsigned int)i]["m_uMean"].asInt();
		json_signatures_array[signum-1].m_vMin = value["signatures"][(unsigned int)i]["m_vMin"].asInt();
		json_signatures_array[signum-1].m_vMax = value["signatures"][(unsigned int)i]["m_vMax"].asInt();
		json_signatures_array[signum-1].m_vMean = value["signatures"][(unsigned int)i]["m_vMean"].asUInt();
		json_signatures_array[signum-1].m_rgb = value["signatures"][(unsigned int)i]["m_rgb"].asUInt();
		json_signatures_array[signum-1].m_type = value["signatures"][(unsigned int)i]["m_type"].asUInt();
	}
	
	return json_signatures_array;
}




int json_write_file(const ColorSignature *json_signatures_array, const char *file_name)
{
	Json::Value  item;
	Json::Value arrayObj;
	Json::Value root;
	int i;

	
	
	for(i=0; i<CL_NUM_SIGNATURES; i++)
	{
		if((json_signatures_array[i].m_uMean == 0) && (json_signatures_array[i].m_vMean == 0))
		{
			continue;
		}
		item["lable"] = i+1;
		item["m_uMin"] = json_signatures_array[i].m_uMin;
		item["m_uMax"] = json_signatures_array[i].m_uMax;
		item["m_uMean"] = json_signatures_array[i].m_uMean;
		item["m_vMin"] = json_signatures_array[i].m_vMin;
		item["m_vMax"] = json_signatures_array[i].m_vMax;
		item["m_vMean"] = json_signatures_array[i].m_vMean;
		item["m_rgb"] = json_signatures_array[i].m_rgb;
		item["m_type"] = json_signatures_array[i].m_type;
		arrayObj.append(item);
	}

	root["signatures"] = arrayObj;

	long fd = open(file_name, O_RDWR | O_CREAT, 0777);
	if(fd <= 0)
	{
		printf("open file failed\r\n");
		return OPEN_FILE_ERROR;
	}

	std::string save_string = root.toStyledString();

	write(fd, save_string.c_str(), save_string.size());

	close(fd);

	return ME_OK;
	
}






