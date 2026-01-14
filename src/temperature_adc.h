
#ifndef __TEMPERATURE_ADC_H
#define __TEMPERATURE_ADC_H

#include <stdint.h> 

/**
 * @brief 初始化 ADC，用于温度测量
 *
 * @return 0 表示成功，非 0 表示失败
 */
int temp_adc_init(void);


void update_cool_temp();

double read_die_temp();

/**
 * @brief 从 ADC 读取电压值
 *
 * @param voltage_mv 指向存储电压值（单位：mV）的指针
 * @return 0 表示成功，非 0 表示失败
 */
int temp_read_adc_voltage(uint32_t *voltage_mv);


int temp_read_adc_raw(uint32_t *raw);

float temp_raw_to_temperature(uint32_t raw);

#endif /* __TEMPERATURE_ADC_H */
