
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/usb_c/usbc_pd.h>
#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>

#include "app_ui.h"
#include "tft/canvas.h"

#include "tft/fonts.h"

#include "usb_pd.h"
#include "temperature_adc.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define FPS 10

static void ui_set_state(struct app *app, const enum ui_state state);

static const struct device *ina226_dev = DEVICE_DT_GET(DT_ALIAS(ina226));

static const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

// 屏幕背光控制
static const struct gpio_dt_spec lcd_blk = GPIO_DT_SPEC_GET(DT_ALIAS(lcd_blk), gpios);

static void display_init(void)
{
	if (!device_is_ready(display_dev)) {
		LOG_ERR("DISPLAY device not ready");
		return;
	}

	display_blanking_off(display_dev);
	display_set_orientation(display_dev, DISPLAY_ORIENTATION_ROTATED_90);
	draw_fill_screen(display_dev, COLOR_BLACK);
	// 开启屏幕背光
	gpio_pin_configure_dt(&lcd_blk, GPIO_OUTPUT_HIGH | GPIO_PULL_UP);
	gpio_pin_set_dt(&lcd_blk, 1);
}

static void sample_fetch(struct app *app)
{
	sensor_sample_fetch_chan(ina226_dev, SENSOR_CHAN_ALL);
}

static void main_entry(void *obj)
{
	struct app *app = (struct app *)obj;
	app->tip_ctrl->heater_on = true;
}

static void main_event(struct app *app, const enum event evt)
{
	if (evt == EVT_UP) {
		app->tip_ctrl->setpoint += 10.0f;
	} else if (evt == EVT_DOWN) {
		app->tip_ctrl->setpoint -= 10.0f;
	} else if (evt == EVT_OK) {
		ui_set_state(app, UI_PREVIEW);
		app->req.val = FULL_SCREEN;
	}
}

static enum smf_state_result main_draw(void *obj)
{
	struct app *app = (struct app *)obj;
	char buf[32];
	struct sensor_value val;

	sample_fetch(app);

	snprintf(buf, sizeof(buf), "%3d\xb0" "C", (int32_t)app->tip_ctrl->cur_temp);
	draw_text(display_dev, buf, 10, 7, Font_16x26,
            app->tip_ctrl->is_sleeping ? COLOR_GREEN : COLOR_YELLOW,
            COLOR_BLACK);

	int8_t x_off = 100;

	int8_t y_off = 2;
	snprintf(buf, sizeof(buf), "SET:%03d", (int32_t)app->tip_ctrl->setpoint);
	draw_text(display_dev, buf, x_off, y_off, Font_7x10, COLOR_YELLOW, COLOR_BLACK);

	sensor_channel_get(ina226_dev, SENSOR_CHAN_VOLTAGE, &val);
	y_off += 10 + 4;
	snprintf(buf, sizeof(buf), "U:%2d.%1dV", val.val1, (val.val2 / 100000) % 10);
	draw_text(display_dev, buf, x_off, y_off, Font_7x10, COLOR_WHITE, COLOR_BLACK);

	sensor_channel_get(ina226_dev, SENSOR_CHAN_POWER, &val);
	y_off += 10 + 4;
	snprintf(buf, sizeof(buf), "P:%2d.%1dW", val.val1, (val.val2 / 100000) % 10);
	draw_text(display_dev, buf, x_off, y_off, Font_7x10, COLOR_RED, COLOR_BLACK);

	//snprintf(buf, sizeof(buf), "%-5s", app->tip_ctrl->is_sleeping ? "sleep" : "run");
	//draw_text(display_dev, buf, x_off - 40, y_off, Font_7x10, COLOR_GREEN, COLOR_BLACK);
	return SMF_EVENT_HANDLED;
}

static void pid_tuning_entry(void *obj)
{
}

static enum smf_state_result pid_tuning_draw(void *obj)
{
	char buf[32];
	struct app *app = (struct app *)obj;
	snprintf(buf, sizeof(buf), "%03d", (int32_t)app->tip_ctrl->cur_temp);
	draw_text(display_dev, buf, 10, 7, Font_16x26, COLOR_YELLOW, COLOR_BLACK);

	int8_t x_off = 100;

	int8_t y_off = 2;
	snprintf(buf, sizeof(buf), "Kp:%03d", (int32_t)(pid_get_kp(&app->tip_ctrl->pid) * 100));
	draw_text(display_dev, buf, x_off, y_off, Font_7x10, COLOR_YELLOW, COLOR_BLACK);

	y_off += 10 + 3;
	snprintf(buf, sizeof(buf), "Ki:%03d", (int32_t)(pid_get_ki(&app->tip_ctrl->pid) * 100));
	draw_text(display_dev, buf, x_off, y_off, Font_7x10, COLOR_WHITE, COLOR_BLACK);

	y_off += 10 + 3;
	snprintf(buf, sizeof(buf), "Kd:%03d", (int32_t)(pid_get_kd(&app->tip_ctrl->pid) * 100));
	draw_text(display_dev, buf, x_off, y_off, Font_7x10, COLOR_RED, COLOR_BLACK);

	snprintf(buf, sizeof(buf), "%d", app->p_adj);
	draw_text(display_dev, buf, 80, 25, Font_7x10, COLOR_YELLOW, COLOR_BLACK);
	return SMF_EVENT_HANDLED;
}

static void pid_tuning_event(struct app *app, const enum event evt)
{
	if (evt == EVT_OK) { // 切换pid调整的参数类型
		enum pid_adj adj = app->p_adj;
		if (adj < ADJ_KD) {
			adj++;
		} else {
			adj = ADJ_KP;
		}
		app->p_adj = adj;
		return;
	}
	float kp = pid_get_kp(&app->tip_ctrl->pid);
	float ki = pid_get_ki(&app->tip_ctrl->pid);
	float kd = pid_get_kd(&app->tip_ctrl->pid);

	switch (app->p_adj) {
	case ADJ_KP:
		if (evt == EVT_UP) {
			kp += 0.1f;
		} else if (evt == EVT_DOWN) {
			kp -= 0.1f;
		}
		break;
	case ADJ_KI:
		if (evt == EVT_UP) {
			ki += 0.01f;
		} else if (evt == EVT_DOWN) {
			ki -= 0.01f;
		}
		break;
	case ADJ_KD:

		if (evt == EVT_UP) {
			kd += 0.1f;
		} else if (evt == EVT_DOWN) {
			kd -= 0.1f;
		}
		break;
	}
	pid_set_tunings(&app->tip_ctrl->pid, kp, ki, kd);
}

static void preview_entry(void *obj)
{
	struct app *app = (struct app *)obj;
	update_cool_temp();
	app->tip_ctrl->heater_on = false;
}

static enum smf_state_result preview_draw(void *obj)
{
	struct app *app = (struct app *)obj;
	char buf[32];
	struct sensor_value val;

	sample_fetch(app);

	int8_t x_off = 80;

	int8_t y_off = 8;

	snprintf(buf, sizeof(buf), "%3d", (int32_t)app->tip_ctrl->cur_temp);
	draw_text(display_dev, buf, 0, y_off, Font_7x10, COLOR_YELLOW, COLOR_BLACK);

	sensor_channel_get(ina226_dev, SENSOR_CHAN_VOLTAGE, &val);
	snprintf(buf, sizeof(buf), "VBUS:%2d.%1dV", val.val1, (val.val2 / 100000) % 10);
	draw_text(display_dev, buf, x_off, y_off, Font_7x10, COLOR_WHITE, COLOR_BLACK);
	y_off += 18;

	int16_t die_temp = (int16_t)(read_die_temp() * 10);
	snprintf(buf, sizeof(buf), "TEMP:%2d.%1d", die_temp / 10, die_temp % 10);
	draw_text(display_dev, buf, 0, y_off, Font_7x10, COLOR_RED, COLOR_BLACK);
	return SMF_EVENT_HANDLED;
}

static enum event preview_event(struct app *app)
{
	enum event evt;
	if (check_pd_ready(app->pd_data)) { // 检测pd请求是否已完成
		sample_fetch(app);
		struct sensor_value val;
		sensor_channel_get(ina226_dev, SENSOR_CHAN_VOLTAGE, &val);
		if (abs(CONFIG_PD_MAX_REQUESTED_VOLTAGE - (int)(sensor_value_to_double(&val) * 1000)) <
		    2000) { // 检测实际电压跟请求电压差不超过2V
			evt = EVT_HOME;
		} else {
			// 尝试重新请求pd
			pd_send_hard_reset();
			evt = EVT_EMPTY;
		}
	} else { // 什么都不做,等待pd通信完成
		evt = EVT_EMPTY;
	}
	return evt;
}

static const struct smf_state ui_states[UI_STATE_COUNT] = {
	[UI_MAIN] = SMF_CREATE_STATE(main_entry, main_draw, NULL, NULL, NULL),
	[UI_PID_TUNING] = SMF_CREATE_STATE(pid_tuning_entry, pid_tuning_draw, NULL, NULL, NULL),
	[UI_PREVIEW] = SMF_CREATE_STATE(preview_entry, preview_draw, NULL, NULL, NULL),
};

static void ui_set_state(struct app *app, const enum ui_state state)
{
	smf_set_state(SMF_CTX(app), &ui_states[state]);
}

static enum ui_state ui_get_current_state(struct app *app)
{
	return app->ctx.current - &ui_states[0];
}

void app_init(struct app *app)
{

	display_init();

	if (!device_is_ready(ina226_dev)) {
		LOG_ERR("INA226 device not ready");
		return;
	}

	memset(app, 0, sizeof(struct app));

	k_mutex_init(&app->mutex);
	app->last_time = k_uptime_get_32() + (1000 / FPS) * 2; // 保证第一次被刷新

	smf_set_initial(SMF_CTX(app), &ui_states[UI_PREVIEW]);
}

void app_draw(struct app *app)
{
	struct req_value *req;
	if (k_mutex_lock(&app->mutex, K_FOREVER)) {
		return;
	}

	// 控制帧数
	int32_t delay = MIN((1000 / FPS) - (k_uptime_get_32() - app->last_time),
			    2000); // 防止可能的时间溢出导致等待时间过长
	k_timeout_t timeout;
	if (delay <= 0) {
		timeout = K_NO_WAIT;
	} else {
		timeout = K_MSEC(delay);
	}

	// 等待超时或者提前有事件发生，需要马上刷新界面
	req = k_fifo_get(&app->req_fifo, timeout);

	app->last_time = k_uptime_get_32(); // 从这里记录时间，把屏幕刷新所用时间计算进去

	if (req != NULL && req->val == FULL_SCREEN) {
		draw_fill_screen(display_dev, COLOR_BLACK);
	}
	smf_run_state(SMF_CTX(app));

	k_mutex_unlock(&app->mutex);
}

void app_event_handler(struct app *app, enum event evt)
{
	if (k_mutex_lock(&app->mutex, K_FOREVER)) {
		return;
	}
	enum ui_state state = ui_get_current_state(app);
	app->req.val = NORMAL;
	// 防止preview界面被反复进入
	if (state == UI_PREVIEW && evt != EVT_ENTER_PREVIEW) {
		evt = preview_event(app); // 任意按键，检测pd电压是否达到要求
	}
	switch (evt) {
	case EVT_NEXT: {
		state++;
		if (state >= UI_STATE_COUNT) {
			// 回到初始界面
			state = UI_MAIN;
		}
		ui_set_state(app, state);
		app->req.val = FULL_SCREEN;
		break;
	}
	case EVT_BACK: {
		state--;
		if (state < UI_MAIN) {
			state = UI_STATE_COUNT - 1; // 到最后界面
		}
		ui_set_state(app, state);
		app->req.val = FULL_SCREEN;
		break;
	}
	case EVT_HOME: {
		if (state != UI_MAIN) {
			ui_set_state(app, UI_MAIN);
			app->req.val = FULL_SCREEN;
		}
		break;
	}
	case EVT_UP:
	case EVT_DOWN:
	case EVT_OK: {
		if (state == UI_MAIN) {
			main_event(app, evt);
		} else if (state == UI_PID_TUNING) {
			pid_tuning_event(app, evt);
		}
		break;
	}
	case EVT_ENTER_PREVIEW: {
		if (state != UI_PREVIEW) {
			ui_set_state(app, UI_PREVIEW);
			app->req.val = FULL_SCREEN;
		}
		break;
	}
	case EVT_EMPTY:
	default:
		break;
	}
	// 发生事件，马上刷新屏幕
	k_fifo_put(&app->req_fifo, &app->req);

	k_mutex_unlock(&app->mutex);
}
