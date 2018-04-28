#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>

#include <stdio.h>
#include <stdlib.h>
#include "daemon_inc.h"

//#define anyka_print	printf
#define RTC_DEV	"/dev/rtc0"

static int rtc_fd = -1;

/**
 * *  @brief		打开rtc设备文件
 * *  @author	ye_guohong
 * *  @date		2014-12-10
 * *  @param	无
 * *  @return		0: 成功，-1: 失败
 * */
int rtc_dev_open(void)
{
	int ret = -1;
	
	rtc_fd = open(RTC_DEV, O_RDWR);
	if(rtc_fd == -1)
	{
		anyka_print("[%s:%d]open rtc failed,%s\n", __func__, __LINE__, strerror(errno));
		ret = -1;
	}
	else
	{
		anyka_print("open rtc ok\n");
		ret = 0;
	}

	return ret;
}

/**
 * *  @brief		关闭rtc设备文件
 * *  @author	ye_guohong
 * *  @date		2014-12-10
 * *  @param	无
 * *  @return		0: 成功，-1: 失败
 * */
int rtc_dev_close(void)
{
	int ret;
	
	ret = close(rtc_fd);

	rtc_fd = -1;

	return ret;
}

/**
 * *  @brief		设置多少秒后产生wakeup alarm
 * *  @author	ye_guohong
 * *  @date		2014-12-10
 * *  @param	seconds: 多少秒后产生wakeup alarm
 * *  @return		0: 成功，-1: 失败
 * */
int rtc_set_wakeup_alarm(unsigned int seconds)
{
	struct rtc_wkalrm alrm;
	struct tm		time;
	struct tm		*ptime;
	time_t 			tm_t;

	anyka_print("RTC_RD_TIME:%d.\n",RTC_RD_TIME);
	if (ioctl(rtc_fd, RTC_RD_TIME, &time) < 0) {
		anyka_print("[%s:%d]read rtc real time failed,%s\n", __func__, __LINE__, strerror(errno));
		goto failed;
	}

	anyka_print("rtc real time read:year:%d,mon:%d,mday:%d,hour:%d,min:%d,sec:%d.\n",
		time.tm_year,
		time.tm_mon,
		time.tm_mday,
		time.tm_hour,
		time.tm_min,
		time.tm_sec
		);

	tm_t = mktime(&time);
	anyka_print("rtc seconds:%d.\n",(int)tm_t);

	ptime = localtime(&tm_t);
	anyka_print("back to date:year:%d,mon:%d,mday:%d,hour:%d,min:%d,sec:%d.\n",
		ptime->tm_year,
		ptime->tm_mon,
		ptime->tm_mday,
		ptime->tm_hour,
		ptime->tm_min,
		ptime->tm_sec
		);

	tm_t += seconds;
	ptime = localtime(&tm_t);
	anyka_print("after plus seconds to date:year:%d,mon:%d,mday:%d,hour:%d,min:%d,sec:%d.\n",
		ptime->tm_year,
		ptime->tm_mon,
		ptime->tm_mday,
		ptime->tm_hour,
		ptime->tm_min,
		ptime->tm_sec
		);

	alrm.enabled	= 1;
	alrm.pending	= 1;
	alrm.time.tm_sec	= ptime->tm_sec;
	alrm.time.tm_min	= ptime->tm_min;
	alrm.time.tm_hour	= ptime->tm_hour;
	alrm.time.tm_mday	= ptime->tm_mday;
	alrm.time.tm_mon	= ptime->tm_mon;
	alrm.time.tm_year	= ptime->tm_year;
	alrm.time.tm_wday	= ptime->tm_wday;
	alrm.time.tm_yday	= ptime->tm_yday;
	anyka_print("RTC_WKALM_SET:%d.\n",RTC_WKALM_SET);
	if (ioctl(rtc_fd, RTC_WKALM_SET, &alrm) < 0) {
		anyka_print("[%s:%d] set wakeup alarm failed,%s\n", __func__, __LINE__, strerror(errno));
		goto failed;
	}

	return 0;

failed:
	return -1;
}

/**
 * *  @brief		rtc wakeup设置总入口
 * *  @author	ye_guohong
 * *  @date		2014-12-10
 * *  @param	delay_s: 多少秒后产生wakeup alarm
 * *  @return		0: 成功，-1: 失败
 * */
int rtc_wakeup_standby(int delay_s)
{
	rtc_dev_open();

	rtc_set_wakeup_alarm(delay_s);

	rtc_dev_close();

	return 0;
}
