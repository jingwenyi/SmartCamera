/**
 * @filename updater.c
 * @brief for linux update use
 * Copyright (C) 2010 Anyka (Guangzhou) Software Technology Co., LTD
 * @author zhangshenglin
 * @date 2012-12-07
 * @version 1.0
 * @ref 
 */
/*how to use*/
/*
 *			local K=./zImage B=./nandboot.bin L=./logo.bmp D=./ddrpar.txt
 *./updater http K=http://www.a.com/zI B=http://www.a.com/nb.bin L=http://www.a.com/l.bmp X=1 D=./ddrpar.txt
 *			ftp K=/path/file1 B=/path/file2 L=/path/file3 A=a.b.c.d P=port U=aaa C=xxx X=1 D=./ddrpar.txt
 *
 * local:the file is on local
 * http:the file is on http server
 * ftp:the file is on ftp server
 *
 * K:kernel
 * B:nandboot
 * L:logo
 * A:ip addr
 * P:port
 * U:username
 * C:password
 * MTD[x]:update mtd[x]
 * */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/fcntl.h>
#include "fha_char_interface.h"
#include "httpclient.h"
#include "ftpclient.h"

#define KERNEL_FILE "zImage"
#define BOOT_FILE "nandboot.bin"
#define LOGO_FILE "logo.bmp"
int bNeedCheckFile = 1;//是否需要校验


#define CAMERA			"/etc/init.d/rcS"

#define SEM_PROJ_ID	0x23
#define SEM_NUMS		12

#define KERNEL_UPDATE_OP						0
#define KERNEL_UPDATE_OP_DOWNLOAD		1
#define KERNEL_UPDATE_SD						2
#define KERNEL_UPDATE_DOWNLOAD			3
#define KERNEL_UPDATE_CAN_BURN_KER	4
#define KERNEL_UPDATE_FILE					5

#define APP_UPDATE_OP								6
#define APP_UPDATE_OP_DOWNLOAD			7
#define APP_UPDATE_OP_SD						8
#define APP_UPDATE_DOWNLOAD					9
#define APP_UPDATE_CAN_BURN_APP			10
#define APP_UPDATE_FILE							11

key_t sem_key;
int sem_id;

union semun{
	int			val;		//semctl SETVAL
	struct semid_ds *	buf;		//semctl IPC_STAT, IPC_SET
	unsigned short	*	array; 	//Array for GETALL, SETALL
	struct seminfo	*	 __buf;	//IPC_INFO
};

int g_bIsSem = 0;


/*****************************************************************
 *@brief:update the bootloader
 *@author:zhangshenglin
 *@date:2012-12-07
 *@param filename:boot file name
 *@return:int
 *@retval:0:success/ other value:fail
 ******************************************************************/
static int UpdateBoot(const char* filename, const char* dlinkfilename)
{
	return fha_interface_UpdateBoot(filename, dlinkfilename);
}

/*****************************************************************
 *@brief:update the kernel
 *@author:zhangshenglin
 *@date:2012-12-07
 *@param filename:kernel file name
 *@return:int
 *@retval:0:success/ other value:fail
 ******************************************************************/
static int UpdateKernel(const char* filename)
{
	return fha_interface_UpdateBin(filename, "BIOS");
}

/*****************************************************************
 *@brief:update the logo
 *@author:zhangshenglin
 *@date:2012-12-07
 *@param filename:logo file name
 *@return:int
 *@retval:0:success/ other value:fail
 ******************************************************************/
static int UpdateLogo(const char* filename)
{
	return fha_interface_UpdateBin(filename, "LOGO");
}

/*****************************************************************
 *@brief:update the mtd
 *@author:zhangshenglin
 *@date:2012-12-07
 *@param filename:root file name
 *@param partition: partition
 *@return:int
 *@retval:0:success/ other value:fail
 ******************************************************************/
static int UpdateMTD(const char* filename, int partition)
{
	return fha_interface_UpdateMTD(filename, partition);
}

/*****************************************************************
 *@brief:update the mac addr
 *@author:zhangshenglin
 *@date:2012-12-07
 *@param mac:mac addr
 *@return:int
 *@retval:0:success/ other value:fail
 ******************************************************************/
static int UpdateMac(const char* mac)
{
	return fha_interface_UpdateMac(mac);
}

/*****************************************************************
 *@brief:update the sn
 *@author:zhangshenglin
 *@date:2012-12-07
 *@param sn:sn number
 *@return:int
 *@retval:0:success/ other value:fail
 ******************************************************************/
static int UpdateSn(const char* sn)
{
	return fha_interface_UpdateSn(sn);
}

/*****************************************************************
 *@brief:update the Bsn
 *@author:zhangshenglin
 *@date:2012-12-07
 *@param bsn:Bsn number
 *@return:int
 *@retval:0:success/ other value:fail
 ******************************************************************/
static int UpdateBsn(const char* bsn)
{
	return fha_interface_UpdateBsn(bsn);
}

static void print_help(void)
{
	printf( "\nUsage:\n"
		"updater [option] [type]=[value]\n\n"
		"option:\n"
		"local	-- the file is on local\n"
		"http	-- the file is on http server\n"
		"ftp	-- the file is on ftp server\n\n"
		"type:\n"
		"K	-- update kernel\n"
		"B	-- update nandboot\n"
		"L	-- update bootup logo\n"
		"D	-- update RAM parameter, must update with nandboot!\n"
		"MTDx	-- update mtd x\n"
		"A	-- IP address\n"
		"P	-- port number\n"
		"U	-- server username\n"
		"C	-- server password\n"
		"X	-- need check file, 1 - yes, 0 - no\n\n"
		"m	-- mac addr\n"
		"s	-- serial number\n"
		"b	-- bar code\n"
		"example:\n"
		"updater local K=/mnt/zImage\n"
		"updater ftp K=/path/file1 A=a.b.c.d P=port U=aaa C=xxx\n");
}

/**
 * *  @brief       init the system V semaphore
 * *  @author      
 * *  @date        2013-5-9
 * *  @param[in]   none
 * *  @return	   	none
 * */
static int init_systemv_sem()
{
	union semun seminfo;
	unsigned short array[SEM_NUMS] = {1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1};
	
	sem_key = ftok(CAMERA, SEM_PROJ_ID); //use camera.ini to generate the key
	if (sem_key < 0) {
		printf("[%s:%d] ftok fail!", __func__, __LINE__);
		return -1;
	}
	
	if ((sem_id = semget(sem_key, 0, 0)) < 0) {
		if ((sem_id = semget(sem_key, SEM_NUMS, IPC_CREAT | 0666)) < 0) {
			printf("[%s:%d] semget fail, error %d!", __func__, __LINE__, errno);
			return -1;
		}
		
		seminfo.array = array;
		if (semctl(sem_id, 0, SETALL, seminfo) < 0) {
			printf("[%s:%d] semctl fail, error %d!", __func__, __LINE__, errno);
			return -1;
		}
	}
	
	return 0;
}

#define PATH_LEN 256
int main(int argc, char* argv[])
{
	char KLinkPath[PATH_LEN];//内核路径
	char BLinkPath[PATH_LEN];//boot路径
	char LLinkPath[PATH_LEN];//logo文件路径
	char DLinkPath[PATH_LEN];//内存参数路径
	char DownloadFile[PATH_LEN];//下载文件
	char MLinkPath[PATH_LEN];//root镜像文件路径
	
	char FtpSrvAddr[32];//ftp 服务器地址
	unsigned short FtpSrvPort = 21;//ftp 默认端口号 
	char FtpUsername[32];//ftp服务用户名
	char FtpPassword[32];//ftp服务密码

	char Mac[32];//mac地址
	char Sn[32];//序列号
	char Bsn[32];//条码
	int partition;
	int i = 0;

	memset(KLinkPath, 0, PATH_LEN);
	memset(BLinkPath, 0, PATH_LEN);
	memset(LLinkPath, 0, PATH_LEN);
	memset(DLinkPath, 0, PATH_LEN);
	memset(DownloadFile, 0, PATH_LEN);
	memset(MLinkPath, 0, PATH_LEN);

	memset(FtpSrvAddr, 0, 32);
	strcpy(FtpUsername, "anonymous");
	strcpy(FtpPassword, "anonymous");
	memset(Mac, 0, 32);
	memset(Sn, 0, 32);
	memset(Bsn, 0, 32);
	
	if(argc < 3)
	{
		print_help();
		return -1;
	}

	if (!strcmp(argv[1], "-h")) {
		print_help();
		return -1;
	}

	for(i = 2; i < argc; i ++)
	{
		if(argv[i][0] == 'K' && argv[i][1] == '=')
		{
			strcpy(KLinkPath, argv[i] + 2);
		}
		else if(argv[i][0] == 'B' && argv[i][1] == '=')
		{
			strcpy(BLinkPath, argv[i] + 2);
		}
		else if(argv[i][0] == 'L' && argv[i][1] == '=')
		{
			strcpy(LLinkPath, argv[i] + 2);
		}
		else if(argv[i][0] == 'D' && argv[i][1] == '=')
		{
			strcpy(DLinkPath, argv[i] + 2);
		}
		else if(argv[i][0] == 'A' && argv[i][1] == '=')
		{
			strcpy(FtpSrvAddr, argv[i] + 2);
		}
		else if(argv[i][0] == 'P' && argv[i][1] == '=')
		{
			FtpSrvPort = atoi(argv[i] + 2);
		}
		else if(argv[i][0] == 'U' && argv[i][1] == '=')
		{
			strcpy(FtpUsername, argv[i] + 2);
		}
		else if(argv[i][0] == 'C' && argv[i][1] == '=')
		{
			strcpy(FtpPassword, argv[i] + 2);
		}
		else if(argv[i][0] == 'X' && argv[i][1] == '=')
		{
			bNeedCheckFile = atoi(argv[i] + 2);
		}
		else if(argv[i][0] == 'M' && argv[i][4] == '=')
		{
			partition = atoi(argv[i] + 3);
			strcpy(MLinkPath, argv[i] + 5);
			printf("mtd part:%d\n", partition);
			printf("mtd path:%s\n", MLinkPath);
		}
		else if(argv[i][0] == 'm' && argv[i][1] == '=')
		{
			strcpy(Mac, argv[i] + 2);
		}
		else if(argv[i][0] == 's' && argv[i][1] == '=')
		{
			strcpy(Sn, argv[i] + 2);
		}
		else if(argv[i][0] == 'b' && argv[i][1] == '=')
		{
			strcpy(Bsn, argv[i] + 2);
		}
		else
		{
			continue;
		}
	}

	printf("kernel path:%s\n", KLinkPath);
	printf("boot   path:%s\n", BLinkPath);
	//printf("logo   path:%s\n", LLinkPath);
	printf("mac:%s\n", Mac);
	//printf("sn:%s\n", Sn);
	//printf("bsn:%s\n", Bsn);
    
    //step 1:download if need
	if(strcmp(argv[1], "local") == 0)//本地升级
	{
		printf("Update from local file\n");
	}
	else if(strcmp(argv[1], "ftp") == 0)//ftp网络升级
	{
		printf("Update from ftp server file\n");
		//printf("Not support now!\n");
		printf("server ip:%s,port:%u\n", FtpSrvAddr, FtpSrvPort);
		printf("Now, start to download file from server\n");

		if(BLinkPath[0] != 0)
		{
			printf("start to download Boot file\n");
			if(FtpGet(FtpSrvAddr, FtpSrvPort, FtpUsername, FtpPassword, BLinkPath, BOOT_FILE) > 0)
			{
				memset(BLinkPath, 0, PATH_LEN);
				strcpy(BLinkPath, BOOT_FILE);
				printf("download Boot file success\n");
			}
			else
			{
				printf("download Boot file fail\n");
			}
		}

		if(KLinkPath[0] != 0)
		{
			printf("start to download Kernel file\n");
			if(FtpGet(FtpSrvAddr, FtpSrvPort, FtpUsername, FtpPassword, KLinkPath, KERNEL_FILE) > 0)
			{
				memset(KLinkPath, 0, PATH_LEN);
				strcpy(KLinkPath, KERNEL_FILE);
				printf("download Kernel file success\n");
			}
			else
			{
				printf("download Kernel file fail\n");
			}
		}

		if(LLinkPath[0] != 0)
		{
			printf("start to download Logo file\n");
			if(FtpGet(FtpSrvAddr, FtpSrvPort, FtpUsername, FtpPassword, LLinkPath, LOGO_FILE) > 0)
			{
				memset(LLinkPath, 0, PATH_LEN);
				strcpy(LLinkPath, LOGO_FILE);
				printf("download Logo file success\n");
			}
			else
			{
				printf("download Logo file fail\n");
			}
		}
	}
	else if(strcmp(argv[1], "http") == 0)//http网络升级
	{
		printf("Update from http server file\n");
		printf("Now, start to download file from server\n");
		if(BLinkPath[0] != 0)
		{
			printf("start to download Boot file\n");
			if(HttpDownload(BLinkPath, DownloadFile) == 0)
			{
				memset(BLinkPath, 0, PATH_LEN);
				strcpy(BLinkPath, DownloadFile);
				printf("download Boot file success\n");
			}
			else
			{
				printf("download Boot file fail\n");
			}
			
		}

		if(KLinkPath[0] != 0)
		{
			printf("start to download Kernel file\n");

			if(HttpDownload(KLinkPath, DownloadFile) == 0)
			{
				memset(KLinkPath, 0, PATH_LEN);
				strcpy(KLinkPath, DownloadFile);
				printf("download Kernel file success\n");
			}
			else
			{
				printf("download Kernel file fail\n");
			}
		}

		if(LLinkPath[0] != 0)
		{
			printf("start to download Logo file\n");

			if(HttpDownload(LLinkPath, DownloadFile) == 0)
			{
				memset(LLinkPath, 0, PATH_LEN);
				strcpy(KLinkPath, DownloadFile);
				printf("download Logo file success\n");
			}
			else
			{
				printf("download Logo file fail\n");
			}
		}
	}
	else
	{
		printf("param error\n");
		return 1;
	}

    //step 2:update
	if (!init_systemv_sem()) g_bIsSem = 1; 
	
	if(fha_interface_init() == 0)//fha接口初始化
	{
		printf("fha_interface_init success\n");
	}
	else
	{
		printf("fha_interface_init failed\n");
		return -1;
	}

	if(KLinkPath[0] != 0)//升级内核
	{
		if(UpdateKernel(KLinkPath) == 0)
		{
			if (g_bIsSem) semctl(sem_id, KERNEL_UPDATE_FILE, SETVAL, 1);
			printf("update KERNEL bin success\n");
		}
		else
		{
			if (g_bIsSem) semctl(sem_id, KERNEL_UPDATE_FILE, SETVAL, 0);
			printf("update KERNEL bin failure\n");
		}
	}
	
	if(LLinkPath[0] != 0)//升级logo文件
	{
		if(UpdateLogo(LLinkPath) == 0)
		{
			printf("update LOGO bin success\n");
		}
		else
		{
			printf("update LOGO bin failure\n");
		}
	}

	if(BLinkPath[0] != 0)//升级boot区
	{
		if(UpdateBoot(BLinkPath, DLinkPath) == 0)
		{
			printf("update boot success\n");
		}
		else
		{
			printf("update boot failure\n");
		}
	}

	if(MLinkPath[0] != 0)//升级root镜像
	{
		if(UpdateMTD(MLinkPath, partition) == 0)
		{
			if (g_bIsSem) semctl(sem_id, APP_UPDATE_FILE, SETVAL, 1);
			printf("update mtd%d success\n", partition);
		}
		else
		{
			if (g_bIsSem) semctl(sem_id, APP_UPDATE_FILE, SETVAL, 0);
			printf("update mtd%d failed\n", partition);
		}
	}

	if(strlen(Mac) > 0)//升级mac地址
	{
		if(UpdateMac(Mac) == 0)
		{
			printf("update mac success\n");
		}
		else
		{
			printf("update mac failed\n");
		}
	}
	if(strlen(Sn) > 0)//升级序列号
	{
		if(UpdateSn(Sn) == 0)
		{
			printf("update Sn success\n");
		}
		else
		{
			printf("update Sn failed\n");
		}
	}
	if(strlen(Bsn) > 0)//升级条码
	{
		if(UpdateBsn(Bsn) == 0)
		{
			printf("update Bsn success\n");
		}
		else
		{
			printf("update Bsn failed\n");
		}
	}
	
	if(fha_interface_destroy() == 0)//释放fha接口
	{
		printf("fha_interface_destroy success\n");
	}
	else
	{
		printf("fha_interface_destroy failed\n");
		return -1;
	}

	printf("######### Update End! You Should Reboot The System ######!\n");
	return 0;
}
