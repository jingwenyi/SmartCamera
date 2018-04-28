
/**
 * JUAN onvif£\n
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <NkUtils/log.h>
#include <NkUtils/macro.h>

#include "basetype.h"
#include "common.h"
#include "anyka_config.h"
#include "isp_module.h"
#include "ak_n1_interface.h"


#include "onvif.h"
#include "env_common.h"

#define JA_PROP_IPV4_STR(__Prop, __text, __size) \
	snprintf(__text, (__size), "%d.%d.%d.%d",\
		(NK_Int)(__Prop[0]),\
		(NK_Int)(__Prop[1]),\
		(NK_Int)(__Prop[2]),\
		(NK_Int)(__Prop[3]))


static int _get_system_information(lpNVP_DEV_INFO info)
{
	strcpy(info->manufacturer, "JIUAN");
	strcpy(info->devname,"IPCAM");
	strcpy(info->model, "IPCAM");
	strcpy(info->sn, "JUAN1234567890");
	strcpy(info->firmware, "1.1.1.1");
	strcpy(info->sw_version, "1.0.0.0");
	strcpy(info->sw_builddate, "2015-11-18");
	strcpy(info->hw_version, "1.1.0.0");
	strcpy(info->hwid, "HW000");
	return 0;
}
static int _get_date_time(lpNVP_SYS_TIME systime)
{
	time_t t_now;
	struct tm *ptm;
	struct tm tm_local, tm_gm;
	
	systime->ntp_enable = AK_FALSE;
	strcpy(systime->ntp_server, "");
	systime->tzone = -800;

	time(&t_now);
	localtime_r(&t_now, &tm_local);
	gmtime_r(&t_now, &tm_gm);

	ptm = &tm_local;
	systime->local_time.date.year = ptm->tm_year;
	systime->local_time.date.month = ptm->tm_mon;
	systime->local_time.date.day = ptm->tm_mday;
	systime->local_time.time.hour = ptm->tm_hour;
	systime->local_time.time.minute = ptm->tm_min;
	systime->local_time.time.second = ptm->tm_sec;		
	ptm = &tm_gm;
	systime->gm_time.date.year = ptm->tm_year;
	systime->gm_time.date.month = ptm->tm_mon;
	systime->gm_time.date.day = ptm->tm_mday;
	systime->gm_time.time.hour = ptm->tm_hour;
	systime->gm_time.time.minute = ptm->tm_min;
	systime->gm_time.time.second = ptm->tm_sec;	
	return 0;
}

static int _set_date_time(lpNVP_SYS_TIME systime)
{		
	char cmd[256] = {0};	//cmd buffer
	sprintf(cmd, "date \"%d-%d-%d %d:%d:%d\"", systime->gm_time.date.year+1900, systime->gm_time.date.month+1, systime->gm_time.date.day,
						systime->gm_time.time.hour+8, systime->gm_time.time.minute, systime->gm_time.time.second);

	if(system(cmd) < 0)			//system cmd,execute "system date \XX-XX-XX XX:XX:XX\"
	{	
		return -1;	
	}
	bzero(cmd, sizeof(cmd));  //clean buffer

    //save to rtc
	sprintf(cmd, "hwclock -w");
	if(system(cmd) < 0)
	{	
		return -1;	
	}

	
	return 0;
}


static unsigned int _get_dhcp_status(void)
{
	char buf[10] = {0};
	
	int fd = open("/tmp/dhcp_status", O_RDONLY);
	if(fd < 0) {
		perror("open");
		return 0;
	}
	read(fd, buf, 1);

	close(fd);
	//anyka_print("[%s:%d] dhcp: %d\n", __func__, __LINE__, atoi(buf));

	return atoi(buf);
}


static int _get_interface(lpNVP_ETHER_CONFIG ether)
{
	int i = 0;
	char ip[64]={0};
	char mac[64]={0};
	
	//ifconfig eth0
    do_syscmd("ifconfig eth0 | grep 'inet addr' | awk '{print $2}'", ip);
    while(ip[i])
    {
        if (ip[i]=='\n' || ip[i]=='\r' || ip[i]==' ')
            ip[i] = '\0';
        i++;
    }

	//anyka_print("_get_interface: ip  = %s.\n", &ip[5]);

	do_syscmd("ifconfig eth0 | grep 'HWaddr' | awk '{print $5}'", mac);
    while(mac[i])
    {
        if (mac[i]=='\n' || mac[i]=='\r' || mac[i]==' ')
            mac[i] = '\0';
        i++;
    }

	//anyka_print("_get_interface: mac  = %s.\n", mac);
    
    Psystem_net_set_info sys_net_info = anyka_get_sys_net_setting();

	ether->dhcp = _get_dhcp_status();
	
	NVP_IP_INIT_FROM_STRING(ether->ip, &ip[5]);
	NVP_IP_INIT_FROM_STRING(ether->netmask, sys_net_info->netmask);
	NVP_IP_INIT_FROM_STRING(ether->gateway, sys_net_info->gateway);
    NVP_MAC_INIT_FROM_STRING(ether->mac, mac);
	NVP_IP_INIT_FROM_STRING(ether->dns1, sys_net_info->firstdns);
	NVP_IP_INIT_FROM_STRING(ether->dns2, sys_net_info->backdns);
	ether->http_port = 80;//port_n.value;
	ether->rtsp_port = 554;//port_n.value;

	return 0;
}

static int _set_interface(lpNVP_ETHER_CONFIG ether)
{
	char cmd[128] = {0};
	char ipaddr[32], netmask[32], gateway[32];

	JA_PROP_IPV4_STR(ether->ip, ipaddr, sizeof(ipaddr));
	JA_PROP_IPV4_STR(ether->netmask, netmask, sizeof(netmask));
	JA_PROP_IPV4_STR(ether->gateway, gateway, sizeof(gateway));

	sprintf(cmd, "ifconfig eth0 %s netmask %s", ipaddr, netmask);
	system(cmd);

	sprintf(cmd, "route add default gw %s", gateway);
	system(cmd);


	Psystem_net_set_info net_info = anyka_get_sys_net_setting();
    
	net_info->dhcp = 0;
	strcpy(net_info->ipaddr, ipaddr);
	strcpy(net_info->netmask, netmask);
	strcpy(net_info->gateway, gateway);

	anyka_set_sys_net_setting(net_info);
				
	return 0;
}

static int _get_color(lpNVP_COLOR_CONFIG color, int id)
{
	color->brightness = (float)g_img_val.brightness;
	color->contrast = (float)g_img_val.contrast;
	color->hue = (float)g_img_val.hue;
	color->saturation = (float)g_img_val.saturation;
	color->sharpness = (float)g_img_val.sharp;
	
	//printf("get color hue:%.0f, brightness:%.0f, saturation:%.0f, contrast:%.0f\n", color->hue, color->brightness, color->saturation, color->contrast);

	return 0;
}


static int _set_color(lpNVP_COLOR_CONFIG color, int id)
{

	printf("set color hue:%.0f, brightness:%.0f, saturation:%.0f, contrast:%.0f\n", color->hue, color->brightness, color->saturation, color->contrast);

	Isp_Effect_Set(EFFECT_HUE, color->hue - IMG_EFFECT_DEF_VAL);
	Isp_Effect_Set(EFFECT_BRIGHTNESS, color->brightness - IMG_EFFECT_DEF_VAL);
	Isp_Effect_Set(EFFECT_SATURATION, color->saturation - IMG_EFFECT_DEF_VAL);
	Isp_Effect_Set(EFFECT_CONTRAST, color->contrast - IMG_EFFECT_DEF_VAL);

	g_img_val.hue = (T_U32)color->hue;
	g_img_val.brightness = (T_U32)color->brightness;
	g_img_val.contrast = (T_U32)color->contrast;
	g_img_val.saturation = (T_U32)color->saturation;
	g_img_val.sharp = (T_U32)color->sharpness;

	ak_n1_save_video_image_setting(&g_img_val);
	return 0;
}

static int _get_image_option(lpNVP_IMAGE_OPTIONS image, int id)
{
	// FIX me
	image->brightness.min = 0;
	image->brightness.max = 100;
	image->saturation.min = 0;
	image->saturation.max = 100;
	image->sharpness.min = 0;
	image->sharpness.max = 100;
	image->contrast.min = 0;
	image->contrast.max = 100;
	image->hue.min = 0;
	image->hue.max = 100;

	image->ircut_mode.nr = 3;
	image->ircut_mode.list[0] = 0;
	image->ircut_mode.list[1] = 1;
	image->ircut_mode.list[2] = 2;

	return 0;
}

static int _get_image(lpNVP_IMAGE_CONFIG image, int id)
{
	image->ircut.control_mode = 0;
	image->ircut.ircut_mode = 0;

	image->wdr.enabled = AK_FALSE;
	image->wdr.WDRStrength = 0;

	image->manual_sharp.enabled = AK_FALSE;
	image->manual_sharp.sharpnessLevel = 0;

	image->d3d.enabled = AK_FALSE;
	image->d3d.denoise3dStrength = 0;

	_get_color(&image->color, id);

	_get_image_option(&image->option, id);
	
	return 0;
}

static int _set_image(lpNVP_IMAGE_CONFIG image, int id)
{
	_set_color(&image->color, id);

	return 0;
}


static int _get_video_source(lpNVP_V_SOURCE src, int id)
{
	if (0 == id)
	{
		src->resolution.width = 1280;
		src->resolution.height = 720;	
	}
	else if (1 == id)
	{
		src->resolution.width = 640;
		src->resolution.height = 360;	
	}

	src->fps = 25;

	_get_image(&src->image, id);
	
	return 0;
}

static int _set_video_source(lpNVP_V_SOURCE src, int id)
{	
	anyka_print("name %s, token %s, fps %f, width %d, height %d\n", src->name,src->token,src->fps,src->resolution.width,src->resolution.height);
	return 0;
}


static int _get_video_input_conf(lpNVP_VIN_CONFIG vin, int id)
{
	vin->rect.nX = 0;
	vin->rect.nY = 0;

	if (0 == id)
	{
		vin->rect.width = 1280;
		vin->rect.height = 720;	
	}
	else if (1 == id)
	{
		vin->rect.width = 640;
		vin->rect.height = 360;	
	}

	vin->rotate.enabled = AK_FALSE;
	
	vin->rotate.degree = 0;
	return 0;
}

static int _set_video_input_conf(lpNVP_VIN_CONFIG vin, int id)
{	
	return 0;
}

static int _get_video_encode_option(lpNVP_VENC_OPTIONS venc, int id)
{
	int n;
	int fps_min = 25, fps_max  = 25;
	int bps_min = 32, bps_max = 4000;
	int resolution_nr = 1;
	int width[16] = {640, };
	int height[16] = {360, };

	if (0 == id)
	{
		width[0] = 1280;
		height[0] = 720;	
		bps_min = 256;
		bps_max = 4096;
	}
	else if (1 == id)
	{
		width[0] = 640;
		height[0] = 360;	
		bps_min = 128;
		bps_max = 1024;
	}

	// FIM Me
	venc->enc_fps.min = fps_min;
	venc->enc_fps.max = fps_max;

	venc->enc_gov.min = 1;
	venc->enc_gov.max = fps_max;

	venc->enc_interval.min = 1;
	venc->enc_interval.max = 1;
	
	venc->enc_quality.min = 0;
	venc->enc_quality.max = 4;

	
	venc->enc_bps.min  = bps_min;
	venc->enc_bps.max = bps_max;
	
	venc->resolution_nr = resolution_nr;
	for ( n = 0; n < resolution_nr; n++) {
		NVP_SET_SIZE(&venc->resolution[n], width[n], height[n]);
	}

	venc->enc_profile_nr = 1;
	venc->enc_profile[0] = NVP_H264_PROFILE_MAIN;

	return 0;	
}

static int _get_video_encode(lpNVP_VENC_CONFIG venc, int id)
{
	if (0 == id)
	{
		venc->width = 1280;
		venc->height = 720;	
		venc->enc_bps = g_video_set.mainbps;
	}
	else if (1 == id)
	{
		venc->width = 640;
		venc->height = 360;	
		venc->enc_bps = g_video_set.subbps;
	}
	
	venc->enc_fps = 25;
	venc->enc_gov = 25;
	venc->enc_interval = 1;
	venc->quant_mode = g_video_set.videomode + NVP_QUANT_CBR;

	venc->enc_type = NVP_VENC_H264;
	venc->enc_profile = NVP_H264_PROFILE_MAIN;//NVP_H264_PROFILE_MAIN;
	venc->user_count = 4;

	venc->option.index = venc->index;
	strcpy(venc->option.token, venc->token);
	strcpy(venc->option.enc_token, venc->enc_token);
	_get_video_encode_option(&venc->option, id);

	return 0;
}


static int _set_video_encode(lpNVP_VENC_CONFIG venc, int id)
{
	anyka_print("width %d,",venc->width);
	anyka_print("heigth %d,",venc->height);
	anyka_print("quant_mode %d,",venc->quant_mode);
	anyka_print("enc_fps %d,enc_bps %d,enc_gov %d ,enc_type", venc->enc_fps,venc->enc_bps, venc->enc_gov);

	if (venc->enc_bps < venc->option.enc_bps.min)
	{
		venc->enc_bps = venc->option.enc_bps.min;
	}
	else if (venc->enc_bps > venc->option.enc_bps.max)
	{
		venc->enc_bps = venc->option.enc_bps.max;
	}
	
	if (0 == id)
	{
		video_set_encode_bps(FRAMES_ENCODE_RECORD, venc->enc_bps);
		g_video_set.mainbps = venc->enc_bps;
	}
	else if(1 == id)
	{
		video_set_encode_bps(FRAMES_ENCODE_VGA_NET, venc->enc_bps);
		g_video_set.subbps = venc->enc_bps;
	}

	T_BOOL bNeedRestart = AK_FALSE;

	if (g_video_set.videomode  + NVP_QUANT_CBR != venc->quant_mode)
	{
		g_video_set.videomode = venc->quant_mode - NVP_QUANT_CBR;
		bNeedRestart = AK_TRUE;
	}

	ak_n1_set_video_setting(&g_video_set);

	if (bNeedRestart)
	{
		anyka_print("video mode changed, restart anyka_ipc!");
		exit(0);
	}
	
	return 0;
}

static int _get_audio_input(lpNVP_AIN_CONFIG ain, int id)
{
	return 0;
}

static int _set_audio_input(lpNVP_AIN_CONFIG ain, int id)
{
	return 0;
}

static int _get_audio_encode(lpNVP_AENC_CONFIG aenc, int id)
{
	//FIX me
	aenc->channel = 1;
	aenc->enc_type = NVP_AENC_G711;
	aenc->sample_size = 8;
	aenc->sample_rate = 8000;
	
	aenc->user_count = 2;
	return 0;
}

static int _set_audio_encode(lpNVP_AENC_CONFIG aenc, int id)
{
	anyka_print("index %d,name %s,token %s,user_count %d,enc_type %d,sample_rate%d,sample_size %d\n",aenc->index,aenc->name,aenc->token,aenc->user_count,aenc->enc_type,aenc->sample_rate,aenc->sample_size);
	return 0;
}

static int _get_motion_detection(lpNVP_MD_CONFIG md, int id)
{
	md->type = NVP_MD_TYPE_GRID;

	md->grid.columnGranularity = 22;
	md->grid.rowGranularity = 15;
	md->grid.sensitivity = 80;
	memset(md->grid.granularity, 0xff, sizeof(md->grid.granularity));
	//FIM me
	md->grid.threshold = 5;
	// FIX me
	md->delay_off_alarm = 300;
	md->delay_on_alarm = 200;
	
	return 0;
}

static int _set_motion_detection(lpNVP_MD_CONFIG md, int id)
{	
	return 0;
}

static int _get_video_analytic(lpNVP_VAN_CONFIG van, int id)
{
	return 0;
}

static int _set_video_analytic(lpNVP_VAN_CONFIG van, int id)
{
	return 0;
}


static int _get_ptz(lpNVP_PTZ_CONFIG ptz, int id)
{
	return 0;
}

static int _set_ptz(lpNVP_PTZ_CONFIG ptz, int id)
{
	return 0;
}

static int _get_profile(lpNVP_PROFILE_CHN profile)
{
	int i;
	// FIX me
	profile->profile_nr = 2;
	profile->venc_nr = profile->profile_nr;
	profile->aenc_nr = 1;

	for (i = 0; i < profile->venc_nr; i++) {
		_get_video_encode(&profile->venc[i], i);
	}
	for (i = 0; i < profile->aenc_nr; i++) {
		_get_audio_encode(&profile->aenc[i], i);
	}
	_get_video_source(&profile->v_source, profile->index);
	for (i = 0; i < profile->vin_conf_nr; i++) {
		_get_video_input_conf(&profile->vin[i], i);
	}
	_get_audio_input(&profile->ain, profile->index);
	_get_ptz(&profile->ptz, profile->index);
	_get_video_analytic(&profile->van, profile->index);
	_get_motion_detection(&profile->md, profile->index);

	return 0;
}

static int _set_profile(lpNVP_PROFILE_CHN profile)
{
	int i;
	int ret = 0;
	//
	for (i = 0; i < profile->venc_nr; i++) {
		if (_set_video_encode(&profile->venc[i], profile->index) < 0)
			ret = -1;
	}
	for (i = 0; i < profile->aenc_nr; i++) {
		if (_set_audio_encode(&profile->aenc[i], profile->index) < 0)
			ret = -1;
	}
	if (_set_video_source(&profile->v_source, profile->index) < 0)
		ret = -1;
	for (i = 0; i < profile->vin_conf_nr; i++) {
		if (_set_video_input_conf(&profile->vin[i], profile->index) < 0)
			ret = -1;
	}
	if (_set_audio_input(&profile->ain, profile->index) < 0)
		ret = -1;
	if (_set_ptz(&profile->ptz, profile->index) < 0)
		ret = -1;
	if (_set_video_analytic(&profile->van, profile->index) < 0)
		ret = -1;
	if (_set_motion_detection(&profile->md, profile->index) < 0)
		ret = -1;

	return ret;
}

static int _get_profiles(lpNVP_PROFILE profiles)
{
	int i;

	profiles->chn = NVP_MAX_CH;
	//
	for ( i = 0; i < profiles->chn; i++) {
		_get_profile(&profiles->profile[i]);
	}
	return 0;
}

static int _set_profiles(lpNVP_PROFILE profiles)
{
	int i;
	int ret = 0;
	//
	for ( i = 0; i < profiles->chn; i++) {
		if(_set_profile(&profiles->profile[i]) < 0)
			ret = -1;
	}
	return ret;
}

static int _get_all(lpNVP_ENV env)
{
	_get_system_information(&env->devinfo);
	_get_date_time(&env->systime);
	_get_interface(&env->ether);
	_get_profiles(&env->profiles);

	return 0;
}

static int _set_all(lpNVP_ENV env)
{
	int ret = 0;

	if (_set_date_time(&env->systime) < 0)
		ret = -1;
	if (_set_interface(&env->ether) < 0)
		ret = -1;
	if (_set_profiles(&env->profiles) < 0)
		ret = -1;

	return ret;	
}

static void _cmd_system_boot(long l, void *r)
{
	//TICKER_del_task(_cmd_system_boot);
	anyka_print("system reboot now...\n");
	system("reboot");
}

static int _cmd_ptz(lpNVP_CMD cmd, const char *module, int keyid)
{
	const char *ptz_cmd_name[] = 
	{
		"PTZ_CMD_UP",
		"PTZ_CMD_DOWN",
		"PTZ_CMD_LEFT",
		"PTZ_CMD_RIGHT",
		"PTZ_CMD_LEFT_UP",
		"PTZ_CMD_RIGHT_UP",
		"PTZ_CMD_LEFT_DOWN",
		"PTZ_CMD_RIGHT_DOWN",
		"PTZ_CMD_AUTOPAN",
		"PTZ_CMD_IRIS_OPEN",
		"PTZ_CMD_IRIS_CLOSE",
		"PTZ_CMD_ZOOM_IN",
		"PTZ_CMD_ZOOM_OUT",
		"PTZ_CMD_FOCUS_FAR",
		"PTZ_CMD_FOCUS_NEAR",
		"PTZ_CMD_STOP",
		"PTZ_CMD_WIPPER_ON",
		"PTZ_CMD_WIPPER_OFF",
		"PTZ_CMD_LIGHT_ON",
		"PTZ_CMD_LIGHT_OFF",
		"PTZ_CMD_POWER_ON",
		"PTZ_CMD_POWER_OFF",
		"PTZ_CMD_GOTO_PRESET",
		"PTZ_CMD_SET_PRESET",
		"PTZ_CMD_CLEAR_PRESET",
		"PTZ_CMD_TOUR",
	};

	anyka_print("%s(%d)\n", ptz_cmd_name[cmd->ptz.cmd], cmd->ptz.cmd);
	switch(cmd->ptz.cmd) 
	{
		case NVP_PTZ_CMD_LEFT:
			break;
		case NVP_PTZ_CMD_RIGHT:
			break;
		case NVP_PTZ_CMD_UP:
			break;
		case NVP_PTZ_CMD_DOWN:
			break;
		case NVP_PTZ_CMD_ZOOM_IN:
			break;
		case NVP_PTZ_CMD_ZOOM_OUT:
			break;
		case NVP_PTZ_CMD_SET_PRESET:
			break;
		case NVP_PTZ_CMD_GOTO_PRESET:
			break;
		case NVP_PTZ_CMD_CLEAR_PRESET:
			break;
		case NVP_PTZ_CMD_STOP:
			break;
		default:
			break;
	}

	return 0;
}


static void _nvp_env_init(lpNVP_ENV env)
{
#define ONVIF_SET_NT(ptr, value)			snprintf(ptr, sizeof(ptr), "%s", value)
#define ONVIF_SET_NT_CHN(ptr, value,chn)	snprintf(ptr, sizeof(ptr), "%s%d", value, chn)
#define ONVIF_SET_NT_ID(ptr, value,chn, id)	snprintf(ptr, sizeof(ptr), "%s%d_%d", value, chn, id)
	int i, n;

	if (env == NULL)
		return;

	memset(&env->devinfo, 0, sizeof(stNVP_DEV_INFO));
	memset(&env->systime, 0, sizeof(stNVP_DATE_TIME));
	memset(&env->ether, 0, sizeof(stNVP_ETHER_CONFIG));
	memset(&env->profiles, 0, sizeof(stNVP_PROFILE));

	env->profiles.chn =  NVP_CH_NR;
	for ( n = 0; n < NVP_MAX_CH; n++)
	{
		env->profiles.profile[n].index = n;
		env->profiles.profile[n].profile_nr = NVP_VENC_NR;
		for ( i = 0; i < NVP_MAX_VENC; i++) {
			ONVIF_SET_NT_ID(env->profiles.profile[n].name[i], "Profile",n,  i);
			ONVIF_SET_NT_ID(env->profiles.profile[n].token[i], "ProfileToken", n,i);
			
			ONVIF_SET_NT_ID(env->profiles.profile[n].ain_name[i], "AIN", n,i);
			ONVIF_SET_NT_ID(env->profiles.profile[n].ain_token[i], "AINToken", n,i);
		}

		ONVIF_SET_NT_CHN(env->profiles.profile[n].v_source.name, "VIN", n);
		ONVIF_SET_NT_CHN(env->profiles.profile[n].v_source.token, "VINToken", n);
		ONVIF_SET_NT_CHN(env->profiles.profile[n].v_source.image.name, "IMG", n);
		ONVIF_SET_NT_CHN(env->profiles.profile[n].v_source.image.token, "IMGToken", n);
		ONVIF_SET_NT_CHN(env->profiles.profile[n].v_source.image.src_token, "VINToken", n);
		ONVIF_SET_NT_CHN(env->profiles.profile[n].v_source.image.color.src_token, "VINToken", n);
		
		env->profiles.profile[n].venc_nr = NVP_VENC_NR;
		for ( i = 0; i < NVP_MAX_VENC; i++) {
			env->profiles.profile[n].venc[i].index = i;
			ONVIF_SET_NT_ID(env->profiles.profile[n].venc[i].name, "Profile", n, i);
			ONVIF_SET_NT_ID(env->profiles.profile[n].venc[i].token, "ProfileToken", n, i);
			ONVIF_SET_NT_ID(env->profiles.profile[n].venc[i].enc_name, "VENC", n, i);
			ONVIF_SET_NT_ID(env->profiles.profile[n].venc[i].enc_token, "VENCToken", n, i);
		}

		env->profiles.profile[n].vin_conf_nr = NVP_VIN_IN_A_SOURCE;
		for ( i = 0; i < NVP_MAX_VIN_IN_A_SOURCE; i++) {
			env->profiles.profile[n].venc[i].index = i;
			ONVIF_SET_NT_ID(env->profiles.profile[n].vin[i].name, "VIN", n, i);
			ONVIF_SET_NT_ID(env->profiles.profile[n].vin[i].token, "VINToken", n, i);
		}

		ONVIF_SET_NT_CHN(env->profiles.profile[n].ain.name, "AIN", n);
		ONVIF_SET_NT_CHN(env->profiles.profile[n].ain.token, "AINToken", n);

		env->profiles.profile[n].aenc_nr = NVP_AENC_NR;
		for ( i = 0; i < NVP_MAX_AENC; i++) {
			env->profiles.profile[n].aenc[i].index = i;
			ONVIF_SET_NT_ID(env->profiles.profile[n].aenc[i].name, "AENC", n, i);
			ONVIF_SET_NT_ID(env->profiles.profile[n].aenc[i].token, "AENCToken", n, i);
		}

		ONVIF_SET_NT_CHN(env->profiles.profile[n].van.name, "VAN", n);
		ONVIF_SET_NT_CHN(env->profiles.profile[n].van.token, "VANToken", n);

		ONVIF_SET_NT_CHN(env->profiles.profile[n].md.rule_name, "MDRule", n);
		ONVIF_SET_NT_CHN(env->profiles.profile[n].md.module_name, "MDModule", n);

		ONVIF_SET_NT_CHN(env->profiles.profile[n].ptz.name, "PTZ", n);
		ONVIF_SET_NT_CHN(env->profiles.profile[n].ptz.token, "PTZToken", n);
		ONVIF_SET_NT_CHN(env->profiles.profile[n].ptz.node_name, "PTZNode", n);
		ONVIF_SET_NT_CHN(env->profiles.profile[n].ptz.node_token, "PTZNodeToken", n);
		env->profiles.profile[n].ptz.preset_nr = NVP_MAX_PTZ_PRESET;
		for ( i  = 0; i < NVP_MAX_PTZ_PRESET; i++) {
			env->profiles.profile[n].ptz.preset[i].index = i;
			memset(env->profiles.profile[n].ptz.preset[i].name, 0, sizeof(env->profiles.profile[n].ptz.name));
			sprintf(env->profiles.profile[n].ptz.preset[i].token, "PresetToken%d", i + 1);
			env->profiles.profile[n].ptz.preset[i].in_use = AK_FALSE;
		}
		env->profiles.profile[n].ptz.default_pan_speed = 0.5;
		env->profiles.profile[n].ptz.default_tilt_speed = 0.5;
		env->profiles.profile[n].ptz.default_zoom_speed = 0.5;
		env->profiles.profile[n].ptz.tour_nr = 0;
		for ( i  = 0; i < NVP_MAX_PTZ_TOUR; i++) {
			env->profiles.profile[n].ptz.tour[i].index = i;
			memset(env->profiles.profile[n].ptz.tour[i].name, 0, sizeof(env->profiles.profile[n].ptz.tour[i].name));
			sprintf(env->profiles.profile[n].ptz.tour[i].token, "TourToken%d", i + 1);
		}
	}

	
	ONVIF_SET_NT(env->ether.name, "eth0");
	ONVIF_SET_NT(env->ether.token, "Eth0Token");

	T_IMG_SET_VALUE *pImgset = NULL;

	pImgset = ak_n1_get_video_image_setting();

	if (NULL != pImgset)
	{
		g_img_val.hue = pImgset->hue;
		g_img_val.brightness = pImgset->brightness;
		g_img_val.saturation = pImgset->saturation;
		g_img_val.contrast = pImgset->contrast;
		g_img_val.sharp = pImgset->sharp;
	}
	else
	{
		g_img_val.hue = IMG_EFFECT_DEF_VAL;
		g_img_val.brightness = IMG_EFFECT_DEF_VAL;
		g_img_val.saturation = IMG_EFFECT_DEF_VAL;
		g_img_val.contrast = IMG_EFFECT_DEF_VAL;
		g_img_val.sharp = IMG_EFFECT_DEF_VAL;
	}

	printf("init hue:%lu, brightness:%lu, saturation:%lu, contrast:%lu\n", g_img_val.hue, g_img_val.brightness, g_img_val.saturation, g_img_val.contrast);

	Isp_Effect_Set(EFFECT_HUE, g_img_val.hue - IMG_EFFECT_DEF_VAL);
	Isp_Effect_Set(EFFECT_BRIGHTNESS, g_img_val.brightness - IMG_EFFECT_DEF_VAL);
	Isp_Effect_Set(EFFECT_SATURATION, g_img_val.saturation - IMG_EFFECT_DEF_VAL);
	Isp_Effect_Set(EFFECT_CONTRAST, g_img_val.contrast - IMG_EFFECT_DEF_VAL);

	ak_n1_get_video_setting(&g_video_set);

}


int NVP_env_load(lpNVP_ENV env, const char *module, int keyid)
{
	char temp[512];
	char *ptr = NULL, *pbuf;
	char *saveptr = NULL;
	int ret;
	int chn, id;
	static int onvif_init=0;

	if (onvif_init == 0)
		_nvp_env_init(env);
	
	onvif_init = 1;

	chn = keyid/100;
	id = keyid%100;
	strncpy(temp, module, 512);
	pbuf = temp;

	//anyka_print("NVP_env_load: %s\n", module);
	
	while((ptr = strtok_r(pbuf, OM_AND, &saveptr)) != NULL)
	{		
		if (OM_MATCH(ptr, OM_ALL)) {
			ret = _get_all(env);
			break;
		}else if (OM_MATCH(ptr, OM_PROFILE)) {
			ret = _get_profile(&env->profiles.profile[chn]);
		}else if (OM_MATCH(ptr, OM_PROFILES)) {
			ret = _get_profiles(&env->profiles);
		}else if (OM_MATCH(ptr, OM_INFO)) {
			ret = _get_system_information(&env->devinfo); 
		}else if (OM_MATCH(ptr, OM_DTIME)) {
			ret = _get_date_time(&env->systime);
		}else if (OM_MATCH(ptr, OM_NET)) {
			ret = _get_interface(&env->ether);
		}  else if (OM_MATCH(ptr, OM_VENC)) {
			ret = _get_video_encode(&env->profiles.profile[chn].venc[id], id);
		}  else if (OM_MATCH(ptr, OM_VSRC)) {
			ret = _get_video_source(&env->profiles.profile[chn].v_source, id);
		}  else if (OM_MATCH(ptr, OM_VINC)) {
			ret = _get_video_input_conf(&env->profiles.profile[chn].vin[id], id);
		}  else if (OM_MATCH(ptr, OM_AENC)) {
			ret = _get_audio_encode(&env->profiles.profile[chn].aenc[id], id);
		}  else if (OM_MATCH(ptr, OM_AIN)) {
			ret = _get_audio_input(&env->profiles.profile[chn].ain, id);
		}  else if (OM_MATCH(ptr, OM_COLOR)) {
			ret = _get_color(&env->profiles.profile[chn].v_source.image.color, id);			
		}  else if (OM_MATCH(ptr, OM_IMG)) {
			ret = _get_image(&env->profiles.profile[chn].v_source.image, id);
		}  else if (OM_MATCH(ptr, OM_MD)) {
			ret = _get_motion_detection(&env->profiles.profile[chn].md, id);
		}  else if (OM_MATCH(ptr, OM_PTZ)) {
			ret = _get_ptz(&env->profiles.profile[chn].ptz, id);
		} else {
			anyka_print("unknown env module: %s\n", ptr);
		}
		pbuf = NULL;
	}
	
	return 0;
}

int NVP_env_save(lpNVP_ENV env, const char *module, int keyid)
{
	char temp[512];
	char *ptr = NULL, *pbuf;
	char *saveptr = NULL;
	int ret;
	int f_ret = 0;
	int chn, id;
	
	chn = keyid / 100;
	id = keyid % 100;
	strncpy(temp, module, 512);
	pbuf = temp;

	anyka_print("NVP_env_save: %s\n", module);
	
	while((ptr = strtok_r(pbuf, OM_AND, &saveptr)) != NULL)
	{
		if (OM_MATCH(ptr, OM_ALL)) {
			ret = _set_all(env);
			break;
		}else if (OM_MATCH(ptr, OM_PROFILE)) {
			ret = _set_profile(&env->profiles.profile[chn]);
		}else if (OM_MATCH(ptr, OM_PROFILES)) {
			ret = _set_profiles(&env->profiles);
		}else if (OM_MATCH(ptr, OM_INFO)) {
			//
		}else if (OM_MATCH(ptr, OM_DTIME)) {
			ret = _set_date_time(&env->systime);
		}else if (OM_MATCH(ptr, OM_NET)) {
			ret = _set_interface(&env->ether);
		}  else if (OM_MATCH(ptr, OM_VENC)) {
			ret = _set_video_encode(&env->profiles.profile[chn].venc[id], id);
		}  else if (OM_MATCH(ptr, OM_VSRC)) {
			ret = _set_video_source(&env->profiles.profile[chn].v_source, id);
		}  else if (OM_MATCH(ptr, OM_VINC)) {
			ret = _set_video_input_conf(&env->profiles.profile[chn].vin[id], id);
		}  else if (OM_MATCH(ptr, OM_AENC)) {
			ret = _set_audio_encode(&env->profiles.profile[chn].aenc[id], id);
		}  else if (OM_MATCH(ptr, OM_AIN)) {
			ret = _set_audio_input(&env->profiles.profile[chn].ain, id);
		}  else if (OM_MATCH(ptr, OM_COLOR)) {
			ret = _set_color(&env->profiles.profile[chn].v_source.image.color, id);			
		}  else if (OM_MATCH(ptr, OM_IMG)) {
			ret = _set_image(&env->profiles.profile[chn].v_source.image, id);
		}  else if (OM_MATCH(ptr, OM_MD)) {
			ret = _set_motion_detection(&env->profiles.profile[chn].md, id);
		}  else if (OM_MATCH(ptr, OM_PTZ)) {
			ret = _set_ptz(&env->profiles.profile[chn].ptz, id);
		} else {
			anyka_print("unknown env module: %s\n", ptr);
			f_ret = -1;
		}
		if (ret < 0)
			f_ret = -1;
		pbuf = NULL;
	}
	return f_ret;
	
}

int NVP_env_cmd(lpNVP_CMD cmd, const char *module, int keyid)
{
	char temp[512];
	char *ptr = NULL, *pbuf;
	char *saveptr = NULL;
	int ret;
	int f_ret = 0;
	int chn, id;

	chn = keyid / 100;
	id = keyid % 100;
	strncpy(temp, module, 512);
	pbuf = temp;

	anyka_print("NVP_env_cmd: %s\n", module);
	
	while((ptr = strtok_r(pbuf, OM_AND, &saveptr)) != NULL)
	{
		if (OM_MATCH(ptr, OM_REBOOT)) {
			_cmd_system_boot(0, 0);
		}else if (OM_MATCH(ptr, OM_SYS_RESET)) {
			anyka_print("unknown env module: %s\n", ptr);
		}else if (OM_MATCH(ptr, OM_PTZ)) {
			ret = _cmd_ptz(cmd, module, keyid);
		} else {
			anyka_print("unknown env module: %s\n", ptr);
			f_ret = -1;
		}
		if (ret < 0)
			f_ret = -1;
		pbuf = NULL;
	}
	
	return f_ret;

}
