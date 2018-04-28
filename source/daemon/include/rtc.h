#ifndef __RTC_H__
#define __RTC_H__

/**
 * *  @brief		rtc wakeup设置总入口
 * *  @author	ye_guohong
 * *  @date		2014-12-10
 * *  @param	delay_s: 多少秒后产生wakeup alarm
 * *  @return		0: 成功，-1: 失败
 * */
int rtc_wakeup_standby(int delay_s);

#endif
