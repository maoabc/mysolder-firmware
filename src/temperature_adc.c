
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>

#include "temperature_adc.h"

LOG_MODULE_REGISTER(tip_temp_adc, LOG_LEVEL_INF);

#define GAIN 201.0f // OPA333 增益

// 获取 ADC 通道配置
static const struct adc_dt_spec adc_chan0 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

#define ADC_REF_MV (adc_chan0.vref_mv)
#define ADC_RES_LEVELS (1u << adc_chan0.resolution)
//#define ADC_REF_MV     3300
//#define ADC_RES_LEVELS 4096
//




static const struct device *die_sensor = DEVICE_DT_GET(DT_NODELABEL(die_temp));
static double cool_temp;

// 初始化 ADC
int temp_adc_init()
{
	int ret;

	if (!device_is_ready(adc_chan0.dev)) {
		LOG_ERR("ADC device not ready");
		return -ENODEV;
	}

	ret = adc_channel_setup_dt(&adc_chan0);
	if (ret != 0) {
		LOG_ERR("ADC channel setup failed: %d", ret);
		return ret;
	}
	if (!device_is_ready(die_sensor)) {
		LOG_ERR("DIE temperature  is not ready");
		return -ENODEV;
	}
    update_cool_temp();

	return 0;
}

void update_cool_temp()
{
	cool_temp = read_die_temp();
}

double read_die_temp()
{
	int ret;
	struct sensor_value temp;
	ret = sensor_sample_fetch_chan(die_sensor, SENSOR_CHAN_DIE_TEMP);
	if (ret < 0) {
		return 25;
	}

	ret = sensor_channel_get(die_sensor, SENSOR_CHAN_DIE_TEMP, &temp);
	if (ret < 0) {
		return 25;
	}
	return sensor_value_to_double(&temp);
}

// 读取 ADC 值并转换为电压
int temp_read_adc_voltage(uint32_t *voltage_mv)
{
	int ret;
	*voltage_mv = 0;

	struct adc_sequence sequence = {
		.buffer = voltage_mv,
		.buffer_size = sizeof(*voltage_mv),
	};
	adc_sequence_init_dt(&adc_chan0, &sequence);

	ret = adc_read_dt(&adc_chan0, &sequence);
	if (ret != 0) {
		LOG_ERR("ADC read failed: %d", ret);
		return ret;
	}
	return adc_raw_to_millivolts_dt(&adc_chan0, voltage_mv);
}

int temp_read_adc_raw(uint32_t *raw)
{
	int ret;

	*raw = 0;

	struct adc_sequence sequence = {
		.options = NULL,
		.buffer = raw,
		.buffer_size = sizeof(*raw),
		.calibrate = false,
	};
	adc_sequence_init_dt(&adc_chan0, &sequence);

	ret = adc_read_dt(&adc_chan0, &sequence);
	if (ret != 0) {
		LOG_ERR("ADC read failed: %d", ret);
		return ret;
	}
	return 0;
}

#if defined(CONFIG_BOARD_T12_G431)

// From https://github.com/Ralim/IronOS/blob/dev/source/Core/BSP/Miniware/ThermoModel.cpp

static const int32_t uVtoDegC[] = {
	//
	//
	0,     0,   //
	266,   10,  //
	522,   20,  //
	770,   30,  //
	1010,  40,  //
	1244,  50,  //
	1473,  60,  //
	1697,  70,  //
	1917,  80,  //
	2135,  90,  //
	2351,  100, //
	2566,  110, //
	2780,  120, //
	2994,  130, //
	3209,  140, //
	3426,  150, //
	3644,  160, //
	3865,  170, //
	4088,  180, //
	4314,  190, //
	4544,  200, //
	4777,  210, //
	5014,  220, //
	5255,  230, //
	5500,  240, //
	5750,  250, //
	6003,  260, //
	6261,  270, //
	6523,  280, //
	6789,  290, //
	7059,  300, //
	7332,  310, //
	7609,  320, //
	7889,  330, //
	8171,  340, //
	8456,  350, //
	8742,  360, //
	9030,  370, //
	9319,  380, //
	9607,  390, //
	9896,  400, //
	10183, 410, //
	10468, 420, //
	10750, 430, //
	11029, 440, //
	11304, 450, //
	11573, 460, //
	11835, 470, //
	12091, 480, //
	12337, 490, //
	12575, 500, //

};

static int32_t LinearInterpolate(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x)
{
	return y1 + (((((x - x1) * 1000) / (x2 - x1)) * (y2 - y1))) / 1000;
}
int32_t InterpolateLookupTable(const int32_t *lookupTable, const int noItems, const int32_t value)
{
	for (int i = 1; i < (noItems - 1); i++) {
		// If current tip temp is less than current lookup, then this current lookup
		// is the higher point to interpolate
		if (value < lookupTable[i * 2]) {
			return LinearInterpolate(lookupTable[(i - 1) * 2],
						 lookupTable[((i - 1) * 2) + 1], lookupTable[i * 2],
						 lookupTable[(i * 2) + 1], value);
		}
	}
	return LinearInterpolate(
		lookupTable[(noItems - 2) * 2], lookupTable[((noItems - 2) * 2) + 1],
		lookupTable[(noItems - 1) * 2], lookupTable[((noItems - 1) * 2) + 1], value);
}
#define ITEM_COUNT (sizeof(uVtoDegC) / (2 * sizeof(uVtoDegC[0])))

float temp_raw_to_temperature(uint32_t raw)
{
	uint32_t uV = (raw * ADC_REF_MV) / ADC_RES_LEVELS;
	uV = uV * (1000 / GAIN);
	return InterpolateLookupTable(uVtoDegC, ITEM_COUNT, uV) + cool_temp;
}
#elif  defined(CONFIG_BOARD_C245_G431)

// From https://github.com/AxxAxx/AxxSolder
//

#define TC_COMPENSATION_X2_T245 (-6.818562488097707e-07)
#define TC_COMPENSATION_X1_T245 0.1432374243560926
#define TC_COMPENSATION_X0_T245 23.777399955382318

float temp_raw_to_temperature(uint32_t raw)
{
	return raw * raw * TC_COMPENSATION_X2_T245 + raw * TC_COMPENSATION_X1_T245 + cool_temp;
}
#else

// From https://github.com/AxxAxx/AxxSolder
//
#define TC_COMPENSATION_X2_T210 (6.082461666584128e-06)
#define TC_COMPENSATION_X1_T210 0.3823655573322506
// #define TC_COMPENSATION_X0_T210 20.968033870812942

float temp_raw_to_temperature(uint32_t raw)
{
	return raw * raw * TC_COMPENSATION_X2_T210 + raw * TC_COMPENSATION_X1_T210 + cool_temp;
}
#endif
