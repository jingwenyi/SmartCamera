#ifndef _DAEMON_INIT_H_
#define _DAEMON_INIT_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>


/**
* @brief 	daemon_init
* 			将当前进程设置为守护进程接口
* @date 	2015/3
* @param:	void
* @return 	int
* @retval 	0
*/

int daemon_init(void);

#endif
