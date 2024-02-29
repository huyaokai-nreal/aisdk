/*
 * @Author: jszhang jszhang@nreal.ai
 * @Date: 2023-03-01 07:09:10
 * @LastEditors: jszhang jszhang@nreal.ai
 * @LastEditTime: 2023-03-02 06:59:42
 */
#include "rsntiny_postprocess.h"

#include "elementwise_mul.h"
#include "reducesum.h"
#include "softmax.h"

/**
 * @description:
 * @param {float* restrict} input_hm: input heatmap, 1x32x32x32
 * @param {float* restrict} kpt_x_out: output x coords, normalized to 0-1, 1x21
 * @param {float* restrict} kpt_y_out: output y coords, normalized to 0-1, 1x21
 * @return {*}
 */
static std::vector<float> hm_softmax(1 * 21 * 32 * 32);
static std::vector<float> hm_reduce_col(1 * 21 * 32 * 1);
static std::vector<float> hm_reduce_row(1 * 21 * 1 * 32);
static std::vector<float> hm_reduce_col_row(1 * 21 * 32);
static std::vector<float> hm_reduce_row_col(1 * 21 * 32);

void rsntiny_postprocess(float* __restrict input_hm, float* __restrict kpt_x_out, float* __restrict kpt_y_out) {
    std::vector<float> mul_coeff = {0,    0.03125, 0.0625, 0.09375, 0.125, 0.15625, 0.1875, 0.21875,
                                    0.25, 0.28125, 0.3125, 0.34375, 0.375, 0.40625, 0.4375, 0.46875,
                                    0.5,  0.53125, 0.5625, 0.59375, 0.625, 0.65625, 0.6875, 0.71875,
                                    0.75, 0.78125, 0.8125, 0.84375, 0.875, 0.90625, 0.9375, 0.96875};

    softmax_last_dim(input_hm, hm_softmax.data(), {1, 21, 32 * 32});

    reduce_sum_h(hm_softmax.data(), hm_reduce_col.data(), 21, 32, 32);
    reduce_sum_w(hm_softmax.data(), hm_reduce_row.data(), 21, 32, 32);

    elementwise_mult_hm(hm_reduce_col.data(), mul_coeff.data(), hm_reduce_col_row.data(), 21, 21 * 32);
    elementwise_mult_hm(hm_reduce_row.data(), mul_coeff.data(), hm_reduce_row_col.data(), 21, 21 * 32);

    reduce_sum_w(hm_reduce_col_row.data(), kpt_x_out, 21, 1, 32);
    reduce_sum_h(hm_reduce_row_col.data(), kpt_y_out, 21, 32, 1);

    return;
}