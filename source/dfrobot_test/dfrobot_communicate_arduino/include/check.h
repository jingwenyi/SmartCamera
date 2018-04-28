/*
** filename:check.h
** author: jingwenyi
** date: 2016.07.19
*/

#ifndef _CHECK_H
#define  _CHECK_H

int CalCrc2(unsigned char* buf);
bool CheckCrc2(unsigned char *buf);
unsigned char *package_data(unsigned char cmd, 
	unsigned char self_addr,unsigned char* send_data, int send_len, int *len);


#endif //_CHECK_H

