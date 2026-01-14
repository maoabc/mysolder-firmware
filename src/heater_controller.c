

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app_ui.h"
#include "heater_controller.h"
#include "temperature_adc.h"

LOG_MODULE_REGISTER(soldering_tip_controller);

#define PID_KP (CONFIG_PID_KP_1000X / 1000.0f)
#define PID_KI (CONFIG_PID_KI_1000X / 1000.0f)
#define PID_KD (CONFIG_PID_KD_1000X / 1000.0f)

#define PID_MAX_OUTPUT 450

// 定时采样通道
#define TIP_ADC_COUNTER_CHAN 0
/* 用于延时的timers通道 */
#define TIP_ADC_DELAY_CHAN   1

static struct controller tip_ctrl;

// 烙铁pwm相关控制
//
#define PWM_DEVICE    DT_NODELABEL(solder_heater)
// 从设备树获得pwm周期
#define PWM_PERIOD_NS DT_PWMS_PERIOD_BY_IDX(PWM_DEVICE, 0)

static const struct pwm_dt_spec pwm_dev = PWM_DT_SPEC_GET(PWM_DEVICE);

int soldering_tip_pwm_set_duty_cycle(uint8_t percent)
{
	if (percent > CONFIG_MAX_DUTY_CYCLE) {
		percent = CONFIG_MAX_DUTY_CYCLE;
	}
	if (percent < 0) {
		percent = 0;
	}

	/* 将百分比转换为纳秒 */
	uint32_t pulse_ns = (percent * PWM_PERIOD_NS) / 100;

	/* 设置 PWM 周期和脉宽 */
	return pwm_set_dt(&pwm_dev, PWM_PERIOD_NS, pulse_ns);
}

static void heater_update()
{
	// 测试
	uint8_t duty;
	if (tip_ctrl.heater_on) {
		float pid_out = pid_get_output(&tip_ctrl.pid);
		duty = (uint8_t)(((pid_out / PID_MAX_OUTPUT)) * CONFIG_MAX_DUTY_CYCLE);
	} else {
		duty = 0;
	}
	soldering_tip_pwm_set_duty_cycle(duty);
}

//

static const struct device *tip_adc_counter_dev = DEVICE_DT_GET(DT_NODELABEL(tip_adc_counter));

static void adc_work_handler(struct k_work *work)
{
	uint32_t temp_raw;
	// 执行adc 然后启动pwm
	temp_read_adc_raw(&temp_raw);

	temp_raw = moving_avg_compute(&tip_ctrl.filter_ctx, temp_raw);

	bool need_compute;
#if ((CONFIG_PID_COMPUTE_INTERVAL / CONFIG_TIP_SAMPLING_PERIOD_MS) > 1)
	static uint8_t c = 0;
	c++;
	if (c >= (CONFIG_PID_COMPUTE_INTERVAL / CONFIG_TIP_SAMPLING_PERIOD_MS)) {
		c = 0;
		need_compute = tip_ctrl.heater_on;
	} else {
		need_compute = false;
	}
#else
	need_compute = tip_ctrl.heater_on;
#endif

	float tt = temp_raw_to_temperature(temp_raw);
	tip_ctrl.cur_temp = tt;
	if (need_compute) {
		pid_compute(&tip_ctrl.pid, tt,
			    tip_ctrl.is_sleeping ? tip_ctrl.sleep_setpoint : tip_ctrl.setpoint);
	}

	heater_update();
}

K_WORK_DEFINE(adc_work, adc_work_handler);

K_THREAD_STACK_DEFINE(adc_workq_stack, 1024);
static struct k_work_q adc_workq;

static void tip_adc_counter_callback(const struct device *dev, uint8_t chan_id, uint32_t ticks,
				     void *user_data)
{
	struct tip_adc_counter_config *tip_adc_cfg = user_data;
	// 关闭烙铁pwm
	heater_off();

	// 等待mosfet完全关断执行adc
	counter_set_channel_alarm(dev, TIP_ADC_DELAY_CHAN, &tip_adc_cfg->delay_cfg);

	// 触发下一次计数
	// 因为执行adc时间远小于两次adc间隔,所以直接这里触发
	counter_set_channel_alarm(dev, TIP_ADC_COUNTER_CHAN, &tip_adc_cfg->tip_cfg);
}

static void tip_adc_delay_callback(const struct device *dev, uint8_t chan_id, uint32_t ticks,
				   void *user_data)
{
	// 取消可能没被执行的任务
	k_work_cancel(&adc_work);
	// 提交adc任务
	k_work_submit_to_queue(&adc_workq, &adc_work);
}

int init_tip_controller(struct app *app)
{
	int ret;

	k_work_queue_init(&adc_workq);
	// 启动队列对应的线程
	k_work_queue_start(&adc_workq,
			   adc_workq_stack,                        // 栈地址
			   K_THREAD_STACK_SIZEOF(adc_workq_stack), // 栈大小
			   K_PRIO_PREEMPT(3),                      // 优先级
			   NULL);
	if (!device_is_ready(tip_adc_counter_dev)) {
		LOG_ERR("Thermocouple adc counter device not ready");
		return -ENODEV;
	}
	if (!device_is_ready(pwm_dev.dev)) {
		LOG_ERR("PWM device not ready");
		return -ENODEV;
	}
	moving_avg_init(&tip_ctrl.filter_ctx, 2);
	tip_ctrl.setpoint = CONFIG_RUNNING_SETPOINT_C;
	tip_ctrl.heater_on = false;
	tip_ctrl.sleep_setpoint = CONFIG_SLEEPING_SETPOINT_C;
	tip_ctrl.is_sleeping = false;

	pid_init(&tip_ctrl.pid, PID_KP, PID_KI, PID_KD, CONFIG_PID_COMPUTE_INTERVAL_MS,
		 PID_CD_DIRECT);
	pid_set_output_limits(&tip_ctrl.pid, 0, PID_MAX_OUTPUT);

	counter_start(tip_adc_counter_dev);

	// 对热电偶进行采样
	tip_ctrl.adc_cfg.tip_cfg.flags = 0;
	tip_ctrl.adc_cfg.tip_cfg.ticks =
		counter_us_to_ticks(tip_adc_counter_dev, CONFIG_TIP_SAMPLING_PERIOD_MS * 1000);
	tip_ctrl.adc_cfg.tip_cfg.callback = tip_adc_counter_callback;
	tip_ctrl.adc_cfg.tip_cfg.user_data = &tip_ctrl.adc_cfg;

	// 等待mos完全关断延时配置
	tip_ctrl.adc_cfg.delay_cfg.flags = 0;
	tip_ctrl.adc_cfg.delay_cfg.ticks =
		counter_us_to_ticks(tip_adc_counter_dev, CONFIG_MOSFET_OFF_DELAY_US);
	tip_ctrl.adc_cfg.delay_cfg.callback = tip_adc_delay_callback;
	tip_ctrl.adc_cfg.delay_cfg.user_data = &tip_ctrl.adc_cfg;

	// 触发adc计数
	ret = counter_set_channel_alarm(tip_adc_counter_dev, TIP_ADC_COUNTER_CHAN,
					&tip_ctrl.adc_cfg.tip_cfg);
	if (ret != 0) {
		LOG_ERR("Thermocouple adc counter setup failed: %d", ret);
		return ret;
	}
	app->tip_ctrl = &tip_ctrl;

	return 0;
}
