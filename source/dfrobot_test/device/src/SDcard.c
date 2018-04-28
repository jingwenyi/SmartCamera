#include <sys/socket.h>
#include <linux/netlink.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "common.h"
#include "SDcard.h"

#define SDDEV 			"/dev/mmcblk0"
#define SD_DEV_NODE 	"/dev/mmcblk0p1"
#define MOUNT_POINT		"/mnt"

int sd_status= 1;


/**
* @brief check_sdcard
		 check system sd status 
* @author cyh
* @date 
* @param[in] void
* @return int 
* @retval system sd status,
* 		  sd mounted, return 0, else return -1;
*/

int check_sdcard(void)
{
	char *p = NULL;
	char result[4096] = {0};

	do_syscmd("mount", result);

	/** check mmcblk0p1 **/
	p = strstr(result, SD_DEV_NODE);
	if(p) {
		p = strstr(p, MOUNT_POINT);
		if(p)
			return 0;
	}

	/** check mmcblk0 **/
	p = strstr(result, SDDEV);
	if(p) {
		p = strstr(p, MOUNT_POINT);
		if(p)
			return 0;
	}

	return -1;
}


/**
* @brief 	sd_mount_sd
* 			mount sd card to /mnt
* @author 	CYH
* @date 
* @param[in]  void
* @return    	  int
* @retval   mount success return 0, else return -1;
*/

int sd_mount_sd(void)
{ 
	if(access(SD_DEV_NODE, R_OK) == 0)
		system("mount /dev/mmcblk0p1 /mnt");
	else
		system("mount /dev/mmcblk0 /mnt");

	if(check_sdcard() < 0) {
		sd_status = 0;
		anyka_print("[%s:%d] *** fail to mount sd ***\n",
				__func__, __LINE__);
		return -1;
	}
	sd_status = 1;
	anyka_print("[%s:%d] *** mount the sd on /mnt ***\n",
			__func__, __LINE__);

	return 0;
}

/**
* @brief  umount_sd
		 umount the sd
* @author cyh
* @date 
* @param[in] void
* @return  int
* @retval  success 0, fail -1
*/
int sd_umount_sd(void)
{
	system("umount -l /mnt");

	if(check_sdcard() == 0) {
		sd_status = 1;
		anyka_print("[%s:%d] *** umount sd fail ***\n", 
				__func__, __LINE__);
		return -1;
	}
	sd_status = 0;
	anyka_print("[%s:%d] *** umount the sd ***\n", 
			__func__, __LINE__);
	
	return 0;
}

/**
* @brief sd_set_status
		set system sd status
* @author aj
* @date 
* @param[in] int status, status which you want sets
* @return  void
* @retval   
*/

void sd_set_status(int status)
{
	sd_status = status;
	anyka_print("[%s:%d] *** set sd status: %d ***\n", 
		__func__, __LINE__, sd_status);
}

/**
* @brief sd_get_status
		get system sd status
* @author cyh
* @date 
* @param[in] 
* @return  int
* @retval   the sd card status
*/

int sd_get_status(void)
{
    return sd_status;
}

/**
* @brief sd_init_status
		 init system sd status
* @author aj
* @date 
* @param[in] void
* @return void
* @retval 
*/
void sd_init_status(void)
{
	T_U8 check_card=0;

	if(access(SD_DEV_NODE, R_OK) == 0)
        check_card = 1;
    else if (access(SDDEV, R_OK) == 0)
        check_card = 1;

    sd_status = 0;
    if (check_card)
    {
        for (int i=0; i<5; i++)
        {
            if (check_sdcard() == 0)
            {
                sd_status = 1;
                break;
            }
            sleep(1);
        }
    }
	
	anyka_print("[%s:%d] sdcard status: %d\n",
			__func__, __LINE__, sd_status);

}

