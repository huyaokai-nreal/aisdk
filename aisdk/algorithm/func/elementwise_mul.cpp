/*
 * @Author: jszhang jszhang@nreal.ai
 * @Date: 2023-03-01 06:45:31
 * @LastEditors: jszhang jszhang@nreal.ai
 * @LastEditTime: 2023-03-02 02:39:28
 */
#include "elementwise_mul.h"

/**
 * @description:
 * @param {float* restrict} A
 * @param {float* restrict} B
 * @param {float* restrict} C
 * @param {int} size

 */
void elementwise_mult(float* __restrict A, float* __restrict B, float* __restrict C, int size) {
    for (int i = 0; i < size; i++) {
        C[i] = A[i] * B[i];
    }
    return;
}

void elementwise_mult_hm(float* __restrict hm_input, float* __restrict coeffs, float* __restrict hm_output,
                         int channels, int size) {
    int col_size = size / channels;

    for (int c = 0; c < channels; c++) {
        elementwise_mult(hm_input + c * col_size, coeffs, hm_output + c * col_size, col_size);
    }
    return;
}