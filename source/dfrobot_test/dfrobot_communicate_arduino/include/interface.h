/*
** filename: interface.h
** author: jingwenyi
** date: 2016.07.19
*/

#ifndef _INTERFACE_H
#define _INTERFACE_H

#include <stdio.h>
#include <stdint.h>
#include "dfrobot_main.h"

#define R1280X720	0x01
#define R640X480	0x02
#define R320X240	0x03



/*
*arduino to dfrobot
*/
#define CMD_START_ADDR			0X00
#define CMD_START_MODE			(CMD_START_ADDR + 1)
#define CMD_STOP_MODE			(CMD_START_ADDR + 2)

//set cmd
#define CMD_SET_ADDR			0X20
#define CMD_SET_SSID			(CMD_SET_ADDR + 0)
#define CMD_SET_PASSWORD		(CMD_SET_ADDR + 1)
#define CMD_SET_RESOLUTION		(CMD_SET_ADDR + 2)
#define CMD_SET_VOICE			(CMD_SET_ADDR + 3)
#define CMD_SET_NAME			(CMD_SET_ADDR + 4)
#define CMD_SET_FRAME			(CMD_SET_ADDR + 5)

//get cmd
#define	CMD_GET_ADDR			0X30
//#define CMD_GET_PARAM			(CMD_GET_ADDR + 0)
#define CMD_GET_SSID			(CMD_GET_ADDR + 0)
#define CMD_GET_PASSWORD		(CMD_GET_ADDR + 1)
#define CMD_GET_RESOLUTION		(CMD_GET_ADDR + 2)
#define CMD_GET_FRAME			(CMD_GET_ADDR + 3)
#define CMD_GET_NAME			(CMD_GET_ADDR + 4)
#define CMD_GET_VOICE			(CMD_GET_ADDR + 5)


//other cmd
#define CMD_OTHER_ADDR			0X40
#define	CMD_SAVE_FRAME			(CMD_OTHER_ADDR + 0)

//heartbeat packet
#define CMD_HEARTBEAT_PACKET	0X7E
	



/*
* dfrobot to arduino
*/
#define	CMD_DFROBOT_STATUS		0X50
//handle get cmd
#if 0
#define	HANDLE_GET_CMD_START			0X20
#define CMD_HANDLE_GET_NAME				(HANDLE_GET_CMD_START + 1)
#define CMD_HANDLE_GET_RESOLUTION		(HANDLE_GET_CMD_START + 2)
#define CMD_HANDLE_GET_FRAME			(HANDLE_GET_CMD_START + 3)
#define CMD_HANDLE_GET_VOICE			(HANDLE_GET_CMD_START + 4)
#define CMD_HANDLE_GET_SSID				(HANDLE_GET_CMD_START + 5)
#define CMD_HANDLE_GET_PASSWORD			(HANDLE_GET_CMD_START + 6)
#endif

#define	HANDLE_GET_CMD_START			(0x80 + 0x30)
#define CMD_HANDLE_GET_SSID				(HANDLE_GET_CMD_START + 0)
#define CMD_HANDLE_GET_PASSWORD			(HANDLE_GET_CMD_START + 1)
#define CMD_HANDLE_GET_RESOLUTION		(HANDLE_GET_CMD_START + 2)
#define CMD_HANDLE_GET_FRAME			(HANDLE_GET_CMD_START + 3)
#define CMD_HANDLE_GET_NAME				(HANDLE_GET_CMD_START + 4)
#define CMD_HANDLE_GET_VOICE			(HANDLE_GET_CMD_START + 5)


#define RESPOND_HRARTBREAT_PACKET 		(HANDLE_GET_CMD_START + 0X7E)	







typedef enum 
{
	ERR_START_ALL_OK,  //0
	ERR_VIDEO_NO_RUNNING, // 1
	ERR_START_VIDEO_OK,   
	ERR_START_VIDEO_FAILED,
	ERR_STOP_VIDEO_OK,
	ERR_AUDIO_NO_RUNNGING,  //5
	ERR_START_AUDIO_OK,   
	ERR_START_AUDIO_FAILED,
	ERR_STOP_AUDIO_OK,
	ERR_TULING_NO_RUNNING,  //9
	ERR_START_TULING_OK,  
	ERR_START_TULING_FAILED,
	ERR_STOP_TULING_OK,
	ERR_TALK2TALK_NO_RUNNING,  // 13
	ERR_START_TALK2TALK_OK,  
	ERR_START_TALK2TALK_FAILED,
	ERR_STOP_TALK2TALK_OK,
	ERR_APLAY_NO_RUNNING, //17
	ERR_START_APLAY_OK,  
	ERR_START_APLAY_FAILED,
	ERR_STOP_APLAY_OK,
	ERR_WIFI_NO_RUNNING, //21
	ERR_WIFI_START_OK,
	ERR_WIFI_START_FAILED,
	ERR_WIFI_STOP_OK,
	ERR_RESOLUTION_SET_OK,//25
	ERR_RESOLUTION_SET_FAILED,
	ERR_FRAME_SET_OK,//27
	ERR_FRAME_SET_FAILED,
	ERR_VOICE_SET_OK,//29
	ERR_VOICE_SET_FAILED,
	ERR_TAKE_PHOTO_OK,//31
	ERR_TAKE_PHOTO_FAILED,
	ERR_INIT_CONFIG_FAILED,//33
	ERR_SD_NO_EXIST,//34
	ERR_NAME_SET_OK,//35
	ERR_NAME_SET_FAILED,
	ERR_SSID_SET_OK,//37
	ERR_SSID_SET_FAILED,
	ERR_PASSWORD_SET_OK,//39
	ERR_PASSWORD_SET_FAILED,
	ERR_MAX
}dfrobot_stat;




struct mode_bit
{
	unsigned char video_mode_running:1;  
	unsigned char audio_mode_running:1;
	unsigned char tuling_mode_running:1;
	unsigned char talk2talk_mode_running:1;
	unsigned char aplay_mode_running:1;
	unsigned char save_bit:3;
};


struct err_stat
{
    uint8_t cmd;
	unsigned char len;
    void*    data;
};


class Interface
{

public:
	Interface();
	~Interface();
	
	//video mode func
	void start_video_mode();
	void stop_video_mode();
	
	//audio mode func
	void start_audio_mode();
	void stop_audio_mode();

	//talk to talk
	void start_talk2talk_mode();
	void stop_talk2talk_mode();


	//tuling mode func
	void start_tuling_mode();
	void stop_tuling_mode();

	//aplay audio
	void start_aplay_mode();
	void stop_aplay_mode();


	void start_all_mode();
	void stop_all_mode();

	void start_wifi();
	void restart();
	void restartWifi();
	void stop_wifi();

	//set cmd
	void arduino_set_ssid(const char *ssid);
	void arduino_set_password(const char *passwrd);
	void arduino_set_resolution(unsigned char res);
	void arduino_set_voice(unsigned char voice);
	void arduino_set_name(const char *name);
	void arduino_set_frame(unsigned char frame);

	//get cmd
	void arduino_get_ssid();
	void arduino_get_password();
	void arduino_get_resolution();
	void arduino_get_frame();
	void arduino_get_name();
	void arduino_get_voice();

	//other cmd
	void arduino_save_photo();
	void arduino_heartbeat_packet();
	
	
	
	


	bool get_video_mode_running_status()
	{
		return ((mode_status.video_mode_running == 1) ?  true : false);
	}
	bool get_audio_mode_running_status()
	{
		return ((mode_status.audio_mode_running == 1) ?  true : false);
	}

	bool get_tuling_mode_running_status()
	{
		return ((mode_status.tuling_mode_running == 1) ?  true : false);
	}

	bool get_talk2talk_mode_running_status()
	{
		return ((mode_status.talk2talk_mode_running == 1) ?  true : false);
	}

	bool get_aplay_mode_running_status()
	{
		return ((mode_status.aplay_mode_running == 1) ?  true : false);
	}

	void set_video_mode_running_status( bool status)
	{
		mode_status.video_mode_running = (status == true) ? 1:0;
	}

	void set_audio_mode_running_status(bool status)
	{
		mode_status.audio_mode_running = (status == true) ? 1:0;
	}

	void set_tuling_mode_running_status(bool status)
	{
		mode_status.tuling_mode_running = (status == true) ? 1:0;
	}

	void set_talk2talk_mode_running_status(bool status)
	{
		mode_status.talk2talk_mode_running = (status == true) ? 1:0;
	}

	void set_aplay_mode_running_status(bool status)
	{
		mode_status.aplay_mode_running = (status == true) ? 1:0;
	}

	
	
	

private:
	struct mode_bit mode_status;
	

	
};


#endif //_INTERFACE_H

