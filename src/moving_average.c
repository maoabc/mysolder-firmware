
/**
  ******************************************************************************
  * @file    moving_average.c
  * @author  Mohammad Hussein Tavakoli Bina, Sepehr Hashtroudi
  * @brief   This file contains an efficient implementation of moving average filter
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

#include "moving_average.h"

void moving_avg_init(moving_avg_filter_ctx *ctx, uint32_t window_length)
{
    ctx->window_length = window_length;
    ctx->window_pointer = 0;
    ctx->sum = 0;

    for (uint32_t i = 0; i < ctx->window_length; i++) {
        ctx->history[i] = 0;
    }
}

uint32_t moving_avg_compute(moving_avg_filter_ctx *ctx, uint32_t raw_data)
{
    ctx->sum += raw_data;
    ctx->sum -= ctx->history[ctx->window_pointer];
    ctx->history[ctx->window_pointer] = raw_data;
    if (ctx->window_pointer < ctx->window_length - 1) {
        ctx->window_pointer += 1;
    } else {
        ctx->window_pointer = 0;
    }
    return ctx->sum /ctx->window_length;
}

void moving_avg_set_value(moving_avg_filter_ctx *ctx, uint32_t raw_data)
{
    ctx->sum = raw_data * ctx->window_length;
    ctx->window_pointer = 0;

    for (uint32_t i = 0; i < ctx->window_length; i++) {
        ctx->history[i] = raw_data;
    }
}
