/*
** filename: check.cpp
** author: jingwenyi
** date: 2016.07.19
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "check.h"




int CalCrc2( unsigned char* buf)
{
	if(buf == NULL)
		return -1;
	if((0x55 != buf[0]) || (0xaa != buf[1]))
		return -1;
	int i;
	unsigned char crc=0;
	int len = buf[3]+6;
	for(i=0; i<len-1; i++)
	{
		crc += buf[i];
	}

	buf[len-1] = crc;
	return len; //返回buf 的有效长度
}


bool CheckCrc2(unsigned char *buf)
{
	
	if(buf == NULL)
		return false;
	if((0x55 != buf[0]) || (0xaa != buf[1]))
		return false;
	int i;
	unsigned char crc=0;
	int len = buf[3]+6;
	for(i=0; i<len-1; i++)
	{
		crc += buf[i];
	}

	if(buf[len-1] !=  crc)
		return false;
	
	
	return true;
	
}

unsigned char *package_data(unsigned char cmd, unsigned char self_addr,unsigned char* send_data, int send_len, int *len)
{
	if((cmd == 0x55) || (cmd == 0xaa) || (send_len > 256)) return NULL;
	if((send_len > 0) && (send_data == NULL))  return NULL;
	
	int all_len = send_len + 6;
	*len = all_len;
	unsigned char *tmp = (unsigned char *)malloc(all_len);
	tmp[0] = 0x55;
	tmp[1] = 0xaa;
	tmp[2] = self_addr;
	tmp[3] = send_len;
	tmp[4] = cmd;

	if(send_len > 0)
	{
		memcpy(tmp+5, send_data, send_len);
	}

	CalCrc2(tmp);
	return tmp;
}

