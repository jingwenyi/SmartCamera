/*
** filename: interface.cpp
** author: jingwenyi
** date: 2016.07.19
**
*/


#include <stdio.h>
#include <string.h>
#include "interface.h"

Interface::Interface()
{
	memset(&mode_status,0, sizeof(&mode_status));

}
Interface::~Interface()
{
	
}


//video mode func
void Interface::start_video_mode()
{
	main_start_video_mode();
}

void Interface::stop_video_mode()
{
	main_stop_video_mode();
}


	

	//audio mode func
void Interface::start_audio_mode()
{
	main_start_audio_mode();
}

void Interface::stop_audio_mode()
{
	main_stop_audio_mode();
}


//talk to talk
void Interface::start_talk2talk_mode()
{
	main_start_talk2talk_mode();
}
void Interface::stop_talk2talk_mode()
{
	main_stop_talk2talk_mode();
}




//tuling mode func
void Interface::start_tuling_mode()
{
	main_start_tuling_mode();
}
void Interface::stop_tuling_mode()
{
	main_stop_tuling_mode();
}

//aplay audio
void Interface::start_aplay_mode()
{
	main_start_aplay_mode();
}
void Interface::stop_aplay_mode()
{
	main_stop_aplay_mode();
}



void Interface::start_all_mode()
{
	main_start_all_mode();
}
void Interface::stop_all_mode()
{
	main_stop_all_mode();
}

void Interface::start_wifi()
{
	main_start_wifi();
}
void Interface::restart()
{
	main_restart();
}
void Interface::restartWifi()
{
	main_restartWifi();
}
void Interface::stop_wifi()
{
	main_stop_wifi();
}


//set cmd
void Interface::arduino_set_ssid(const char *ssid)
{
	main_arduino_set_ssid(ssid);
}
void Interface::arduino_set_password(const char *passwrd)
{
	main_arduino_set_password(passwrd);
}
void Interface::arduino_set_resolution(unsigned char res)
{
	main_arduino_set_resolution(res);
}
void Interface::arduino_set_voice(unsigned char voice)
{
	main_arduino_set_voice(voice);
}
void Interface::arduino_set_name(const char *name)
{
	main_arduino_set_name(name);
}
void Interface::arduino_set_frame(unsigned char frame)
{
	main_arduino_set_frame(frame);
}

//get cmd
void Interface::arduino_get_ssid()
{
	main_arduino_get_ssid();
}
void Interface::arduino_get_password()
{
	main_arduino_get_password();
}
void Interface::arduino_get_resolution()
{
	main_arduino_get_resolution();
}
void Interface::arduino_get_frame()
{
	main_arduino_get_frame();
}
void Interface::arduino_get_name()
{
	main_arduino_get_name();
}
void Interface::arduino_get_voice()
{
	main_arduino_get_voice();
}

//other cmd
void Interface::arduino_save_photo()
{
	main_arduino_save_photo();
}
void Interface::arduino_heartbeat_packet()
{
	main_arduino_heartbeat_packet();
}





