/*
 * @Author: Zhang Junsong
 * @Date: 2023-02-28 06:58:34
 * @Last Modified by: Zhang Junsong
 * @Last Modified time: 2023-02-28 07:10:00
 */
#pragma once

#include <vector>

void softmax_last_dim(float* input, float* output, const std::vector<int> dims);
