/*
 * @Author: jszhang jszhang@nreal.ai
 * @Date: 2023-03-10 07:53:29
 * @LastEditors: jszhang jszhang@nreal.ai
 * @LastEditTime: 2023-03-10 08:06:23
 * @FilePath: /nreal_hand_demo_android/src/core/netalgo/impl/functions/permute.cc
 */

#include "permute.h"

void NHWC2NCHW(const float *source, float *dest, int b, int c, int area) {
    int sourceBatchsize = c * area;
    int destBatchSize = sourceBatchsize;
    for (int bi = 0; bi < b; ++bi) {
        auto srcBatch = source + bi * sourceBatchsize;
        auto dstBatch = dest + bi * destBatchSize;
        for (int i = 0; i < area; ++i) {
            auto srcArea = srcBatch + i * c;
            auto dstArea = dstBatch + i;
            for (int ci = 0; ci < c; ++ci) {
                dstArea[ci * area] = srcArea[ci];
            }
        }
    }
}