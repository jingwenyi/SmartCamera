#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "TXDeviceSDK.h"
#include "TXWifisync.h"
#include "common.h"
#include "realtek.h"
#include "bcmdhd.h"
#include "mt7601.h"

int g_sniffer_running = 1;

/**
* @brief 信号处理函数
* @author ye_guohong
* @date 2015-03-31
* @param[in]    sig     信号值
* @return void
*/
static void sigprocess(int sig)
{
	anyka_print("smartlink catch signal: %d.\n", sig);

	switch (sig) {
	case SIGSEGV:
	case SIGINT:
	case SIGTERM:
		g_sniffer_running = 0;
		break;

	default:
		break;
	}
}

/**
* @brief 信号处理注册
* @author ye_guohong
* @date 2015-03-31
* @param[in] void
* @return void
*/
static void sig_init(void)
{
	signal(SIGSEGV, sigprocess);
	signal(SIGINT, sigprocess);
	signal(SIGTERM, sigprocess);
}

/**
* @brief main
* @author ye_guohong
* @date 2015-03-31
* @param[in] void
* @return int
* @retval if return 0 success, otherwise failed
*/
int main()
{		
	sig_init();

	if (is_realtek())
		return realtek_smartlink();
	else if (is_bcmdhd())
		return bcmdhd_smartlink();
	else if (1/*is_mtk()*/)
		return mtk_smartlink();		

	anyka_print("Cann't find wirless device\n");
	return -1;
}

