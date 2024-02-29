/*
 * @Author: jszhang jszhang@nreal.ai
 * @Date: 2023-03-01 06:45:59
 * @LastEditors: jszhang jszhang@nreal.ai
 * @LastEditTime: 2023-03-02 02:40:28
 */
#pragma once

void elementwise_mult_hm(float* __restrict hm_input, float* __restrict coeffs, float* __restrict hm_output,
                         int channels, int size);
