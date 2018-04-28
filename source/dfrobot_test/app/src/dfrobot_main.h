/*
** filename: dfrobot_main.h
** author: jingwenyi
** date: 2016.07.19
*/

#include "socket_transfer_client.h"


//video mode func
int main_start_video_mode();
int main_stop_video_mode();

	

//audio mode func
int main_start_audio_mode();
int main_stop_audio_mode();

//talk to talk
int main_start_talk2talk_mode();
int main_stop_talk2talk_mode();




//tuling mode func
int main_start_tuling_mode();
int main_stop_tuling_mode();


//aplay audio
int main_start_aplay_mode();
int main_stop_aplay_mode();


void main_start_all_mode();

void main_stop_all_mode();


void pc_voice_mp3_notify(const char *filename);

void main_start_wifi();
void main_restart();
void main_restartWifi();
void main_stop_wifi();

//set cmd
void main_arduino_set_ssid(const char *ssid);
void main_arduino_set_password(const char *passwrd);
void main_arduino_set_resolution(unsigned char res);
void main_arduino_set_voice(unsigned char voice);
void main_arduino_set_name(const char *name);
void main_arduino_set_frame(unsigned char frame);

//get cmd
void main_arduino_get_ssid();
void main_arduino_get_password();
void main_arduino_get_resolution();
void main_arduino_get_frame();
void main_arduino_get_name();
void main_arduino_get_voice();

//other cmd
void main_arduino_save_photo();
void main_arduino_heartbeat_packet();
	


//pc <-----> smartcamera
unsigned char get_current_resolution();

void set_current_resolution(unsigned char val);
void switch_resolution(int w, int h);

void set_current_voice_size(unsigned char val);
unsigned char get_current_voice_size();
void main_set_config_name(const char *dst);
bool set_tuling_param(TULING_SET_PARAM * param);






