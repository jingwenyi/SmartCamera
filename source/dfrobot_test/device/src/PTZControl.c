/**
* @brief  PTZ control
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include "common.h"
#include "PTZControl.h"
#include "anyka_config.h"


#define AK_MOTOR_DEV0 "/dev/ak-motor0"//左右
#define AK_MOTOR_DEV1 "/dev/ak-motor1"//上下

#define CONFIG_PTZ_UNHIT_NAME  "/tmp/ak_sys_start_flag"

#define MOTOR_MAX_BUF 	(64)

typedef enum
{
	MT_STOP,
	MT_STEP,
	MT_RUN,
	MT_RUNTO,
	MT_CAL,
}MT_STATUS;

struct ak_motor
{
	int running;
	
	struct notify_data data;
	int fd;
	MT_STATUS mt_status;
	int cw;
	int step_degree;

	int default_speed;
	int run_speed;
	int runto_speed;
	int hit_speed;
	int step_speed;
	
	int nMax_hit;//撞击角度

	int dg_save;//保存设置位置的角度
	int dg_cur;//偏移角度，理论上的角度
	int steps_cur;	//偏移步数，真实的步数，与偏移角度有对应关系。
};

//角度的范围为[x, 0]，右和下有0
#define MAX_SPEED 100
#define H_MAX_DG 380//最大转动角度，这里定义足够大
#define V_MAX_DG 180//最大转动角度，这里定义足够大
#define INVALID_DG	370
#define DEFAULT_LEFT_HIT_DG		340
#define DEFAULT_UP_HIT_DG		92


int PTZInitOK = PTZ_WAIT_INIT;
int PTZControlPositionInit(void);
static int ak_motor_set_had_cal(void);

static struct ptz_pos posnum[6];

static struct ak_motor akmotor[] = 
{
	[0] = {
		.fd = -1,
		.mt_status = MT_STOP,
		.step_degree = 16,//这个值或者它的倍数的误差最小
		.default_speed = MAX_SPEED,//5,
		.run_speed = MAX_SPEED,//5,
		.runto_speed = MAX_SPEED,//15,
		.hit_speed = MAX_SPEED,//15,
		.step_speed = 15,//15,//8,
	},
	[1] = {
		.fd = -1,
		.mt_status = MT_STOP,
		.step_degree = 8,
		.default_speed = MAX_SPEED,//3,
		.run_speed = MAX_SPEED,//3,
		.runto_speed = MAX_SPEED,//15,
		.hit_speed = MAX_SPEED,//15,
		.step_speed = 15,//15,//8,
	},
};

/**
* @brief  motor revise param
* 
* @author dengzhou
* @date 2013-04-07
* @param[motor] 
* @return void
* @retval 
*/
static void ak_motor_revise_param(struct ak_motor *motor)
{
	#define MIN_H_HIT_ANGLE	(320)
	#define MIN_V_HIT_ANGLE	(90)
	#define MAX_H_HIT_ANGLE (360)
	#define MAX_V_HIT_ANGLE (110)

	int def_angle;
	int min_angle;
	int max_angle;

	if (motor == &akmotor[0])
	{
		def_angle = DEFAULT_LEFT_HIT_DG;
		min_angle = MIN_H_HIT_ANGLE;
		max_angle = MAX_H_HIT_ANGLE;
	}
	else
	{
		def_angle = DEFAULT_UP_HIT_DG;
		min_angle = MIN_V_HIT_ANGLE;
		max_angle = MAX_V_HIT_ANGLE;
	}

	if ((motor->nMax_hit < min_angle) ||
		(motor->nMax_hit > max_angle))
	{
		motor->nMax_hit = def_angle;
	}
}

static int motor_turn(struct ak_motor *motor, int steps, int is_cw)
{
	if(motor->fd < 0)
		return -1;

	int ret;
	int cmd = is_cw ? AK_MOTOR_TURN_CLKWISE:AK_MOTOR_TURN_ANTICLKWISE;

	ret = ioctl(motor->fd, cmd, &steps);	
	if(ret) {
		anyka_print("%s fail. is_cw:%d, steps:%d, ret:%d.\n", __func__, steps, is_cw, ret);
	}
	
	return ret;
}

static int angle2step(int dg)
{
	int steps;

	steps = (dg * 64 * 64) / (360 * 2);

	return steps;
}

static int step2angle(int steps)
{
	int dg;

	dg = (steps * 360 * 2) / (64 * 64);

	return dg;
}

static int calc_turn_steps(struct ak_motor *motor, int *orig_angle, int is_cw)
{
	int steps;
	int angle = *orig_angle;
	int old_angle = *orig_angle;
	int offset_angle;
	int offset_steps;
	
	if (is_cw) {
		if (angle + motor->dg_cur > motor->nMax_hit)
			angle = motor->nMax_hit - motor->dg_cur;
	} else {
		if (motor->dg_cur - angle < 0)
			angle = motor->dg_cur;
	}

	if (angle < 0) {
		angle = 0;
		anyka_print("[%s] turn angle < 0. [%s] motor, dg_cur:%d, steps_cur:%d, nMax_hit:%d\n",
				__func__, (motor == &akmotor[0]) ? "LEFT":"UP", motor->dg_cur, motor->steps_cur,
				motor->nMax_hit);
	}

	if (!angle)
		return 0;

	*orig_angle = angle;

	if (is_cw) {
		offset_angle = motor->dg_cur + angle;
		offset_steps = angle2step(offset_angle);
		steps = offset_steps - motor->steps_cur;
	} else {
		offset_angle = motor->dg_cur - angle;
		offset_steps = angle2step(offset_angle);
		steps = motor->steps_cur - offset_steps;
	}

	anyka_print("[%s] [%s] motor, dg_cur:%d, steps_cur:%d, nMax_hit:%d, steps:%d, old_angle:%d, res_angle:%d\n",
			__func__, (motor == &akmotor[0]) ? "LEFT":"UP", motor->dg_cur, motor->steps_cur,
			motor->nMax_hit, steps, old_angle, *orig_angle);
	return steps;
}

static void set_turn_position_info(struct ak_motor *motor, int is_cw, int angle, int steps)
{
	if (is_cw) {
		motor->dg_cur += angle;
		motor->steps_cur += steps;
	} else {
		motor->dg_cur -= angle;
		motor->steps_cur -= steps;
	}
}

/**
* @brief  motor turn
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/
static int motor_turn_dg(struct ak_motor *motor, int angle, int is_cw)
{
	if(motor->fd < 0)
		return -1;

	int res_angle = angle;
	int steps;
	int ret;

	steps = calc_turn_steps(motor, &res_angle, is_cw);
	if (steps <= 0) {
		anyka_print("[%s] turn steps:%d\n", __func__, steps);
		return -1;
	}

	//anyka_print("[%s] turn angle:%d, res_angle:%d, steps:%d\n", __func__, angle, res_angle, steps);
	ret = motor_turn(motor, steps, is_cw);
	if(ret) {
		anyka_print("%s fail. is_cw:%d, angle:%d, ret:%d.\n", __func__, angle, is_cw, ret);
	} else {
		set_turn_position_info(motor, is_cw, res_angle, steps);
	}
	return ret;
}

/**
* @brief  change speed
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/

static int motor_change_speed(struct ak_motor *motor, int ang_speed)
{
	int ret;	
	int val = ang_speed;
	if(motor->fd < 0)
		return -1;
	ret = ioctl(motor->fd, AK_MOTOR_SET_ANG_SPEED, &val);	
	if(ret) {
		anyka_print("%s fail.  ang_speed:%d, ret:%d.\n", __func__, ang_speed, ret);
	}
	return ret;
}

static int motor_get_speed(struct ak_motor *motor)
{
	int ret;	
	int val;

	if(motor->fd < 0)
		return -1;
	ret = ioctl(motor->fd, AK_MOTOR_GET_ANG_SPEED, &val);	
	if(ret) {
		anyka_print("%s fail.  ret:%d.\n", __func__, ret);
		val = 16;
	}

	return val;
}

/**
* @brief  check motor turn timeout
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return int
* @retval return 1 timeout
*/
int ak_motor_check_event_timeout(time_t *base, int timeout)
{
	time_t t;

	t = time(0);

	/* 防止系统时间同步和时区同步造成误判 */
	if (t - *base > 1800)
		*base = time(0);

	t -= *base;
	if (t > timeout)
		return 1;

	return 0;
}

/**
* @brief motor_wait_hit
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/

static int ak_motor_wait_hit_stop(struct ak_motor *motor, int *remain_steps, int speed)
{
	fd_set fds;
	int ret;
	struct timeval tv;
	time_t t, t2;
	int steps_per_second = angle2step(speed);
	const int timeout = (*remain_steps) / steps_per_second + 2;
	
	if(motor->fd < 0)
		return -1;

	t = time(0);
start:
	t2 = time(0);
	tv.tv_sec	= timeout - (t2 - t);
	tv.tv_usec	= 0;
	FD_ZERO(&fds);
	FD_SET(motor->fd, &fds);
	ret = select(motor->fd+1, &fds, NULL, NULL, &tv);
	if(ret <= 0) {
		anyka_print("wait, ret:%d,\n", ret);
		if (ret == 0)
			anyka_print("%s select timeout.\n", __func__);
		return -1;
	}
	
	ret = read(motor->fd, (void*)&motor->data, sizeof(struct notify_data));

	if (motor->data.event & AK_MOTOR_EVENT_HIT)
	{
		*remain_steps = motor->data.remain_steps;
		return 0;
	}
	else if (motor->data.event & AK_MOTOR_EVENT_STOP)
	{
		*remain_steps = motor->data.remain_steps;
		return -1;
	}
	else
	{
		anyka_print("not hit,motor->data.event = %d, %d\n", motor->data.event, motor->data.remain_steps);
		if (ak_motor_check_event_timeout(&t, timeout))
		{
			anyka_print("%s event timeout.\n", __func__);
			return -1;
		}

		goto start;
	}
	
	*remain_steps = motor->data.remain_steps;
	return 0;
}

/**
* @brief motor turn degrees
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/

static int ak_motor_turn(struct ak_motor *motor, int dg)
{
	int rg;
	int speed;
	
	if (dg <= 0)
		dg = motor->step_degree;

	speed = motor_get_speed(motor);

	if (!motor_turn_dg(motor, dg, motor->cw)) {
		rg = angle2step(dg);
		ak_motor_wait_hit_stop(motor, &rg, speed);
		PTZControlSetPosition(1);
	} else {
		return -1;
	}

	return 0;
}

static void* ak_motor_cal_thread(void* data)
{	
	//巡航，校准
	struct ak_motor *motor = data;	
	int rg;
	int speed = motor->hit_speed;
	int max_dg = (motor == &akmotor[0]) ? H_MAX_DG : V_MAX_DG;
	int steps;

	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());

	//motor = &akmotor[i];
	if(motor->mt_status != MT_STOP)
		goto end;
	if(motor->fd < 0)
		goto end;
	
	motor_change_speed(motor, speed);
	motor->mt_status = MT_CAL;
	
	//顺时针转到底
	steps = angle2step(max_dg);
	motor->cw = 1;
	if (!motor_turn(motor, steps, motor->cw)) {
		rg = steps;
		ak_motor_wait_hit_stop(motor, &rg, speed);
	}

	rg = 0;
	motor->cw = 0;
	//逆时针转到底
	if (!motor_turn(motor, steps, motor->cw)) {
		rg = steps;
		ak_motor_wait_hit_stop(motor, &rg, speed);
	}

	//计算出各种角度

	motor->nMax_hit = step2angle(steps - rg);

	anyka_print("MaxHit=%d, steps:%d\n", motor->nMax_hit, steps - rg);

	ak_motor_revise_param(motor);

	anyka_print("Revised MaxHit=%d\n", motor->nMax_hit);

	// 初始化位置信息
	motor->dg_cur		= 0;
	motor->steps_cur	= 0;

	//顺时针转到中间
	motor->cw = 1;
	rg = motor->nMax_hit / 2;
	if (!motor_turn_dg(motor, rg, motor->cw)) {
		rg = angle2step(rg);
		ak_motor_wait_hit_stop(motor, &rg, speed);
	}

	motor->mt_status = MT_STOP;
	motor_change_speed(motor, motor->step_speed);

end:
	pthread_exit(NULL);
	return NULL;
}


/**
* @brief motor thread
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/

static void* ak_motor_self_test(void* data)
{	
	pthread_t th1, th2;
	anyka_pthread_create(&th1, ak_motor_cal_thread, &akmotor[0], ANYKA_THREAD_MIN_STACK_SIZE, -1);
	anyka_pthread_create(&th2, ak_motor_cal_thread, &akmotor[1], ANYKA_THREAD_MIN_STACK_SIZE, -1);

	pthread_join(th1, NULL);
	pthread_join(th2, NULL);

	anyka_print("cal ok\n");

	PTZControlRunPosition(1);
	anyka_print("run positon 1 ok\n");

	ak_motor_set_had_cal();
    PTZInitOK = PTZ_INIT_OK;
    pthread_exit(NULL);
	return NULL;
}

/**
* @brief  ptz status
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return int
* @retval 
*/
int PTZControlStatus(int status)
{
    if(status != -1)
    {
        PTZInitOK = status;
    }
    return PTZInitOK;
}
/**
* @brief create thread 
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return T_S32
* @retval 
*/

static int ak_motor_cal(struct ak_motor* motor)
{
	pthread_t th;
	anyka_pthread_create(&th, ak_motor_self_test, motor, ANYKA_THREAD_MIN_STACK_SIZE, -1);
	return 0;
}

/**
* @brief  runto postion
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return int
* @retval 
*/
static void* ak_motor_runto_thread(void* data)
{	
	anyka_print("[%s:%d] This thread id : %ld\n", __func__, __LINE__, (long int)gettid());

	struct ak_motor *motor = data;
	if(motor->mt_status != MT_STOP)
		return 0;

	int rg;
	motor->mt_status = MT_RUNTO;
	
	motor_change_speed(motor, motor->runto_speed);
	int dg = motor->dg_save - motor->dg_cur;//计算当前位置和保存位置的角度
	
	if(dg > 0)//判断转动方向
	{
		motor->cw = 1;	
	}
	else if(dg < 0)
	{
		motor->cw = 0;
		dg = -dg;
	}
	anyka_print("cw = %d,angle = %d\n", motor->cw, dg);
	if (!motor_turn_dg(motor, dg, motor->cw)) {
		rg = angle2step(dg);
		ak_motor_wait_hit_stop(motor, &rg, motor->runto_speed);
	}

	motor_change_speed(motor, motor->step_speed);
	motor->mt_status = MT_STOP;
	return NULL;
}

/**
* @brief  runto thread
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return int
* @retval return 0 success, otherwise failed
*/
static int ak_motor_runto(struct ak_motor* motor)
{
	if(motor->fd < 0)
		return -1;
	pthread_t th;
	anyka_pthread_create(&th,  ak_motor_runto_thread, motor, ANYKA_THREAD_MIN_STACK_SIZE, -1);
	return 0;
}

/**
* @brief  motor int
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return int
* @retval return 0 success, otherwise failed
*/
static int ak_motor_init(struct ak_motor *motor, char *name)
{
	if(!motor)
		return -1;

	motor->fd = open(name, O_RDWR);
	if(motor->fd < 0)
		return -2;
	
	return 0;
}

/**
* @brief  check had calulate motor parameters
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return int
* @retval return 1 soft reboot
*/
static int ak_motor_check_had_cal(void)
{
	if (access(CONFIG_PTZ_UNHIT_NAME, R_OK) == 0)
		return 1;

	return 0;
}

/**
* @brief  motor parameters init
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return int
* @retval return 0 success, otherwise failed
*/
static int ak_motor_cal_init(void)
{
	int i;
	struct ptz_pos hit;
	struct ak_motor *motor;

	anyka_get_ptz_unhit_info((void *)&hit);
	if (hit.left > INVALID_DG || hit.up > INVALID_DG)
	{
		hit.left = DEFAULT_LEFT_HIT_DG;
		hit.up = DEFAULT_UP_HIT_DG;
	}

	for(i = 0; i < 2; i++)
	{
		motor = &akmotor[i];

		if (posnum[0].left > INVALID_DG || posnum[0].up > INVALID_DG)
		{
			return -1;
		}
		else
		{
			motor->nMax_hit = (i == 0) ? hit.left : hit.up;
			motor->dg_cur = ((i==0) ? posnum[0].left : posnum[0].up);
			motor->steps_cur = angle2step(motor->dg_cur);
		}
	}
	anyka_print("%s nMax_hit:%d&%d, dg_cur:%d&%d.\n", __func__,
		akmotor[0].nMax_hit, akmotor[1].nMax_hit, akmotor[0].dg_cur, akmotor[1].dg_cur);

	return 0;
}

/**
* @brief  motor parameters save
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return int
* @retval return 0 success, otherwise failed
*/
static int ak_motor_set_had_cal(void)
{
	struct ptz_pos hit;

	anyka_get_ptz_unhit_info((void *)&hit);

	hit.left	= akmotor[0].nMax_hit;
	hit.up	= akmotor[1].nMax_hit;
	anyka_set_ptz_unhit_info((void *)&hit);

	return 0;
}

/**
* @brief  PTZ init
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return int
* @retval return 0 success, otherwise failed
*/
int PTZControlInit()
{
	PTZControlPositionInit();

	if (ak_motor_init(&akmotor[0], AK_MOTOR_DEV0) != 0)
	{
        PTZInitOK = PTZ_INIT_OK;
	    return -1;
	}
	if (ak_motor_init(&akmotor[1], AK_MOTOR_DEV1) != 0)
	{
        PTZInitOK = PTZ_INIT_OK;
	    return -1;
	}

	if (ak_motor_check_had_cal())
	{
		if (ak_motor_cal_init())
			ak_motor_cal(NULL);
        PTZInitOK = PTZ_INIT_OK;
	}
	else
	{
		ak_motor_cal(NULL);
	}

	return 0;
}

/**
* @brief  PTZ turn up one step
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return int
* @retval return 0 success, otherwise failed
*/
int PTZControlUp(int dg)
{
	int ret;
	struct ak_motor* motor = &akmotor[1];
	//anyka_print("[%s] dg:%d\n", __func__, dg);
	if(motor->mt_status != MT_STOP)
		return -1;
	motor->mt_status = MT_STEP;
	motor->cw = 1;
	ret = ak_motor_turn(motor, dg);
	
	motor->mt_status = MT_STOP;
	return ret;
}

/**
* @brief  PTZ turn down one step
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return int
* @retval return 0 success, otherwise failed
*/
int PTZControlDown(int dg)
{
	int ret;
	struct ak_motor* motor = &akmotor[1];
	//anyka_print("[%s] dg:%d\n", __func__, dg);
	if(motor->mt_status != MT_STOP)
		return -1;
	motor->mt_status = MT_STEP;
	motor->cw = 0;
	ret = ak_motor_turn(motor, dg);
	
	motor->mt_status = MT_STOP;
	return ret;
}

/**
* @brief  PTZ turn left one step
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return int
* @retval return 0 success, otherwise failed
*/
int PTZControlLeft(int dg)
{
	int ret;
	struct ak_motor* motor = &akmotor[0];
	//anyka_print("[%s] dg:%d\n", __func__, dg);
	if(motor->mt_status != MT_STOP)
		return -1;
	motor->mt_status = MT_STEP;
	motor->cw = 1;
	ret = ak_motor_turn(motor, dg);
	
	motor->mt_status = MT_STOP;
	return ret;
}

/**
* @brief  PTZ turn right one step
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return int
* @retval return 0 success, otherwise failed
*/
int PTZControlRight(int dg)
{
	int ret;
	struct ak_motor* motor = &akmotor[0];
	//anyka_print("[%s] dg:%d\n", __func__, dg);
	if(motor->mt_status != MT_STOP)
		return -1;
	motor->mt_status = MT_STEP;
	motor->cw = 0;
	ret = ak_motor_turn(motor, dg);
	
	motor->mt_status = MT_STOP;
	return ret;
}

/**
* @brief  PTZ set position
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return int
* @retval return 0 success, otherwise failed
*/
int PTZControlSetPosition(int para1)
{
	akmotor[0].dg_save = akmotor[0].dg_cur;
	akmotor[1].dg_save = akmotor[1].dg_cur;
	
	posnum[para1-1].left = akmotor[0].dg_cur;
	posnum[para1-1].up = akmotor[1].dg_cur;
	
	//anyka_print("savepos left:%d, up:%d\n", akmotor[0].dg_cur, akmotor[1].dg_cur);
	anyka_set_ptz_info((void *)posnum, para1);

	return 0;
}

/**
* @brief  PTZ turn to position
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return int
* @retval return 0 success, otherwise failed
*/
int PTZControlRunPosition(int para1)
{
	anyka_print("%s.\n",__func__);
	
	if (posnum[para1-1].left > INVALID_DG || posnum[para1-1].up > INVALID_DG)
		return 1;
	
	akmotor[0].mt_status = MT_STOP;
	akmotor[1].mt_status = MT_STOP;
	akmotor[0].dg_save = posnum[para1-1].left;
	akmotor[1].dg_save = posnum[para1-1].up;
	ak_motor_runto(&akmotor[0]);
	ak_motor_runto(&akmotor[1]);
	
	return 0;
}

/**
* @brief  PTZ deinit
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return int
* @retval return 0 success, otherwise failed
*/
int PTZControlDeinit()
{
	akmotor[0].mt_status = MT_STOP;
	akmotor[1].mt_status = MT_STOP;
	
	if(akmotor[0].fd > 0)
		close(akmotor[0].fd);
	if(akmotor[1].fd > 0)
		close(akmotor[1].fd);
	
	akmotor[0].fd = -1;
	akmotor[1].fd = -1;
	return 0;
}

/**
* @brief  PTZ positions init
* 
* @author dengzhou
* @date 2013-04-07
* @param[void] 
* @return int
* @retval return 0 success, otherwise failed
*/
int PTZControlPositionInit(void)
{
	int i;

	for (i = 0; i < sizeof(posnum) / sizeof(posnum[0]); i++)
		anyka_get_ptz_info(posnum, i + 1);

	return 0;
}

