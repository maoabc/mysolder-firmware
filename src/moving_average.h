/**
  ******************************************************************************
  * @file    moving_average.h
  * @author  Mohammad Hussein Tavakoli Bina, Sepehr Hashtroudi
  * @brief   This file contains function prototypes for moving average filter
  * @remark  2024-09-19 - Edited by Axel Johansson, Adding function
  *          moving_avg_set_value
  ******************************************************************************
  * MIT License
  *
  * Copyright (c) 2018 Mohammad Hussein Tavakoli Bina
  *
  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to deal
  * in the Software without restriction, including without limitation the rights
  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  * copies of the Software, and to permit persons to whom the Software is
  * furnished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in all
  * copies or substantial portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  * SOFTWARE.
  */

#ifndef __MOVING_AVERAGE_H
#define __MOVING_AVERAGE_H

#include <stdint.h>

#define MAX_WINDOW_LENGTH 16

typedef struct {
    uint32_t window_length;
    uint32_t window_pointer; /* Pointer to the first element of window */
    uint32_t history[MAX_WINDOW_LENGTH]; /* Array to store values of filter window */
    uint32_t sum; /* Sum of filter window's elements */
} moving_avg_filter_ctx;


/**
  * @brief  This function initializes filter's data structure
  * @param  ctx: Data structure
  * @param  window_length: Length of the moving average window
  * @retval None
  */
void moving_avg_init(moving_avg_filter_ctx *ctx, uint32_t window_length);

/**
  * @brief  This function filters data with moving average filter
  * @param  ctx: Data structure
  * @param  raw_data: Input raw sensor data
  * @retval Filtered value
  */
uint32_t moving_avg_compute(moving_avg_filter_ctx *ctx, uint32_t raw_data);

/**
  * @brief  Fill moving average filter history with raw_data
  * @param  ctx: Data structure
  * @param  raw_data: Input raw sensor data
  * @retval None
  */
void moving_avg_set_value(moving_avg_filter_ctx *ctx, uint32_t raw_data);

#endif /* __MOVING_AVERAGE_H */
