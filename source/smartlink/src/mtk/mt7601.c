#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <netinet/ether.h>

#include "TXDeviceSDK.h"
#include "TXWifisync.h"
#include "common.h"
#include "mt7601.h"
#include "convert.h"

#define IFNAME	"wlan0"
#define CHANNEL_WAIT_TIME		200
#define CHANNEL_SNIFFER_TIME	10000
#define BUF_SIZ		(1500)

extern int g_sniffer_running;

enum wifi_sniffer_flag {
	WAIT_SYNC_PACKAGE = 1,
	WAIT_DATE_PACKAGE,
};

/**
* @brief 配置抓取channel
* @author ye_guohong
* @date 2015-03-31
* @param[in]    channel     抓取channel
* @param[in]    cmd         命令格式
* @return int
* @retval if return 0 success, otherwise failed
*/
static int sniffer_hopping(int channel, char *cmd)
{
	char cmdstr[256];

	if(channel < 1 || channel >13)
		return -1;

	//sprintf(cmdstr,"echo %d 0 0 > /proc/net/rtl8188eu/%s/monitor\n", channel, IFNAME);
	sprintf(cmdstr, cmd, channel);
	system(cmdstr); 
//	anyka_print("[%s]:channel %d hopping at %ld\n", __FUNCTION__, channel, time(0));
	printf("hop: %d, t: %ld\n",  channel, time(0));
	return 0;
}

/**
* @brief smartlink成功后的回调函数
* @author ye_guohong
* @date 2015-03-31
* @param[in]    pSmartlinkParam     smartlink解析完毕的信息
* @param[in]    pUserData           用户自定义数据
* @return void
* @retval
*/
static void on_smartlink_notify(tx_wifi_sync_param *pSmartlinkParam, void* pUserData)
{
	struct smartlink_wifi_info wifi_info;

	if(!pSmartlinkParam)
		return;

    anyka_print("###################################\n");
	anyka_print("[smartlink]ssid:%s\n",pSmartlinkParam->sz_ssid);
	anyka_print("[smartlink]pswd:%s\n",pSmartlinkParam->sz_password);
	anyka_print("[smartlink]ip:%s\n",pSmartlinkParam->sz_ip);
	anyka_print("[smartlink]bssid:%s\n",pSmartlinkParam->sz_bssid);
	anyka_print("[smartlink]port:%d\n",pSmartlinkParam->sh_port);
    anyka_print("###################################\n");

	strncpy(wifi_info.ssid, pSmartlinkParam->sz_ssid, sizeof(wifi_info.ssid));
	strncpy(wifi_info.passwd, pSmartlinkParam->sz_password, sizeof(wifi_info.passwd));
	store_wifi_info(&wifi_info);
    save2file(pSmartlinkParam->sz_ip, pSmartlinkParam->sh_port);
	//tx_ack_app(inet_addr(pSmartlinkParam->sz_ip), pSmartlinkParam->sh_port);
	if(save_ssid_to_tmp((char *)pSmartlinkParam->sz_ssid, strlen(pSmartlinkParam->sz_ssid)))
		anyka_print("[smartlink] fails to save ssid to tmp\n");

	g_sniffer_running = 0;
}

/**
* @brief 确定该使用什么命令格式来配置抓取channel
* @author ye_guohong
* @date 2015-03-31
* @param[out]   change_channel_cmd      输出抓取channel的命令格式
* @return int
* @retval if return 0 success, otherwise failed
*/


static int find_change_channel_cmd(char *change_channel_cmd)
{

	strcpy(change_channel_cmd, "iwconfig wlan0 channel  %d "); 

	return 0;
}


/**
* @brief 判别是否加载了realtek驱动
* @author ye_guohong
* @date 2015-03-31
* @param[out]   
* @return int
* @retval if return 1 success, otherwise failed
*/
int is_mtk(void)
{

	int i;
	char buf[128];
	int  result =  0;

	char mtk[][16] = {
		"elian",
	};

	for (i = 0 ; i < sizeof(mtk)/sizeof(mtk[0]); i++) {
		sprintf(buf, "/proc/%s", mtk[i]);
		if (access(buf, R_OK) == 0) {
			result = 1;
		}
	}
  	printf("is_mtk7601==%d\n",result);
	return result;
}



char change_chn_cmd[128];
void smartlink_hopping_cb(int nchannel)
{
	sniffer_hopping(nchannel, change_chn_cmd);
}

int mtk_smartlink(void)
{
	if (find_change_channel_cmd(change_chn_cmd))
	{
		printf("Err: cann't find change channel cmd\n"); 
		exit(EXIT_FAILURE);
	}

	/*
	* 初始化wifisync ..............
	*/
	int nGUIDSize;
	char serial_number[32] = {0};  // guid
	char guid_path[] = "/etc/jffs2/tencent.conf";
	if(access(guid_path, F_OK) == 0)
	{
		readBufferFromFile(guid_path, (void *)serial_number, 16, &nGUIDSize);
	}
	else
	{
		//strcpy(serial_number, "30249a57-cf52-41");
		anyka_print("Don't find guid in file: %s\n", guid_path);
		exit(EXIT_FAILURE);
	}
	anyka_print("init_wifi_sync serial_number[%s]\n",serial_number);

	int hopping_time=CHANNEL_WAIT_TIME, sniffer_time=CHANNEL_SNIFFER_TIME;
    start_smartlink(smartlink_hopping_cb, "wlan0"/*jweih*/, on_smartlink_notify, serial_number, WCT_MTK7601, 
                    hopping_time, sniffer_time, NULL);

    while(g_sniffer_running)
    {
        sleep(1);
    }
    
    return 0;
}


