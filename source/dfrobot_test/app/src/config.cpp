/*
** filename: read_config.cpp
** author: jingwenyi
** date: 2016.08.04
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"

static char *compare_str(const char *src, const char* line)
{
	
	int len = strlen(line);
	int src_len = strlen(src);
	const char *tmp = src;
	const char *ptr = line;
	char *ptr1 = NULL; 
	int flag = 0;
	int i,j,ii,jj;

	if((src == NULL) || (line == NULL)) return NULL;

	//if '#' in fisrt ,ignore
	for(i=0; i<len; i++)
	{
		if(ptr[i] == '#')
		{
			return 	NULL;
		}
		else if(ptr[i] <= ' ')
		{
			continue;
		}

		break;
	}

	
	
	for(i=0; i<len; i++)
	{
		if(ptr[i] == tmp[0])
		{
			for(j=0; j<src_len; j++)
			{
				if(ptr[i+j] != tmp[j])
					break;
				if(j == (src_len-1))
				{
					flag = 1;
				}
			}
			
			if(flag)
			{
				ii = i;
				break;
			}
		}
	}

	if(flag)
	{
		//printf("%s\r\n",ptr);
		//printf("%s\r\n",&ptr[ii]);
		ii += src_len;
		//search '='
		for(i=0; i<(len -ii-1); i++)
		{
			//printf("ii+i=%c\r\n",ptr[ii+i]);
			if(ptr[ii+i] == '=')
			{
				jj = ii+i;
				//start get char 
				char *des = (char *)malloc(len);
				if(des == NULL)
				{
					printf("des malloc error\r\n");
					return NULL;
				}
				memset(des, 0, len);
				
				for(j=1; j<=(len-jj); j++)
				{
					
					if(ptr[jj+j] > ' ')
					{
						int k=0;
						while((ptr[jj+j+k]) > ' ')
						{
							des[k] = ptr[jj+j+k];
							k++;
						}
						//printf("des:%s\r\n",des);
						return des;
					}
					else
					{
						continue;
					}
				}
				
			}
			else if(ptr[ii+i] <= ' ')
			{
				continue;
			}

			return NULL;
		}
	}


	return NULL;
		
}

char *read_element(const char *src, const char* filename)
{
	
	char line[1024] = {0};
	int len = 0;
	char *des = NULL;

	if((src == NULL) || (filename == NULL)) return NULL;
	
	FILE *config = fopen(filename, "r");
	if(config == NULL)
	{
		printf("open %s file failed\r\n", filename);
		return NULL;
	}

	while(!feof(config))
	{
		memset(line, 0, 1024);
		if(fgets(line, 1024, config) == NULL)
		{
			break;
		}

		//printf("%s\r\n",line);

		len = strlen(line);
		des = compare_str(src, line);
		if(des != NULL)
		{
			fclose(config);
			return des;
		}
	}

	fclose(config);
	return NULL;

}

//if sucessful return 0
static int search_str(const char *src, const char *line)
{
	int len = strlen(line);
	int src_len = strlen(src);
	const char *tmp = src;
	const char *ptr = line;
	char *ptr1 = NULL; 
	int flag = 0;
	int i,j,ii,jj;

	if((src == NULL) || (line == NULL)) return -1;

	//if '#' in fisrt ,ignore
	for(i=0; i<len; i++)
	{
		if(ptr[i] == '#')
		{
			return 	-1;
		}
		else if(ptr[i] <= ' ')
		{
			continue;
		}

		break;
	}

	for(i=0; i<len; i++)
	{
		if(ptr[i] == tmp[0])
		{
			for(j=0; j<src_len; j++)
			{
				if(ptr[i+j] != tmp[j])
					break;
				if(j == (src_len-1))
				{
					flag = 1;
				}
			}
			
			if(flag)
			{
				ii = i;
				break;
			}
		}
	}

	if(flag)
	{
		ii += src_len;
		//search '='
		for(i=0; i<(len -ii-1); i++)
		{
			if(ptr[ii+i] == '=')
			{
				return 0;
				
			}
			else if(ptr[ii+i] <= ' ')
			{
				continue;
			}

			return -1;
		}
	}
	
	return -1;
}

// if sucessful return 0
int write_element(const char *des, const char *content, const char *filename)
{

	char tmp_name[100] = {0};
	char line[1024] = {0};

	if((des == NULL) || (content == NULL) || (filename == NULL))
	{
		return -1;
	}
	
	sprintf(tmp_name, "%s.tmp",filename);
	FILE *config = fopen(filename, "r");
	if(config == NULL)
	{
		printf("open %s file failed\r\n", filename);
		return -1;
	}

	FILE *tmp_config = fopen(tmp_name, "w+");
	if(tmp_config == NULL)
	{
		printf("open %s file failed\r\n", tmp_name);
		fclose(config);
		return -1;
	}

	
	while(!feof(config))
	{
		memset(line, 0, 1024);
		if(fgets(line, 1024, config) == NULL)
		{
			break;
		}

		//printf("%s\r\n",line);
		if(!search_str(des, line))
		{
			memset(line, 0, 1024);
			sprintf(line, "%s   =    %s\n", des, content);
		}
		
		fwrite(line, 1, strlen(line), tmp_config);
	}

	fclose(config);
	fclose(tmp_config);
#if 1

	memset(line, 0, 1024);
	sprintf(line, "rm -rf %s", filename);
	system(line);

	memset(line, 0, 1024);
	sprintf(line, "mv %s %s", tmp_name, filename);
	system(line);	
#endif
	
	return 0;
}



