#ifndef __PID_CONTROLLER_H
#define __PID_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

#define PID_CD_DIRECT 0
#define PID_CD_REVERSE 1

typedef struct pid_controller {
  float output;                    // 输出值
  float kp, ki, kd;                // 调谐参数
  float disp_kp, disp_ki, disp_kd; // 用于显示的调谐参数
  float output_sum;                // 积分项累积
  float last_input;                // 上一次输入值
  uint32_t sample_time;            // 采样时间（毫秒）
  float out_min, out_max;          // 输出限制
  uint8_t controller_direction;    // 控制方向
} pid_controller;

void pid_init(pid_controller *pid, float kp, float ki, float kd,
              uint32_t sample_time, uint8_t controller_direction);

float pid_compute(pid_controller *pid, float input, float setpoint);
void pid_set_output_limits(pid_controller *pid, float min, float max);

void pid_set_sample_time(pid_controller *pid, uint32_t new_sample_time);
void pid_set_tunings(pid_controller *pid, float kp, float ki, float kd);
void pid_set_controller_direction(pid_controller *pid, uint8_t direction);
float pid_get_kp(pid_controller *pid);
float pid_get_ki(pid_controller *pid);
float pid_get_kd(pid_controller *pid);
uint8_t pid_get_direction(pid_controller *pid);

float pid_get_output(pid_controller *pid);

#endif //__PID_CONTROLLER_H
