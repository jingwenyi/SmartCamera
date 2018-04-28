#include "common.h"
#include "anyka_ini.h"
#define AP				"/etc/jffs2/ap_list"						// scan ap list
#define LINE 				1024
#define AP_SIZE 			8096

/**
 * *  @brief       get val from str
 * *  @author      gao wangsheng
 * *  @date        2013-5-9
 * *  @param[in]   str, name, val 
 * *  @return	   1 on success	
 * */
int cgiGetVal(const char *str, const char *name, char *val)
{
	str = strstr(str, name);
	if(str)
	{
		for(str += strlen(name); (*str) && (*str != '"') && (*str != '\n'); )
		{
			*val++ = *str++;
		}
	}
	
	*val = '\0';
	return str != NULL;
}

/**
 * *  @brief       scan ap info to file
 * *  @author      gao wangsheng
 * *  @date        2013-5-9
 * *  @param[in]   void
 * *  @return	   void	
 * */
void ScanOtherAp2File(void)
{
	char cmd[100];

	// scan other AP to ap_list
	sprintf(cmd, "/sbin/iwlist wlan0 scanning > %s", AP);
	system(cmd);
}

/**
 * *  @brief       get one ap buf
 * *  @author      gao wangsheng
 * *  @date        2013-5-9
 * *  @param[in]   info
 * *  @return      zero on success
 * */
static int cgiAllocOneAp(struct ap_shop *info)
{
	info->ap_num++;
	if (info->ap_num >= sizeof(info->ap_list))
	{
		info->ap_num--;
		anyka_print("[%s:%d] ap list=%d is full!", __func__, __LINE__, info->ap_num);
		return -1;
	}
	return 0;
}


/**
 * *  @brief       init one ap buf
 * *  @author      gao wangsheng
 * *  @date        2013-5-9
 * *  @param[in]   info
 * *  @return      none
 * */
static void cgiInitApInfo(struct ap_shop *info)
{		
	memset(info, 0, sizeof(struct ap_shop));
	info->ap_num = -1;
}
/**
 * *  @brief       check security
 * *  @author      gao wangsheng
 * *  @date        2013-5-9
 * *  @param[in]   str, name, val 
 * *  @return	   none zero on success	
 * */
int cgiCheckSecurity(char *buf)
{
	int val = 0;

	if(strstr(buf, "Encryption key:on"))
	{
		if (strstr(buf, "WPA"))
		{
			val = WIFI_ENCTYPE_WPA_TKIP;
		}
		else if(strstr(buf, "WPA2"))
		{
			val = WIFI_ENCTYPE_WPA2_TKIP;
		}
		
	}
	else if(strstr(buf, "Encryption key:off"))
	{
		val = WIFI_ENCTYPE_NONE;
	}

	return val;
}


/**
 * *  @brief       get an ap infom from buf
 * *  @author      gao wangsheng
 * *  @date        2013-5-9
 * *  @param[in]   buf, ap
 * *  @return      non zero on valid ap
 * */
static int cgiGetOneAp(char *buf, struct ap *ap)
{
	cgiGetVal(buf, "Address: ", ap->address);
	cgiGetVal(buf, "ESSID:\"", ap->ssid);
	cgiGetVal(buf, "Protocol:", ap->protocol);
	cgiGetVal(buf, "Mode:", ap->mode);
	cgiGetVal(buf, "Frequency:", ap->frequency);
	cgiGetVal(buf, "Encryption key:", ap->en_key);
	cgiGetVal(buf, "Bit Rates:", ap->bit_rates);
	cgiGetVal(buf, "level=", ap->sig_level);
	ap->security = cgiCheckSecurity(buf);
	return strlen(ap->ssid);	
}


/**
 * *  @brief       scan ap info
 * *  @author      gao wangsheng
 * *  @date        2013-5-9
 * *  @param[in]   info
 * *  @return      zero on success
 * */
static int cgiScanOtherAp(struct ap_shop *info)
{
	int error = 0;
	char *line;
	char *buffer;
	FILE *ap_file;
	struct ap ap_temp;

	line   = (char *)malloc(1000);
	buffer = (char *)malloc(1000*10);

	anyka_print("%s: Scanning other available wifi AP...\n", __func__);
	
	ScanOtherAp2File();
	
	ap_file = fopen(AP, "r");
	if (NULL == ap_file)
	{
		anyka_print("[%s:%d] open file %s fail!\n", __func__, __LINE__, AP);
		goto fail;
	}
	
	memset(line, '\0', sizeof(line));

	while ((!feof(ap_file)) && (strstr(line, "Address:") == NULL))
	{
		fgets(line, LINE, ap_file); 
	}
	
	for (; (!feof(ap_file)) && (strstr(line, "Address:") != NULL); )
	{
		memset(buffer, '\0', sizeof(buffer));
		strcat(buffer, line);

		// get one AP info to buffer
		for (fgets(line, LINE, ap_file); (!feof(ap_file)) && (strstr(line, "Address:") == NULL); /*nul*/)
		{
			strcat(buffer, line);
			fgets(line, LINE, ap_file);
		}

		// copy one ap info to list
		memset(&ap_temp, 0, sizeof(struct ap));
		if (cgiGetOneAp(buffer, &ap_temp))
		{	
			error = cgiAllocOneAp(info);
			
			if (error)
			{
				anyka_print("[%s:%d] cgi Alloc One Ap fail!\n", __func__, __LINE__);
				fclose(ap_file);
				goto fail;
			}
			
			memcpy(&info->ap_list[info->ap_num], &ap_temp, sizeof(struct ap));
			info->ap_list[info->ap_num].index = info->ap_num;
		}
	}

	free(line);
	free(buffer);
	fclose(ap_file);
	
	return 0;
fail:
	free(line);
	free(buffer);
	return -1;	
}


/**
 * *  @brief       search ap info
 * *  @author      gao wangsheng
 * *  @date        2013-5-9
 * *  @param[in]   setting
 * *  @return      error
 * */
int doWirelessSearch(struct ap_shop *ap)
{
	int error;
	struct ap_shop *ap_info = ap;
	
	cgiInitApInfo(ap_info);
	//error = cgiScanConnectAp(ap_info);
	error = cgiScanOtherAp(ap_info);
	
	return error;
}



