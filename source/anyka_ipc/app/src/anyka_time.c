
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include "common.h"

/***** GETTIME ******/
long gettime(void)
{
	time_t t;
	t = time(NULL);		//get time store at t;

	//t -= (8 * 3600);		//减少8个小时，返回格林尼治时间

	return (long)t;
}

/******** set device time *********/
int settime(T_S64 time_arg)
{
	time_t t;
	t = (time_t)time_arg;

	struct tm *str;
	//str = localtime(&t);
	str = gmtime(&t);
	if(str == NULL)
	{
		anyka_print("get local time error\n");
		//exit(-1);	
		return -1;
	}

	/** construc cmd: date year-mon-mday hour:min:sec **/
	char cmd[256] = {0};	//cmd buffer
	sprintf(cmd, "date \"%d-%d-%d %d:%d:%d\"", str->tm_year+1900, str->tm_mon+1, str->tm_mday,
						str->tm_hour+8, str->tm_min, str->tm_sec);

	if(system(cmd) < 0)			//system cmd,execute "system date \XX-XX-XX XX:XX:XX\"
	{	
		return -1;	
	}
	bzero(cmd, sizeof(cmd));  //clean buffer

    //save to rtc
	sprintf(cmd, "hwclock -w");
	if(system(cmd) < 0)
	{	
		return -1;	
	}

	return 0;
}

