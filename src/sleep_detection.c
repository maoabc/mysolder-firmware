
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <math.h>
#include "sleep_detection.h"
#include "app_ui.h"

// 日志模块
LOG_MODULE_REGISTER(sleep_detection, LOG_LEVEL_INF);

// 定义常量
#define AY_MIN          3.0f           // 工作姿态Y轴最小加速度（m/s²）
#define SLEEP_TIMEOUT   5000           // 休眠超时（ms）
#define STOP_TIMEOUT    1000 * 10 * 60 // 暂停超时（ms）,这里10分钟一直sleep模式就进入stop模式
#define SAMPLE_INTERVAL 200            // 采样间隔（ms）

static const struct device *lis2dw_dev = DEVICE_DT_GET(DT_NODELABEL(lis2dw));

// 姿态检测线程函数
static void posture_detection_thread(void *arg1, void *arg2, void *arg3)
{
	struct app *app = (struct app *)arg1;
	struct sensor_value accel[3]; // X, Y, Z加速度

	static uint32_t sleep_timer_start = 0;

	if (!device_is_ready(lis2dw_dev)) {
		LOG_ERR("LIS2DW12 device not ready");
		return;
	}

	while (1) {
		// 读取加速度
		if (sensor_sample_fetch_chan(lis2dw_dev, SENSOR_CHAN_ACCEL_XYZ) < 0) {
			LOG_ERR("Failed to fetch sensor data");
            goto failed;
		}

		if (sensor_channel_get(lis2dw_dev, SENSOR_CHAN_ACCEL_XYZ, accel) < 0) {
			LOG_ERR("Failed to get acceleration data");
            goto failed;
		}

		float ax = sensor_value_to_float(&accel[0]);
		float ay = sensor_value_to_float(&accel[1]);
		float az = sensor_value_to_float(&accel[2]);

		// 计算总加速度
		float a_total = sqrtf(ax * ax + ay * ay + az * az);

		app->test = (int32_t)(ay * 100);

		// 判断姿态
		if (ay < AY_MIN &&
		    fabsf(a_total - 9.81f) < 1.0f) { // 非向下倾斜且微小移动则可能进入休眠计时
			// 非工作姿态或晃动
			if (sleep_timer_start == 0) {
				sleep_timer_start = k_uptime_get_32();
			}

			// 检查休眠超时
			uint32_t time_diff = k_uptime_get_32() - sleep_timer_start;

			// 先判断更长时间的停止状态，然后判断休眠
			if (sleep_timer_start > 0 && time_diff >= STOP_TIMEOUT) {
				app->tip_ctrl->is_sleeping = true;
				app_event_handler(app, EVT_ENTER_PREVIEW); // 进入预览界面会关闭加热
			} else if (sleep_timer_start > 0 && time_diff >= SLEEP_TIMEOUT) {
				app->tip_ctrl->is_sleeping = true;
			}
		} else {
			sleep_timer_start = 0;
			if (app->tip_ctrl->is_sleeping) {
				app->tip_ctrl->is_sleeping = false;
			}
		}
failed:
		k_msleep(SAMPLE_INTERVAL);
	}
}

// 线程栈和ID
K_THREAD_STACK_DEFINE(posture_thread_stack, 512);
static struct k_thread posture_thread_data;

// 初始化并启动休眠检测
int sleep_detection_init(struct app *app)
{
	if (!device_is_ready(lis2dw_dev)) {
		LOG_ERR("LIS2DW device not ready");
		return -EINVAL;
	}
	// 启动线程
	k_thread_create(&posture_thread_data, posture_thread_stack,
			K_THREAD_STACK_SIZEOF(posture_thread_stack), posture_detection_thread,
			(void *)app, NULL, NULL, 100, 0, K_NO_WAIT);

	k_thread_name_set(&posture_thread_data, "sleep_detection");

	LOG_INF("Sleep detection thread started");
	return 0;
}
