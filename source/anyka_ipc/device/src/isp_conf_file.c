#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "isp_conf_file.h"


#define ISP_DAY_CONFIG_FILE_PATH	"/etc/jffs2/isp.conf"
#define ISP_NIGHT_CONFIG_FILE_PATH	"/etc/jffs2/isp_night.conf"


static FILE *fd_day = NULL;
static FILE *fd_night = NULL;


T_BOOL ISP_Conf_FileCreate(T_BOOL bDayFlag)
{
	if (bDayFlag)
	{
		if (NULL != fd_day)
		{
			fclose(fd_day);
		}

		fd_day = fopen(ISP_DAY_CONFIG_FILE_PATH, "w+b");

		if (NULL == fd_day)
		{
			printf("ISP_Conf_FileCreate day Failed!\n");
			return AK_FALSE;
		}
	}
	else
	{
		if (NULL != fd_night)
		{
			fclose(fd_night);
		}

		fd_night = fopen(ISP_NIGHT_CONFIG_FILE_PATH, "w+b");

		if (NULL == fd_night)
		{
			printf("ISP_Conf_FileCreate night Failed!\n");
			return AK_FALSE;
		}
	}

	return AK_TRUE;
}

T_VOID ISP_Conf_FileClose(T_BOOL bDayFlag)
{
	if (bDayFlag)
	{
		if (NULL != fd_day)
		{
			fclose(fd_day);
			fd_day = NULL;
		}
	}
	else
	{
		if (NULL != fd_night)
		{
			fclose(fd_night);
			fd_night = NULL;
		}
	}
	
}


T_BOOL ISP_Conf_FileRead(T_BOOL bDayFlag, T_U8* buf, T_U32 size, T_U32 offset)
{
	FILE *fd = NULL;
	
	if (NULL == buf)
	{
		printf("ISP_Conf_FileRead buf NULL!\n");
		return AK_FALSE;
	}

	if (bDayFlag)
	{
		if (NULL == fd_day)
		{
			fd_day = fopen (ISP_DAY_CONFIG_FILE_PATH, "r+b");
		}

		fd = fd_day;
	}
	else
	{
		if (NULL == fd_night)
		{
			fd_night = fopen (ISP_NIGHT_CONFIG_FILE_PATH, "r+b");
		}

		fd = fd_night;
	}

	if (NULL != fd)
	{
		fseek(fd, offset, SEEK_SET);
		if (size == fread(buf, 1, size, fd))
		{
			return AK_TRUE;
		}
		else
		{
			printf("fd %d read failed!\n", bDayFlag);
		}
	}
	else
	{
		printf("fd %d open failed!\n", bDayFlag);
	}


	return AK_FALSE;
}

T_BOOL ISP_Conf_FileLoad(T_BOOL bDayFlag, T_U8* buf, T_U32* size)
{
	T_BOOL ret = AK_FALSE;
	FILE *fd = NULL;
	
	if (NULL == buf || NULL == size)
	{
		printf("ISP_Conf_FileLoad param NULL!\n");
		return ret;
	}

	if (bDayFlag)
	{
		if (NULL == fd_day)
		{
			fd_day = fopen (ISP_DAY_CONFIG_FILE_PATH, "r+b");
		}
		
		fd = fd_day;
	}
	else
	{
		if (NULL == fd_night)
		{
			fd_night = fopen (ISP_NIGHT_CONFIG_FILE_PATH, "r+b");
		}
		
		fd = fd_night;
	}

	if (NULL != fd)
	{
		fseek(fd, 0, SEEK_END);

		*size = ftell(fd);

		ret = ISP_Conf_FileRead(bDayFlag, buf, *size, 0);
	}
	else
	{
		printf("fd %d open failed!\n", bDayFlag);
	}

	ISP_Conf_FileClose(bDayFlag);

	return ret;
}


T_BOOL ISP_Conf_FileWrite(T_BOOL bDayFlag, T_U8* buf, T_U32 size, T_U32 offset)
{
	FILE *fd = NULL;
	
	if (NULL == buf)
	{
		printf("ISP_Conf_FileWrite buf NULL!\n");
		return AK_FALSE;
	}

	if (bDayFlag)
	{
		if (NULL == fd_day)
		{
			fd_day = fopen (ISP_DAY_CONFIG_FILE_PATH, "r+b");
		}
		
		fd = fd_day;
	}
	else
	{
		if (NULL == fd_night)
		{
			fd_night = fopen (ISP_NIGHT_CONFIG_FILE_PATH, "r+b");
		}
		
		fd = fd_night;
	}
	
	if (NULL != fd)
	{
		fseek(fd, offset, SEEK_SET);
		if (size == fwrite(buf, 1, size, fd))
		{
			return AK_TRUE;
		}
		else
		{
			printf("fd %d write failed!\n", bDayFlag);
		}
	}
	else
	{
		printf("fd %d open failed!\n", bDayFlag);
	}

	return AK_FALSE;
}

T_BOOL ISP_Conf_FileStore(T_BOOL bDayFlag, T_U8* buf, T_U32 size)
{
	T_BOOL ret = AK_FALSE;
	
	if (NULL == buf || 0 == size)
	{
		printf("ISP_Conf_FileStore param NULL!\n");
		return ret;
	}

	if (ISP_Conf_FileCreate(bDayFlag))
	{
		ret = ISP_Conf_FileWrite(bDayFlag, buf, size, 0);
	}
	else
	{
		printf("fd %d open failed!\n", bDayFlag);
	}

	ISP_Conf_FileClose(bDayFlag);


	return ret;
}




