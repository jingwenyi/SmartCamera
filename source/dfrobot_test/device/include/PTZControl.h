
/**
* @brief  set record led 
* 
* @author dengzhou
* @date 2013-04-07
* @param[in] 
* @return T_S32
* @retval 
*/

#ifndef _PTZ_CONTROL_H_
#define _PTZ_CONTROL_H_
#ifdef __cplusplus
extern "C" {
#endif

enum ak_motor_phase {
	AK_MOTOR_PHASE_A = 0,
	AK_MOTOR_PHASE_B,
	AK_MOTOR_PHASE_C,
	AK_MOTOR_PHASE_D,
	AK_MOTOR_PHASE_NUM,
};

enum ak_motor_hit {
	AK_MOTOR_HIT_LEFT = 0,
	AK_MOTOR_HIT_RIGHT,
	AK_MOTOR_HIT_NUM,
};


#define AK_MOTOR_IOC_MAGIC 		'm'
#define AK_MOTOR_SET_ANG_SPEED 		_IOW(AK_MOTOR_IOC_MAGIC, 11, int)
#define AK_MOTOR_GET_ANG_SPEED 		_IOR(AK_MOTOR_IOC_MAGIC, 12, int)
#define AK_MOTOR_TURN_CLKWISE 		_IOW(AK_MOTOR_IOC_MAGIC, 13, int)
#define AK_MOTOR_TURN_ANTICLKWISE 	_IOW(AK_MOTOR_IOC_MAGIC, 14, int)
#define AK_MOTOR_GET_HIT_STATUS 	_IOW(AK_MOTOR_IOC_MAGIC, 15, int)

#define AK_MOTOR_EVENT_HIT 		(1)
#define AK_MOTOR_EVENT_UNHIT 	(1<<1)
#define AK_MOTOR_EVENT_STOP 	(1<<2)

#define AK_MOTOR_HITTING_LEFT 	(1<<0)
#define AK_MOTOR_HITTING_RIGHT 	(1<<1)

struct notify_data
{
	int hit_num;
	int event;
	int remain_steps;
};

struct ptz_pos
{
	int left;
	int up;
};
enum
{
  PTZ_WAIT_INIT,  
  PTZ_INIT_OK,
  PTZ_SYS_WAIT,
};
//PTZControlInit
int PTZControlInit();
//PTZControlUp
int PTZControlUp(int dg);
//PTZControlDown
int PTZControlDown(int dg);
//PTZControlLeft
int PTZControlLeft(int dg);
//PTZControlRight
int PTZControlRight(int dg);
int PTZControlSetPosition();
int PTZControlRunPosition();
//PTZControlDeinit
int PTZControlDeinit();
int PTZControlStatus(int status);

#ifdef __cplusplus
}
#endif
#endif
