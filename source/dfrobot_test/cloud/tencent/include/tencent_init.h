#ifndef _tencent_init_h_
#define _tencent_init_h_
void tencent_init(unsigned int version);
void ak_qq_send_video_move_alarm(int alarm_type, int save_flag, char *save_path, int start_time, int time_len);
void ak_qq_send_voice_to_phone(char *save_path, int start_time);
void ak_qq_unbind();
bool ak_qq_online();

#endif

