/*
 * @Author: Zhang Junsong
 * @Date: 2023-03-01 06:44:12
 * @Last Modified by:   Zhang Junsong
 * @Last Modified time: 2023-03-01 06:44:12
 */
#pragma once
void reduce_sum_w(float* __restrict input, float* __restrict output, int num_channels, int height, int width);
void reduce_sum_h(float* __restrict input, float* __restrict output, int num_channels, int height, int width);
