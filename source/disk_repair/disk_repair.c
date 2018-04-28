#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <signal.h>
#include <execinfo.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <unistd.h>

#include "anyka_types.h"
#include "mtdlib.h"
#include "file.h"
#include "driver.h"
#include "globallib.h"
#include "global.h"


int g_handle;

T_U32 medium_read(T_PMEDIUM medium, T_U8* buf, T_U32 start, T_U32 size)
{
	off64_t offset_size = (off64_t)start * 512;
	T_U32 read_size = 0;

	if (g_handle == 0)
	{
		return 0;
	}

	lseek64(g_handle, offset_size, SEEK_SET);
	read_size = read(g_handle, buf, size * 512);
//	SetFilePointer(g_handle, offset_size, &high, 0);
//	read_size = ReadFile(g_handle, buf, size * 512, &high,NULL);
	if (read_size <= 0)
	{
		return 0;
	}

	return read_size/512;
}


T_U32 medium_write(T_PMEDIUM medium, T_U8* buf, T_U32 start, T_U32 size)
{
	off64_t offset_size = (off64_t)start * 512;
	T_U32 write_size;

	if (g_handle == 0)
	{
		return 0;
	}

	 lseek64(g_handle, offset_size, SEEK_SET);
//	SetFilePointer(g_handle, offset_size, &high, 0);
	write_size = write(g_handle, buf, size * 512);
	//write_size = WriteFile(g_handle, buf, size * 512, &high, NULL);
	if (write_size <= 0)
	{
		return 0;
	}
	return size;
}

T_BOOL medium_flush(T_PMEDIUM medium)
{

	return AK_TRUE;
}


T_PMEDIUM Creat_Medium(T_U32 capacity)
{
	T_U32 i;
	T_PMEDIUM pmedium = AK_NULL;
	T_U32 BytsPerSec = 512; 
	
	
	pmedium = (T_PMEDIUM)malloc(sizeof(T_MEDIUM));
    
	if(pmedium == AK_NULL)
	{
		return AK_NULL;
	}

	i = 0;
    while (BytsPerSec > 1)
    {
        BytsPerSec >>= 1;
        i++;
    }
    pmedium->SecBit = (T_U8) i;
    
    // if the device is sd, we will set pagesize is 16k
    
    pmedium->PageBit = (T_U8)i;
    pmedium->SecPerPg = 1;
	
	
	pmedium->type = TYPE_MEDIUM;
	pmedium->read = (F_ReadSector)medium_read;
	pmedium->write = (F_WriteSector)medium_write;
	pmedium->capacity = capacity;
	pmedium->flush = medium_flush;
	pmedium->type = MEDIUM_SD;
	pmedium->msg = AK_NULL;
	
	return pmedium;
}


T_U32 anyka_fsGetSecond(T_VOID)
{
	return time(0);

}
T_VOID anyka_fsSetSecond(T_U32 seconds)
{

}
T_S32 anyka_fsUniToAsc(const T_U16 *pUniStr, T_U32 UniStrLen,
                    T_pSTR pAnsibuf, T_U32 AnsiBufLen, T_U32 code)
{
	int i;
	
	for(i=0; i < UniStrLen; i++)
	{
		pAnsibuf[i] = pUniStr[i];
	}
	return i;
 }
T_S32 anyka_fsAscToUni(const T_pSTR pAnsiStr, T_U32 AnsiStrLen,
                    T_U16 *pUniBuf, T_U32 UniBufLen, T_U32 code)
{
	int i;
	
	for(i=0; i < AnsiStrLen; i++)
	{
		 pUniBuf[i] = pAnsiStr[i];
	}
	return i;
}

T_pVOID    anyka_fsRamAlloc(T_U32 size, T_S8 *filename, T_U32 fileline)
{
	return malloc(size);
}
T_pVOID    anyka_fsRamRealloc(T_pVOID var, T_U32 size, T_S8 *filename, T_U32 fileline)
{
	T_pVOID var_new;
	
	var_new = (T_pVOID)malloc( size);
	memcpy(var_new, var, size);
	free(var);
	return var_new;
} 
T_pVOID anyka_fsRamFree(T_pVOID var, T_S8 *filename, T_U32 fileline)
{
	free(var);
	return NULL;
}

T_S32 anyka_fsOsCrtSem(T_U32 initial_count, T_U8 suspend_type, T_S8 *filename, T_U32 fileline)
{
	
        
        return 1;
}
T_S32 anyka_fsOsDelSem(T_S32 semaphore, T_S8 *filename, T_U32 fileline)
{
        return 1;

}
T_S32 anyka_fsOsObtSem(T_S32 semaphore, T_U32 suspend, T_S8 *filename, T_U32 fileline)
{
        return 1;
}
T_S32 anyka_fsOsRelSem(T_S32 semaphore, T_S8 *filename, T_U32 fileline)
{
        return 1;
}

T_U32 anyka_fsGetChipID(T_VOID)
{
	return 0;
}

T_pVOID anyka_fsMemCpy(T_pVOID dst, T_pCVOID src, T_U32 count)
{

	return memcpy(dst, src, count);
}
T_pVOID anyka_fsMemSet(T_pVOID buf, T_S32 value, T_U32 count)
{
	return memset(buf, value, count);
}
T_pVOID anyka_fsMemMov(T_pVOID dst, T_pCVOID src, T_U32 count)
{
	return memmove(dst, src, count);
}
T_S32   anyka_fsMemCmp(T_pCVOID buf1, T_pCVOID buf2, T_U32 count)
{
	return memcmp(buf1, buf2, count);
}


void del_file_main(char *file_name, const char *dev_name, T_U32 total_sec)
{
	//open 
	T_FSINITINFO fsInfo;
	T_PMEDIUM medium = AK_NULL;
	T_U32 driver = 0;
	T_U16 i, del_file[200];
	int ret = 0;

	g_handle = open((const char *)dev_name, O_RDWR);
	if (g_handle <= 0)
	{
		printf("we can't open the device(%s)!\n", dev_name);
		return;
	}
	
    fsInfo.fGetSecond = anyka_fsGetSecond;
    fsInfo.fSetSecond = anyka_fsSetSecond;
    fsInfo.fUniToAsc = anyka_fsUniToAsc;
    fsInfo.fAscToUni = anyka_fsAscToUni;
    fsInfo.fRamAlloc = anyka_fsRamAlloc;
    fsInfo.fRamRealloc = anyka_fsRamRealloc;
    fsInfo.fRamFree = anyka_fsRamFree;
    fsInfo.fCrtSem = anyka_fsOsCrtSem;
    fsInfo.fDelSem = anyka_fsOsDelSem;
    fsInfo.fObtSem = anyka_fsOsObtSem;
    fsInfo.fRelSem = anyka_fsOsRelSem;
 
    fsInfo.fMemCpy = anyka_fsMemCpy;
    fsInfo.fMemSet = anyka_fsMemSet;
    fsInfo.fMemMov = anyka_fsMemMov;
    fsInfo.fMemCmp = anyka_fsMemCmp;
    fsInfo.fPrintf = (F_Printf)printf;
 
    fsInfo.fGetChipId = anyka_fsGetChipID;
    Global_Initial(&fsInfo);


	//creat medium
	medium = Creat_Medium(total_sec);
	if (medium == AK_NULL)
	{
		//¹رӍ
		close(g_handle);
		//CloseHandle(g_handle);
		return;
	}

	//driver_init
	driver = Driver_Initial(medium, 512);
	if(driver == 0)
	{
		//¹رӍ
		close(g_handle);
		//CloseHandle(g_handle);
		return;
	}
	Global_MountDriver(driver, 0);

	for(i = 0; i < strlen((char *)file_name); i ++)
	{
		del_file[i] = file_name[i];
	}
	del_file[i] = 0;
	if(!File_DelUnicode(del_file))
	{
		//к»׍
		Driver_Destroy((T_PDRIVER)driver);
		
		//¹رӍ
		close(g_handle);
		//CloseHandle(g_handle);
		printf("[%s:%d] delete failed\n", __func__, __LINE__);
		return;
	}
	//Global_UninstallDriver(driver);
	//к»׍
	Driver_Destroy((T_PDRIVER)driver);

	//¹رӍ
	ret = close(g_handle);
	//CloseHandle(g_handle);
	if (ret < 0)
	{
		printf("[%s:%d] close failed\n", __func__, __LINE__);
		return;
	}

	printf("[%s:%d] finished\n", __func__, __LINE__);
}

static void sigprocess(int sig)
{
	// Fix me ! 20140828 ! Bug Bug Bug Bug Bug
#if 1
	printf("**************************\n");
	printf("\n##signal %d caught\n", sig);
	printf("**************************\n");
	int ii = 0;
	void *tracePtrs[16];
	int count = backtrace(tracePtrs, 16);
	char **funcNames = backtrace_symbols(tracePtrs, count);
	for(ii = 0; ii < count; ii++)
		printf("%s\n", funcNames[ii]);
	free(funcNames);
	fflush(stderr);
	fflush(stdout);

    #if 1
	if(sig == SIGINT || sig == SIGSEGV || sig == SIGTERM)
	{
		
	}
    #endif
	exit(1);
#endif
}


unsigned int get_tf_size(const char *path)
{
	struct statfs disk_statfs;
	unsigned int total_size;

	bzero(&disk_statfs, sizeof(struct statfs));
	while(statfs(path, &disk_statfs) == -1)
	{
		if(errno != EINTR ){
			printf("statfs: %s Last error == %s\n", path, strerror(errno));
			return -1;
		}
	}

	total_size = disk_statfs.f_bsize / 512;
	total_size = total_size * disk_statfs.f_blocks;

	return total_size;
}

int parse_path(const char *full_path, char *tf_path, char *file_path)
{
	char *p = NULL;

	if(strlen(full_path) <= 0){
		printf("[%s:%d] The file full path error\n", __func__, __LINE__);
		return -1;
	}

	p = strchr(full_path, '/');
	if(p){
		p += 1;
	}else{
		printf("[%s:%d] error path %s\n", __func__, __LINE__, full_path);
		return -1;
	}

	p = strchr(p, '/');
	if(p){
		strncpy(tf_path, full_path, p - full_path);
		printf("[%s:%d] Get TF-Card mount node path : %s\n", __func__, __LINE__, tf_path);

		strncpy(file_path, p, strlen(p));
		printf("[%s:%d] Get file name : %s\n", __func__, __LINE__, file_path);
	}else{
		printf("[%s:%d] error path %s, please make sure the path is absolute.\n", __func__, __LINE__, full_path);
		return -1;
	}


	return 0;
}


int main(int argc, char *argv[])
{
	unsigned int tf_size;
	char cmd[100] = {0};
	char file_path[100] = {0};
	char tf_path[20] = {0};

	if(argc < 3){
		printf("This program need 3 arguments at less.\n");
		return -1;
	}
	
	signal(SIGSEGV, sigprocess);
	signal(SIGINT, sigprocess);
	signal(SIGTERM, sigprocess);

	if(parse_path(argv[1], tf_path, file_path) < 0){
		printf("[%s:%d] error arguments, exit\n", __func__, __LINE__);
		exit(EXIT_FAILURE);
	}

	tf_size = get_tf_size(tf_path);
	printf("[%s:%d] get TF card size : %u\n", __func__, __LINE__, tf_size);

	sprintf(cmd, "umount %s -l", tf_path);
	system(cmd);
	system("sync");
	//system("mount");
	
	del_file_main(file_path, argv[2], tf_size);

	system("sync");
	bzero(cmd, sizeof(cmd));
	sprintf(cmd, "mount %s %s", argv[2], tf_path);
	system(cmd);
	//system("mount");

	return 0;
}
