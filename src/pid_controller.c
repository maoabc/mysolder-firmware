

#include "pid_controller.h"
#include <math.h>

// 初始化 PID
void pid_init(pid_controller *pid, float kp, float ki, float kd, uint32_t sample_time,
	      uint8_t controller_direction)
{

	pid->output = 0;
	pid->sample_time = sample_time;

	pid_set_output_limits(pid, 0, 100);

	// 设置控制方向和调谐参数
	pid_set_controller_direction(pid, controller_direction);
	pid_set_tunings(pid, kp, ki, kd);

	// 初始化积分项和上次输入
	pid->output_sum = 0;
	pid->last_input = 0;
}

// 计算 PID 输出
float pid_compute(pid_controller *pid, float input, float setpoint)
{

	float error = setpoint - input;
	float d_input = (input - pid->last_input);

	// 积分项
	// if(fabsf(error)>5.0f){
	pid->output_sum += (pid->ki * error);
	//}

	// 限幅积分项
	if (pid->output_sum > pid->out_max) {
		pid->output_sum = pid->out_max;
	} else if (pid->output_sum < pid->out_min) {
		pid->output_sum = pid->out_min;
	}

	// 计算输出
	float output = pid->kp * error;

	output += pid->output_sum - pid->kd * d_input;

	// 限幅输出
	if (output > pid->out_max) {
		output = pid->out_max;
	} else if (output < pid->out_min) {
		output = pid->out_min;
	}

	pid->output = output;

	// 保存状态
	pid->last_input = input;
	return output;
}

// 设置输出限制
void pid_set_output_limits(pid_controller *pid, float min, float max)
{
	if (min >= max) {
		return;
	}

	pid->out_min = min;
	pid->out_max = max;

	if (pid->output > pid->out_max) {
		pid->output = pid->out_max;
	} else if (pid->output < pid->out_min) {
		pid->output = pid->out_min;
	}

	if (pid->output_sum > pid->out_max) {
		pid->output_sum = pid->out_max;
	} else if (pid->output_sum < pid->out_min) {
		pid->output_sum = pid->out_min;
	}
}

// 设置调谐参数
void pid_set_tunings(pid_controller *pid, float kp, float ki, float kd)
{
	if (kp < 0 || ki < 0 || kd < 0) {
		return;
	}

	pid->disp_kp = kp;
	pid->disp_ki = ki;
	pid->disp_kd = kd;

	float sample_time_in_sec = (float)pid->sample_time / 1000.0f;
	pid->kp = kp;
	pid->ki = ki * sample_time_in_sec;
	pid->kd = kd / sample_time_in_sec;

	if (pid->controller_direction == PID_CD_REVERSE) {
		pid->kp = -pid->kp;
		pid->ki = -pid->ki;
		pid->kd = -pid->kd;
	}
}
// 设置采样时间
void pid_set_sample_time(pid_controller *pid, uint32_t new_sample_time)
{
	if (new_sample_time > 0) {
		float ratio = (float)new_sample_time / (float)pid->sample_time;
		pid->ki *= ratio;
		pid->kd /= ratio;
		pid->sample_time = new_sample_time;
	}
}

// 设置控制方向
void pid_set_controller_direction(pid_controller *pid, uint8_t direction)
{
	if (direction != pid->controller_direction) {
		pid->kp = -pid->kp;
		pid->ki = -pid->ki;
		pid->kd = -pid->kd;
	}
	pid->controller_direction = direction;
}

// 获取参数
float pid_get_kp(pid_controller *pid)
{
	return pid->disp_kp;
}
float pid_get_ki(pid_controller *pid)
{
	return pid->disp_ki;
}
float pid_get_kd(pid_controller *pid)
{
	return pid->disp_kd;
}
uint8_t pid_get_direction(pid_controller *pid)
{
	return pid->controller_direction;
}
float pid_get_output(pid_controller *pid)
{
	return pid->output;
}
