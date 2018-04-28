/*************************************************

ak_onvif.c
used by onvif

**************************************************/


#include "common.h"
#include "anyka_config.h"
#include "dvs.h"
#include "ak_onvif.h"

int onvif_start_flage; 

/**
 * NAME        IniReadString
 * @BRIEF	read config ini file string
                  
 * @PARAM	const char *section, section name
 			const char *indent,	indent name 
	       	const char *defaultresult,  default result when not found the string
	       	char *result, pointer to result 
	       	int resultlen, lengh of result
	       	char *inifilename, INI file name 
 * @RETURN	int
 * @RETVAL	if return 0 success, otherwise failed
 */

int IniReadString (const char *section, const char *indent,
	       const char *defaultresult, char *result,
	       int resultlen, char *inifilename)
{
  FILE *fp;
  struct stringlist *head, *node, *p, *t;
  char tempstr[MAXSTRLEN];
  char sectionstr[MAXSTRLEN];
  int i, n, len;
  int sectionfinded;
  bzero (sectionstr, MAXSTRLEN);
  sprintf (sectionstr, "[%s]", section);
  fp = fopen (inifilename, "rt");
  if (fp == NULL)
    {
      strcpy (result, defaultresult);
      return 0;
    }
  head = (struct stringlist *) malloc (sizeof (struct stringlist));
  p = head;
  bzero (tempstr, MAXSTRLEN);
  while (fgets (tempstr, MAXSTRLEN, fp) != NULL)
    {
      node = (struct stringlist *) malloc (sizeof (struct stringlist));
      node->next = NULL;
      bzero (node->string, MAXSTRLEN);
      strcpy (node->string, tempstr);
      p->next = node;
      p = p->next;
      bzero (tempstr, MAXSTRLEN);
    }
  fclose (fp);
  p = head;
  while (p->next != NULL)
    {
      t = p->next;
      len = strlen (t->string);
      bzero (tempstr, MAXSTRLEN);
      n = 0;
      for (i = 0; i < len; i++)
	{
	  if ((t->string[i] != '\r') && (t->string[i] != '\n')
	      && (t->string[i] != ' '))
	    {
	      tempstr[n] = t->string[i];
	      n++;
	    }
	}
      if (strlen (tempstr) == 0)
	{
	  p->next = t->next;
	  free (t);
	}
      else
	{
	  bzero (t->string, MAXSTRLEN);
	  strcpy (t->string, tempstr);
	  p = p->next;
	}
    }
  p = head;
  sectionfinded = 0;
  while (p->next != NULL)
    {
      p = p->next;
      if (sectionfinded == 0)
	{
	  if (strcmp (p->string, sectionstr) == 0)
	    sectionfinded = 1;
	}
      else
	{
	  if (p->string[0] == '[')
	    {
	      strcpy (result, defaultresult);
	      goto end;
	    }
	  else
	    {
	      if (strncmp (p->string, indent, strlen (indent)) == 0)
		{
		  if (p->string[strlen (indent)] == '=')
		    {
		      strncpy (result, p->string + strlen (indent) + 1,
			       resultlen);
		      goto end;
		    }
		}
	    }
	}
    }
  strcpy (result, defaultresult);
end:while (head->next != NULL)
    {
      t = head->next;
      head->next = t->next;
      free (t);
    }
  free (head);
  return 0;
}



/**
 * NAME        IniReadInteger
 * @BRIEF	read config ini file interger
                  
 * @PARAM	const char *section, section name
 			const char *indent,	indent name 
	       	const char *defaultresult,  default result when not found the string
	       	char *inifilename, INI file name 
 * @RETURN	int
 * @RETVAL	if return 0 success, otherwise failed
 */

int IniReadInteger (const char *section, const char *indent, int defaultresult,
		char *inifilename)
{
  int i, len;
  char str[1024];
  IniReadString (section, indent, "NULL", str, 1024, inifilename);
  if (strcmp (str, "NULL") == 0)
    {
      return defaultresult;
    }
  len = strlen (str);
  if (len == 0)
    {
      return defaultresult;
    }
  for (i = 0; i < len; i++)
    {
      if ((*(str + i) < '0') || (*(str + i) > '9'))
	{
	  return defaultresult;
	}
    }
  return atoi (str);
}

/**
 * NAME        IniWriteString
 * @BRIEF	write config ini file string
                  
 * @PARAM	const char *section, section name
 			const char *indent,	indent name 
	       	const char *value, value which you want to write
	       	char *inifilename, INI file name 
 * @RETURN	int
 * @RETVAL	if return 0 success, otherwise failed
 */

int IniWriteString (const char *section, const char *indent, const char *value,
		char *inifilename)
{
  FILE *fp;
  struct stringlist *head, *node, *p, *t;
  char tempstr[MAXSTRLEN];
  char sectionstr[MAXSTRLEN];
  int i, n, len;
  int sectionfinded;
  bzero (sectionstr, MAXSTRLEN);
  sprintf (sectionstr, "[%s]", section);
  fp = fopen (inifilename, "rt");
  if (fp == NULL)
    {
      fp = fopen (inifilename, "wt");
      fputs (sectionstr, fp);
      fputs ("\r\n", fp);
      bzero (tempstr, MAXSTRLEN);
      sprintf (tempstr, "%s=%s", indent, value);
      fputs (tempstr, fp);
      fputs ("\r\n", fp);
      fclose (fp);
      return 0;
    }
  head = (struct stringlist *) malloc (sizeof (struct stringlist));
  p = head;
  while (!feof (fp))
    {
      node = (struct stringlist *) malloc (sizeof (struct stringlist));
      node->next = NULL;
      bzero (node->string, MAXSTRLEN);
      fgets (node->string, MAXSTRLEN, fp);
      p->next = node;
      p = p->next;
    }
  fclose (fp);
  p = head;
  while (p->next != NULL)
    {
      t = p->next;
      len = strlen (t->string);
      bzero (tempstr, MAXSTRLEN);
      n = 0;
      for (i = 0; i < len; i++)
	{
	  if ((t->string[i] != '\r') && (t->string[i] != '\n')
	      && (t->string[i] != ' '))
	    {
	      tempstr[n] = t->string[i];
	      n++;
	    }
	}
      if (strlen (tempstr) == 0)
	{
	  p->next = t->next;
	  free (t);
	}
      else
	{
	  bzero (t->string, MAXSTRLEN);
	  strcpy (t->string, tempstr);
	  p = p->next;
	}
    }
  p = head;
  sectionfinded = 0;
  while (p->next != NULL)
    {
      t = p;
      p = p->next;
      if (sectionfinded == 0)
	{
	  if (strcmp (p->string, sectionstr) == 0)
	    {
	      sectionfinded = 1;
	      node =
		(struct stringlist *) malloc (sizeof (struct stringlist));
	      bzero (node->string, MAXSTRLEN);
	      sprintf (node->string, "%s=%s", indent, value);
	      node->next = p->next;
	      p->next = node;
	      p = p->next;
	    }
	}
      else
	{
	  if (p->string[0] == '[')
	    {
	      goto end;
	    }
	  else
	    {
	      if (strncmp (p->string, indent, strlen (indent)) == 0)
		{
		  if (p->string[strlen (indent)] == '=')
		    {
		      node = p;
		      t->next = p->next;
		      free (node);
		      goto end;
		    }
		}
	    }
	}
    }
end:
  fp = fopen (inifilename, "wt");
  p = head;
  while (p->next != NULL)
    {
      p = p->next;
      fputs (p->string, fp);
      fputs ("\r\n", fp);
    }
  if (sectionfinded == 0)
    {
      fputs (sectionstr, fp);
      fputs ("\r\n", fp);
      bzero (tempstr, MAXSTRLEN);
      sprintf (tempstr, "%s=%s", indent, value);
      fputs (tempstr, fp);
      fputs ("\r\n", fp);
    }
  fclose (fp);
  while (head->next != NULL)
    {
      t = head->next;
      head->next = t->next;
      free (t);
    }
  free (head);
  return 0;
}

/**
 * NAME        IniWriteInteger
 * @BRIEF	write config ini file interger
                  
 * @PARAM	const char *section, section name
 			const char *indent,	indent name 
	       	const char *value, value which you want to write
	       	char *inifilename, INI file name 
 * @RETURN	int
 * @RETVAL	if return 0 success, otherwise failed
 */

int IniWriteInteger (const char *section, const char *indent, int value,
		 char *inifilename)
{
  char str[20];
  memset (str, 0, 20);
  sprintf (str, "%d", value);
  IniWriteString (section, indent, str, inifilename);
  return 0;
}

unsigned int get_dhcp_status(void)
{
	char buf[10] = {0};
	
	int fd = open("/tmp/dhcp_status", O_RDONLY);
	if(fd < 0) {
		perror("open");
		return 0;
	}
	read(fd, buf, 1);

	close(fd);
	printf("[%s:%d] dhcp: %d\n", __func__, __LINE__, atoi(buf));

	return atoi(buf);
}

/**
 * NAME        GetNet
 * @BRIEF	get net data
                  
 * @PARAM	struct  netinfo_t *resp, strore info
 * @RETURN	int
 * @RETVAL	if return 0 success, otherwise failed
 */

int GetNet(struct  netinfo_t *resp)
{    
    char ip[64]={0};
    int i=0;
    
    //ifconfig eth0
    do_syscmd("ifconfig eth0 | grep 'inet addr' | awk '{print $2}'", ip);
    while(ip[i])
    {
        if (ip[i]=='\n' || ip[i]=='\r' || ip[i]==' ')
            ip[i] = '\0';
        i++;
    }
    
    Psystem_net_set_info sys_net_info = anyka_get_sys_net_setting();

	unsigned int dhcp = get_dhcp_status();
	resp->IpType = dhcp;
	resp->DnsType = dhcp;

	//resp->IpType = sys_net_info->dhcp;
	//resp->DnsType = sys_net_info->dhcp;
	//sprintf(resp->cIp, "%s", sys_net_info->ipaddr);
	sprintf(resp->cIp, "%s", &ip[5]);
    sprintf(resp->cNetMask, "%s", sys_net_info->netmask);
	sprintf(resp->cGateway, "%s", sys_net_info->gateway);
	sprintf(resp->cMDns, "%s", sys_net_info->firstdns);
	sprintf(resp->cSDns, "%s", sys_net_info->backdns);
	resp->chttpPort = 80;
	
	return 0;
}

/**
 * NAME        SetNet
 * @BRIEF	set net data
                  
 * @PARAM	struct  netinfo_t *resp, info you want to set
 * @RETURN	int
 * @RETVAL  0
 */

int SetNet(struct  netinfo_t *resp)
{
    Psystem_net_set_info net_info = anyka_get_sys_net_setting();
    
	net_info->dhcp = resp->IpType;
	strcpy(net_info->ipaddr, resp->cIp);
	strcpy(net_info->netmask, resp->cNetMask);
	strcpy(net_info->gateway, resp->cGateway);
	strcpy(net_info->firstdns, resp->cMDns);
	strcpy(net_info->backdns, resp->cSDns);
	anyka_set_sys_net_setting(net_info);
	
	return 0;
}
#if 1

/**
 * NAME        init_network
 * @BRIEF	init net
                  
 * @PARAM	dvs_t * dvs, strore info
 * @RETURN	int
 * @RETVAL	0
 */

int init_network (dvs_t * dvs)
{  
    dvs->video_width = 1280;  
    dvs->video_height = camera_get_ch1_height();  
    dvs->video_width_small = 640;  
    dvs->video_height_small = camera_get_ch2_height();  
    return 0;
}
#endif

/**
 * NAME        hal_set_brightness
 * @BRIEF	set brightness
                  
 * @PARAM	dvs_t *dvs,
 			int index, 
 			int brightness
 * @RETURN	int
 * @RETVAL	0
 */

int hal_set_brightness (dvs_t *dvs, int index, int brightness)
{
	// SetBrightness(brightness / 15);//SetBrightness(0);
	camera_set_brightness(brightness);
	return 0;
}

/**
 * NAME        hal_set_contrast
 * @BRIEF	set contrast
                  
 * @PARAM	dvs_t *dvs,
 			int index, 
 			int brightness
 * @RETURN	int
 * @RETVAL	0
 */

int hal_set_contrast (dvs_t *dvs, int index, int contrast)
{
	// SetGAMMA(contrast / 15);//SetGAMMA(0);
	camera_set_gamma(contrast);
	return 0;
}

/**
 * NAME        hal_set_saturation
 * @BRIEF	set saturation
                  
 * @PARAM	dvs_t *dvs,
 			int index, 
 			int brightness
 * @RETURN	int
 * @RETVAL	0
 */

int hal_set_saturation (dvs_t *dvs, int index, int saturation)
{
	// SetSATURATION(saturation / 15);//SetSATURATION(0);
	camera_set_saturation(saturation);
	return 0;
}

/**
 * NAME        GetVideoPara
 * @BRIEF	get vadie para
                  
 * @PARAM	int *fps,  fps
 			int *bitrate, bitrate
 			int *quality, quality
 			int *flage, flage
			int *width, width
			int *height, height
			int main_small, main or small
 * @RETURN	int
 * @RETVAL	0
 */

int GetVideoPara(int *fps, int *bitrate, int *quality, int *flage, 
	int *width, int *height, int main_small)
{
    Psystem_onvif_set_info onvif = anyka_get_sys_onvif_setting();    
	if(!main_small)
	{
		*fps = onvif->fps1;
		*bitrate = onvif->kbps1;
		//quality && flage saved to profile.ini
		*quality = IniReadInteger ("main", "quality", 2, "/etc/jffs2/profile.ini");
		*flage = IniReadInteger ("main", "flage", 0, "/etc/jffs2/profile.ini");
		*width = IniReadInteger ("main", "width", 1280, "/etc/jffs2/profile.ini");
		*height = IniReadInteger ("main", "height", 960, "/etc/jffs2/profile.ini");
	}
	else
	{
		*fps = onvif->fps2;
		*bitrate = onvif->kbps2;
		//quality && flage saved to profile.ini
		*quality = IniReadInteger ("small", "quality", 2, "/etc/jffs2/profile.ini");
		*flage = IniReadInteger ("small", "flage", 1, "/etc/jffs2/profile.ini");
		*width = IniReadInteger ("main", "width", 640, "/etc/jffs2/profile.ini");
		*height = IniReadInteger ("main", "height", 480, "/etc/jffs2/profile.ini");
	}
	return 0;
}


/**
 * NAME        SetVideoPara
 * @BRIEF	set vadie para
                  
 * @PARAM	int *fps,  fps
 			int *bitrate, bitrate
 			int *quality, quality
 			int *flage, flage
			int *width, width
			int *height, height
			int main_small, main or small
 * @RETURN	int
 * @RETVAL	0
 */

int SetVideoPara(int fps, int bitrate, int quality, 
	int width, int height, int flage )
{
    Psystem_onvif_set_info onvif = anyka_get_sys_onvif_setting();    

	if(flage == 0)
	{
        onvif->kbps1 = bitrate;            
        video_set_encode_bps(FRAMES_ENCODE_RECORD, bitrate);
		onvif->fps1= fps;// fps
        video_set_encode_fps(FRAMES_ENCODE_RECORD, fps);
        //quality && flage saved to profile.ini
		IniWriteInteger ("main", "quality", quality,"/etc/jffs2/profile.ini");
		IniWriteInteger ("main", "flage", flage,"/etc/jffs2/profile.ini");	
		IniWriteInteger ("main", "width", width, "/etc/jffs2/profile.ini");
		IniWriteInteger ("main", "height", height, "/etc/jffs2/profile.ini");
	}
	else
	{
        onvif->kbps2 = bitrate;            
        video_set_encode_bps(FRAMES_ENCODE_VGA_NET, bitrate);
		onvif->fps2= fps;// fps
        video_set_encode_fps(FRAMES_ENCODE_VGA_NET, fps);
        //quality && flage saved to profile.ini
		IniWriteInteger ("small", "quality", quality,"/etc/jffs2/profile.ini");
		IniWriteInteger ("small", "flage", flage,"/etc/jffs2/profile.ini");
		IniWriteInteger ("small", "width", width, "/etc/jffs2/profile.ini");
		IniWriteInteger ("small", "height", height, "/etc/jffs2/profile.ini");
	}
    anyka_set_sys_onvif_setting(onvif); 
    anyka_print("SetVideoPara():fps=%d, bitrate=%d, quality=%d\n", fps, bitrate, quality);
    
	return 0;
}


/**
 * NAME        config_load_videocolor
 * @BRIEF	load video recorder config
                  
 * @PARAM	config_videocolor_t * videocolor, store info
 			int index, intex
 * @RETURN	int
 * @RETVAL	0
 */

int config_load_videocolor (config_videocolor_t * videocolor, int index)
{
  char tmpstr[MAX_NAME_LEN];
  sprintf (tmpstr, "%d", index + 1);
  videocolor->brightness =IniReadInteger (tmpstr, "brightness", 50, "/etc/jffs2/videocolor.ini");
  videocolor->contrast =IniReadInteger (tmpstr, "contrast", 50, "/etc/jffs2/videocolor.ini");
  videocolor->saturation =IniReadInteger (tmpstr, "saturation", 50, "/etc/jffs2/videocolor.ini");
  videocolor->hue = IniReadInteger (tmpstr, "hue", 50, "/etc/jffs2/videocolor.ini");
  return 0;
}

/**
 * NAME        get_onvif_utf8
 * @BRIEF	change word to utf-8 type word
                  
 * @PARAM	char *out,  out buf pointer
 			char *in, input buf pointer
 			
 * @RETURN	void
 * @RETVAL	
 */
void get_onvif_utf8(char *out, char *in)
{
	int i, j;
	    for(i = 0, j = 0; i < strlen(in); )
    {
        if(in[i] == '%')
        {
            i ++;
            char tmp = in[i ++];
            if(tmp >= 'A')
            {
                out[j ++] = tmp - 'A' + 10;
            }
            else
            {
                out[j ++] = tmp - '0';
            }
            if(in[i] != '%')
            {
				tmp = in[i ++];
                out[j - 1] = (out[j - 1] << 4) ; 
                if(tmp >= 'A')
                {
                    tmp = tmp - 'A' + 10;
                }
                else
                {
                    tmp = tmp - '0';
                }
                out[j - 1] |= tmp;
            }
        }
        else
        {
            out[j ++] = in[i++];
        }
    }
    out[j] = 0;
}


/**
 * NAME        onvif_format_gb_to_Jovision__code
 * @BRIEF	change gb code to jovision code
                  
 * @PARAM	unsigned char *GB_code, pointer to GB code
 			int gb_len, gb code len
 			unsigned char *Jovisio_code, out put buf pointer
 			
 * @RETURN	void
 * @RETVAL	
 */

void onvif_format_gb_to_Jovision__code(unsigned char *GB_code, int gb_len,unsigned char *Jovision_code)
{
	int i;
	unsigned char *ptr = Jovision_code;

	for(i = 0; i < gb_len; i++)
	{
		if(*GB_code < 0x80)
		{
			*ptr = *GB_code;
		}
		else if(*GB_code < 0xC0)
		{ 
			*ptr = 0xc2;
			ptr ++;
			*ptr = *GB_code;
		}
		else
		{
			*ptr = 0xc3;
			ptr ++;
			*ptr = *GB_code - 0x40;
		}
		ptr ++;
		GB_code ++;
	}
	*ptr = 0;
}


/**
 * NAME        onvif_format_Jovision_to__gb_code
 * @BRIEF	changejovision code  to gb code
                  
 * @PARAM	unsigned char *GB_code, pointer to GB code
 			unsigned char *Jovisio_code, input buf pointer
 			int Jovision_len, jovision code len
 			
 * @RETURN	void
 * @RETVAL	
 */

void onvif_format_Jovision_to__gb_code(unsigned char *GB_code,unsigned char *Jovision_code, int Jovision_len)
{
	int i;

	for(i = 0; i < Jovision_len; i++)
	{
        if(*Jovision_code == 0xc2)
        {
            Jovision_code ++;
            *GB_code = *Jovision_code;
        }
        else if(*Jovision_code == 0xc3)
        {
            Jovision_code ++;
            *GB_code = *Jovision_code + 0x40;
        }
        else
        {
            *GB_code = *Jovision_code;
        }
        Jovision_code ++;
        GB_code ++;
	}
	*GB_code = 0;
}


/**
 * NAME        GetOnvifData
 * @BRIEF	get onvfi config 
                  
 * @PARAM	onvif_data_t *onvif_data
 			
 * @RETURN	void
 * @RETVAL	
 */

int GetOnvifData(onvif_data_t *onvif_data)
{
    unsigned char gb_code[100];
    Pcamera_disp_setting camera_info;
    
    camera_info = anyka_get_camera_info();
    
    font_unicode_to_gb(camera_info->osd_unicode_name, gb_code);
    onvif_format_gb_to_Jovision__code(gb_code, strlen((char *)gb_code), (unsigned char *)onvif_data->identification_name);
//	IniReadString ("onvif_data", "if_name", "H264_IPCAM", onvif_data->identification_name, 256,"/etc/jffs2/onvif_data.ini");
	IniReadString ("onvif_data", "if_location", "shenzhen", onvif_data->identification_location, 256, "/etc/jffs2/onvif_data.ini");

	return 0;
}

/**
 * NAME        SetOnvifData
 * @BRIEF	set onvfi config 
                  
 * @PARAM	onvif_data_t *onvif_data
 			
 * @RETURN	void
 * @RETVAL	
 */

int SetOnvifData(onvif_data_t *onvif_data)
{
    unsigned char gb_code[100];
    unsigned short un_code[100];
    int un_len;
    
    Pcamera_disp_setting camera_info;
    camera_info = anyka_get_camera_info();
    //get_onvif_utf8(camera_info->osd_name, onvif_data->identification_name);
    onvif_format_Jovision_to__gb_code(gb_code, (unsigned char *)onvif_data->identification_name, strlen(onvif_data->identification_name));
    un_len = font_gb_to_unicode(gb_code, un_code);
    font_unicode_to_utf8(un_code, un_len, (unsigned char *)camera_info->osd_name, 50);
    anyka_print("camera_info->osd_name osd name:%s\n", camera_info->osd_name);
    anyka_set_camera_info(camera_info);
	//IniWriteString ("onvif_data", "if_name", onvif_data->identification_name, "/etc/jffs2/onvif_data.ini");
	IniWriteString ("onvif_data", "if_location", onvif_data->identification_location, "/etc/jffs2/onvif_data.ini");

	return 0;
}

