
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>

#include <zephyr/drivers/usb_c/usbc_pd.h>
#include <zephyr/drivers/usb_c/usbc_tc.h>
#include <zephyr/drivers/usb_c/usbc_tcpc.h>

#include "app_ui.h"
#include "usb_pd.h"

#include "heater_controller.h"
#include "temperature_adc.h"
#include "sleep_detection.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static struct app app;

static void input_cb(struct input_event *evt, void *user_data)
{
	if (evt->value) { // press down
		if (evt->code == INPUT_KEY_A) {
			app_event_handler(&app, EVT_UP);
		} else if (evt->code == INPUT_KEY_B) {
			app_event_handler(&app, EVT_DOWN);
		} else if (evt->code == INPUT_KEY_X) {
			app_event_handler(&app, EVT_NEXT);
		} else if (evt->code == INPUT_KEY_Y) {
			app_event_handler(&app, EVT_OK);
		}
	}
}

INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);


int main(void)
{
	app_init(&app);
	pd_start(&app);
	temp_adc_init();

	if (init_tip_controller(&app)) {
		LOG_ERR("Soldering tip controller init failed");
		return -1;
	}
	if(sleep_detection_init(&app)){
		LOG_ERR("Sleep detection init failed");
		return -1;
    }

	while (1) {
		app_draw(&app);
	}
	return 0;
}
