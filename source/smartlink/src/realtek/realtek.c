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
#include "realtek.h"


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

	sprintf(cmdstr,"echo %d 0 0 > /proc/net/rtl8188eu/%s/monitor\n", channel, IFNAME);
	//sprintf(cmdstr, cmd, channel);
	system(cmdstr); 
//	anyka_print("[%s]:channel %d hopping at %ld\n", __FUNCTION__, channel, time(0));
	anyka_print("jwy--hop: %d, t: %ld\n",  channel, time(0));
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

	// the ssid store at /tmp/wireless/
	//strncpy(wifi_info.ssid, pSmartlinkParam->sz_ssid, sizeof(wifi_info.ssid));
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
	int i;
	char buf[128];
	char *result = NULL;

	char realtek[][16] = {
		"rtl8188eu",
		"rtl8189es",
	};

	for (i = 0 ; i < sizeof(realtek)/sizeof(realtek[0]); i++) {
		sprintf(buf, "/proc/net/%s", realtek[i]);
		if (access(buf, R_OK) == 0) {
			result = realtek[i];
		}
	}

	if (NULL == result) {
		return -1;
	}

	strcpy(change_channel_cmd, "echo %d 0 0 > ");
	sprintf(buf, "/proc/net/%s/%s/monitor\n", result, IFNAME);
	strcat(change_channel_cmd, buf);
	//printf("find ch chn cmd: %s\n", change_channel_cmd);
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
int is_realtek(void)
{
	int i;
	char buf[128];
	char *result = NULL;

	char realtek[][16] = {
		"rtl8188eu",
		"rtl8189es",
	};

	for (i = 0 ; i < sizeof(realtek)/sizeof(realtek[0]); i++) {
		sprintf(buf, "/proc/net/%s", realtek[i]);
		if (access(buf, R_OK) == 0) {
			result = realtek[i];
		}
	}

	if (NULL == result) {
		return 0;
	}

	return 1;
}

//#define NEW_IF
#ifdef NEW_IF
char change_chn_cmd[128];
void smartlink_hopping_cb(int nchannel)
{
	sniffer_hopping(nchannel, change_chn_cmd);
}

int realtek_smartlink(void)
{
	if (find_change_channel_cmd(change_chn_cmd)) {
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
    start_smartlink(smartlink_hopping_cb, "wlan0", on_smartlink_notify, serial_number, WCT_REALTEK8188, 
                    hopping_time, sniffer_time, NULL);

    while(g_sniffer_running)
    {
        sleep(1);
    }
    
    return 0;
}
#else
/**
* @brief 计算时间差
* @author ye_guohong
* @date 2015-03-31
* @param[out]   tv_sub      时间差值保存位置
* @param[in]    tv_start    开始时间
* @param[in]    tv_end      结束时间
* @return void
* @retval
*/
static void timeval_subtract(struct timeval *tv_sub,struct timeval *tv_start,struct timeval *tv_end)
{
	if (tv_end->tv_usec >= tv_start->tv_usec)
	{
	    tv_sub->tv_sec = tv_end->tv_sec - tv_start->tv_sec;
	    tv_sub->tv_usec = tv_end->tv_usec - tv_start->tv_usec;
    }
	else {
	    tv_sub->tv_sec = tv_end->tv_sec - tv_start->tv_sec - 1;
	    tv_sub->tv_usec = 1000000 - tv_start->tv_usec + tv_end->tv_usec;
	}
}

/**
* @brief realtek smartlink总入口
* @author ye_guohong
* @date 2015-03-31
* @param[out]   
* @return int
* @retval if return 0 success, otherwise failed
*/
int realtek_smartlink(void)
{
	char change_chn_cmd[128];

	if (find_change_channel_cmd(change_chn_cmd)) {
		printf("Err: cann't find change channel cmd\n"); 
		exit(EXIT_FAILURE);
	}

	/*
	* 设置网卡工作模式为 监听模式，需要调用对应的驱动接口 .......
	*/
	struct ifreq ifopts;
	int sockopt, sockfd;

	//Create a raw socket that shall sniff
	/* Open PF_PACKET socket, listening for EtherType ETHER_TYPE */
	if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
		printf("Err: listener socket\n"); 
		exit(EXIT_FAILURE);
	}

	/* Set interface to promiscuous mode - do we need to do this every time? */
	strncpy(ifopts.ifr_name, IFNAME, IFNAMSIZ-1);
	ioctl(sockfd, SIOCGIFFLAGS, &ifopts);

	ifopts.ifr_flags |= IFF_PROMISC;
	ioctl(sockfd, SIOCSIFFLAGS, &ifopts);
	/* Allow the socket to be reused - incase connection is closed prematurely */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
		printf("Err: setsockopt\n");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	/* Bind to device */
	if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, IFNAME, IFNAMSIZ-1) == -1) {
		printf("Err: SO_BINDTODEVICE, err=%s\n", strerror(errno));
		close(sockfd);
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
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	anyka_print("init_wifi_sync serial_number[%s]\n",serial_number);

	init_wifi_sync(on_smartlink_notify, serial_number, IFNAME);

	/*
	* 抓包 分析 重点是跳频逻辑处理，跳频间隔建议100ms 抓包时长建议4s ..................
	*/
	uint8_t *buf = NULL;
	ssize_t numbytes;
	int current_channel = 1;
	struct timeval tv_start,tv_end,tv_sub;
	int i;
	long ms_interval = CHANNEL_WAIT_TIME;

	buf = malloc(BUF_SIZ);
	if (NULL == buf) {
		anyka_print("malloc error.\n");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	sniffer_hopping(current_channel, change_chn_cmd);
	gettimeofday(&tv_start, NULL);

	while(g_sniffer_running)
	{
		int  ref_channel;
		memset(buf, 0, BUF_SIZ);

		numbytes = recvfrom(sockfd, buf, BUF_SIZ - 1 , 0, NULL, NULL);
		i = fill_80211_frame(buf,numbytes, buf[2],&ref_channel);
		if(QLERROR_LOCK == i ) // 2表示锁定指定信道
		{
			ms_interval = CHANNEL_SNIFFER_TIME; //锁定频道，监听包
			if (current_channel !=ref_channel) //当前信道和评估的信道不一样
			{
				current_channel = ref_channel;
				sniffer_hopping(current_channel, change_chn_cmd);   // 跳到下一个频道
				gettimeofday(&tv_start, NULL);
				anyka_print("smartlink recv beacon and lock channle %d,time=%ld\n",ref_channel,time(NULL));
			}
		}
		else if (QLERROR_HOP == i) //立即切换到下一频道
		{
			//anyka_print("smartlink recv one multicast packet,time=%ld\n",time(NULL));
		}

		gettimeofday(&tv_end, NULL);
		timeval_subtract(&tv_sub, &tv_start, &tv_end);
		
		if(tv_sub.tv_sec * 1000+tv_sub.tv_usec / 1000 >= ms_interval)
		{
			if(++current_channel > 13)
				current_channel = 1;
			
			ms_interval = CHANNEL_WAIT_TIME; 
			sniffer_hopping(current_channel, change_chn_cmd);   // 跳到下一个频道
			wifi_sync_notify_hop(current_channel);
			gettimeofday(&tv_start, NULL);
		}
	}

	free(buf);
	close(sockfd);
	destory_wifi_sync();
	return 0; ;
}
#endif

