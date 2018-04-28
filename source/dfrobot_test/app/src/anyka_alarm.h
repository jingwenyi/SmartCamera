#ifndef _ANYKA_ALARM_H_
#define _ANYKA_ALARM_H_

#include <sys/ioctl.h>
#include <stdint.h>
typedef uint8_t __u8;



/**
* @brief 	anyka_alarm.h
* 			定义alarm 所使用的结构信息
* @date 	2015/3
* @param:	
* @return 	
* @retval 	
*/

/***************** gpio magic  ***********************/
#define APPLY_IOC_MAGIC 'f'		
/*
	   Command definitions which are supported by this dirver
 */
#define GET_GPIO_NUM 	   _IOR(APPLY_IOC_MAGIC, 40, __u8)
#define GET_AVAILABLE_GPIO  _IOR(APPLY_IOC_MAGIC, 41, __u8)
#define SET_GPIO_FUNC	   _IOW(APPLY_IOC_MAGIC, 42, __u8)
#define GET_GPIO_VALUE	   _IOWR(APPLY_IOC_MAGIC, 43, __u8)
#define SET_GPIO_IRQ 	   _IOW(APPLY_IOC_MAGIC, 44, __u8)
#define LISTEN_GPIO_IRQ	   _IOW(APPLY_IOC_MAGIC, 45, __u8)
#define DELETE_GPIO_IRQ	   _IOW(APPLY_IOC_MAGIC, 46, __u8)
#define SET_GPIO_LEVEL	   _IOW(APPLY_IOC_MAGIC, 47, __u8)
#define SET_GROUP_GPIO_LEVEL    _IOW(APPLY_IOC_MAGIC, 48, __u8)

#define AK_GPIO_DIR_OUTPUT	   1
#define AK_GPIO_DIR_INPUT	   0
#define AK_GPIO_INT_DISABLE	   0
#define AK_GPIO_INT_ENABLE	   1
#define AK_GPIO_INT_LOWLEVEL    0
#define AK_GPIO_INT_HIGHLEVEL   1
 
#define AK_GPIO_OUT_LOW		   0
#define AK_GPIO_OUT_HIGH 	   1
 
#define AK_PULLUP_DISABLE	   0
#define AK_PULLUP_ENABLE 	   1
#define AK_PULLDOWN_DISABLE	   0
#define AK_PULLDOWN_ENABLE	   1

/**
* @brief 	gpio_info
* 			定义gpio_info 所使用的结构信息
* @date 	2015/3
* @param:	
* @return 	
* @retval 	
*/

struct gpio_info {
   int pin;
   char pulldown;
   char pullup;
   char value;
   char dir;
   char int_pol;
};


#endif 



