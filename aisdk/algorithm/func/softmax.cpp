/*
 * @Author: jszhang jszhang@nreal.ai
 * @Date: 2023-03-02 08:07:08
 * @LastEditors: jszhang jszhang@nreal.ai
 * @LastEditTime: 2023-03-02 08:19:34
 */
#include "softmax.h"

#include <float.h>
#include <math.h>

#include <array>

#include "aisdk/base/log.h"
#include "aisdk/xengine/cv/xr_cv.h"

void softmax_single_lane_inplace(float* input, float* output, int size) {
    float max = -FLT_MAX;
    for (int i = 0; i < size; i++) {
        max = std::max(max, input[i]);
    }

    float sum = 0.f;
    for (int i = 0; i < size; i++) {
        output[i] = static_cast<float>(exp(input[i] - max));
        sum += output[i];
    }
    AISDK_LOG_TRACE("softmax sum is {}", sum);
    if (std::isnan(sum)) {
        for (int i = 0; i < size; i++) {
            AISDK_LOG_TRACE("nan raw output {}", input[i]);
        }
    }
    for (int i = 0; i < size; i++) {
        output[i] /= sum;
    }
}

void softmax_last_dim_naive(float* input, float* output, const std::array<int, 3>& dims) {
    int prior_dims = dims[0] * dims[1];
    int last_dim = dims[2];
    for (int prior_idx = 0; prior_idx < prior_dims; prior_idx++) {
        float* input_ptr = &input[prior_idx * last_dim];
        float* output_ptr = &output[prior_idx * last_dim];
        softmax_single_lane_inplace(input_ptr, output_ptr, last_dim);
    }
}

void softmax_last_dim(float* input, float* output, const std::array<int, 3>& dims) {
    //#if __aarch64__
    //    aisdk::xengine::softmax_last_dim_asm(input, output, dims);
    //#else
    softmax_last_dim_naive(input, output, dims);
    //#endif
}
