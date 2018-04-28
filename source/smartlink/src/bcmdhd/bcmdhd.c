#include <stdio.h>
#include <unistd.h>
#include "easy_setup.h"
#include "cooee.h"
#include "neeze.h"
#include "akiss.h"
#include "TXDeviceSDK.h"
#include "TXWifisync.h"

int debug_enable = 0;

/**
* @brief 是否加载了bcmdhd wifi驱动
* @author ye_guohong
* @date 2015-03-31
* @param[in]    void
* @return int
* @retval if return 0 success, otherwise failed
*/
int is_bcmdhd(void)
{
	char *bcmdhd_dir = "/sys/module/bcmdhd";

	if (access(bcmdhd_dir, R_OK) == 0) {
		return 1;
	}

	return 0;
}

/*
void usage() {
    printf("-h: show help message\n");
    printf("-d: show debug message\n");
    printf("-k <v>: set 16-char key for all protocols\n");
    printf("-q: enable qqconnect\n");
    printf("-p <v>: bitmask of protocols to enable\n");
    printf("  0x%04x - cooee\n", 1<<EASY_SETUP_PROTO_COOEE);
    printf("  0x%04x - neeze\n", 1<<EASY_SETUP_PROTO_NEEZE);
    printf("  0x%04x - akiss\n", 1<<EASY_SETUP_PROTO_AKISS);
}	
int main(int argc, char* argv[])
{
    int ret;
    int len;
    uint16 val;

    for (;;) {
        int c = getopt(argc, argv, "dhk:p:q");
        if (c < 0) {
            break;
        }

        switch (c) {
            case 'd':
                debug_enable = 1;
                break;
            case 'k':
                cooee_set_key(optarg);
                neeze_set_key(optarg);
                akiss_set_key(optarg);
                break;
            case 'p':
                sscanf(optarg, "%04x", &val);
                easy_setup_enable_protocols(val);
                break;
            case 'q':
                easy_setup_enable_qqcon();
                break;
            case 'h':
                usage();
                return 0;
            default:
                usage();
                return 0;
        }
    }
*/
int bcmdhd_smartlink(void) 
{
	int ret;
	uint16 val = 1;
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

    /* set decrypt key if you have set it in sender */
    ret = cooee_set_key(serial_number);
    if (ret)
		exit(EXIT_FAILURE);
		
    easy_setup_enable_protocols(val);		
		
    easy_setup_enable_qqcon();


    ret = easy_setup_start();
    if (ret) return ret;

    /* query for result, blocks until cooee comes or times out */
    ret = easy_setup_query();
    if (!ret) {
        char ssid[33]; /* ssid of 32-char length, plus trailing '\0' */
        ret = easy_setup_get_ssid(ssid, sizeof(ssid));
        if (!ret) {
            printf("ssid: %s\n", ssid);
        }

        char password[65]; /* password is 64-char length, plus trailing '\0' */
        ret = easy_setup_get_password(password, sizeof(password));
        if (!ret) {
            printf("password: %s\n", password);
        }

        uint16 protocol;
        char ip[16]; /* ipv4 max length */ 
        uint16 port;             
        ret = easy_setup_get_protocol(&protocol);
        if (ret) {
            printf("failed getting protocol.\n");
        } else if (protocol == EASY_SETUP_PROTO_COOEE) {
         //   char ip[16]; /* ipv4 max length */ 
            ret = cooee_get_sender_ip(ip, sizeof(ip));
            if (!ret) {
                printf("sender ip: %s\n", ip);
            }

         //   uint16 port;      
            ret = cooee_get_sender_port(&port);
            if (!ret) {
                printf("sender port: %d\n", port);
            }
        } else if (protocol == EASY_SETUP_PROTO_NEEZE) {
         //   char ip[16]; /* ipv4 max length */
            ret = neeze_get_sender_ip(ip, sizeof(ip));
            if (!ret) {
                printf("sender ip: %s\n", ip);
            }

         //   uint16 port;
            ret = neeze_get_sender_port(&port);
            if (!ret) {
                printf("sender port: %d\n", port);
            }
        }/* else if (protocol == EASY_SETUP_PROTO_AKISS) {
            uint8 random;
            ret = akiss_get_random(&random);
            if (!ret) {
                printf("random: 0x%02x\n", random);
            }
        } */else {      	
           printf("unknown protocol %d\n", protocol);
        }

        /* if easy_setup_get_security() returns -1, try it more times */
        ret = easy_setup_get_security();
        printf("security: ");
        if (ret == WLAN_SECURITY_WPA2) printf("wpa2\n");
        if (ret == WLAN_SECURITY_WPA) printf("wpa\n");
        if (ret == WLAN_SECURITY_WEP) printf("wep\n");
        if (ret == WLAN_SECURITY_NONE) printf("none\n");
   	
        struct smartlink_wifi_info wifi_info;
        strncpy(wifi_info.ssid, ssid, sizeof(wifi_info.ssid));
        strncpy(wifi_info.passwd, password, sizeof(wifi_info.passwd));
        store_wifi_info(&wifi_info);

        save2file(ip, port);
        //tx_ack_app((unsigned int)ip, (unsigned int)port);

		if(save_ssid_to_tmp(ssid, strlen(ssid)))
			anyka_print("[%s] fails to save ssid to tmp\n", __func__);
		else
			anyka_print("[%s] save ssid to tmp\n", __func__);
    }

    /* must do this! */
    easy_setup_stop();

    return 0;
}

