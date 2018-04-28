/**
* @brief  sd card API
* 
* @author cyh
* @date 2015-11-04
*/

#ifndef __SDCARD_H__
#define __SDCARD_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief check_sdcard
		 check system sd status 
* @author aj
* @date 
* @param[in] void
* @return int 
* @retval system sd status,
* 		  sd mounted, return 0, else return -1;
*/

int check_sdcard( void );


/**
* @brief 	sd_mount_sd
* 			mount sd card to /mnt
* @author 	CYH
* @date 
* @param[in]  void
* @return    	  int
* @retval   mount success return 0, else return -1;
*/

int sd_mount_sd(void);

/**
* @brief  umount_sd
		 umount the sd
* @author cyh
* @date 
* @param[in] void
* @return  int
* @retval  success 0, fail -1
*/
int sd_umount_sd(void);


/**
* @brief sd_set_status
		set system sd status
* @author aj
* @date 
* @param[in] int status, status which you want sets
* @return  void
* @retval   
*/

void sd_set_status(int status);

/**
* @brief sd_get_status
		get system sd status
* @author aj
* @date 
* @param[in] 
* @return  int
* @retval   the sd card status
*/
int sd_get_status(void);

/**
* @brief sd_init_status
		 init system sd status
* @author aj
* @date 
* @param[in] void
* @return void
* @retval 
*/

void sd_init_status(void);

#ifdef __cplusplus
}
#endif

#endif
