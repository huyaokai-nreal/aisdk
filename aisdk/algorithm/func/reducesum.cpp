/*
 * @Author: Zhang Junsong
 * @Date: 2023-03-01 06:44:09
 * @Last Modified by: Zhang Junsong
 * @Last Modified time: 2023-03-01 06:44:56
 */
#include "reducesum.h"

/**
 * @description:
 * @param {float* __restrict} input
 * @param {float* __restrict} output
 * @param {int} num_channels
 * @param {int} height
 * @param {int} width
 * @return {*}
 */
void reduce_sum_w(float* __restrict input, float* __restrict output, int num_channels, int height, int width) {
    for (int c = 0; c < num_channels; ++c) {
        for (int h = 0; h < height; ++h) {
            float sum = 0.0f;
            for (int w = 0; w < width; ++w) {
                sum += input[c * height * width + h * width + w];
            }
            output[c * height + h] = sum;
        }
    }
}

/**
 * @description:
 * @param {float* __restrict} input
 * @param {float* __restrict} output
 * @param {int} num_channels
 * @param {int} height
 * @param {int} width
 * @return {*}
 */
void reduce_sum_h(float* __restrict input, float* __restrict output, int num_channels, int height, int width) {
    for (int c = 0; c < num_channels; ++c) {
        for (int w = 0; w < width; ++w) {
            float sum = 0.0f;
            for (int h = 0; h < height; ++h) {
                sum += input[c * height * width + h * width + w];
            }
            output[c * width + w] = sum;
        }
    }
}