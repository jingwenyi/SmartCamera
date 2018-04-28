#ifndef _anyka_ftp_update_h_
#define _anyka_ftp_update_h_


/**
* @brief  在系统同步时间后，如果当前时间是在升级区间内，
              将启动升级信息检查，如果发现不配对，将启动升级
* 
* @param[in]  int cur_year  系统当前的年，目前认为系统时间在大于2014年，
                                       才认为同步了时间，否则不会启动FTP升级
                     int cur_time 以当天00：00开始算，以秒为单位，当前的时间，
* @return void
* @retval none
*/

void anyka_ftp_update_main(int cur_year, int cur_time);

#endif

