

#ifndef __HEATER_CONTROLLER_H
#define __HEATER_CONTROLLER_H

#include <stdint.h>
#include <zephyr/drivers/counter.h>

#include "moving_average.h"
#include "pid_controller.h"


struct tip_adc_counter_config {
  // 烙铁头每隔一段时间触发adc
  struct counter_alarm_cfg tip_cfg;
  // adc计数触发后，延迟一些时间等待mosfet关断,执行adc
  struct counter_alarm_cfg delay_cfg;
};

struct controller {
  struct tip_adc_counter_config adc_cfg;
  moving_avg_filter_ctx filter_ctx;
  struct pid_controller pid;
  float cur_temp; // 当前温度
  float setpoint;     // 设置温度
  bool is_sleeping;
  float sleep_setpoint;     // 休眠模式设置温度
  bool heater_on;
};

#define heater_off()                                                           \
  do {                                                                         \
    soldering_tip_pwm_set_duty_cycle(0);                                       \
  } while (0)

int soldering_tip_pwm_set_duty_cycle(uint8_t percent);

struct app;

int init_tip_controller(struct app *app);

#endif // __HEATER_CONTROLLER_H
