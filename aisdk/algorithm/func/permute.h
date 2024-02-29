/*
 * @Author: jszhang jszhang@nreal.ai
 * @Date: 2023-03-10 07:52:16
 * @LastEditors: jszhang jszhang@nreal.ai
 * @LastEditTime: 2023-03-10 08:06:09
 * @FilePath: /nreal_hand_demo_android/src/core/netalgo/impl/functions/permute.h
 */
#pragma once

void NHWC2NCHW(const float *source, float *dest, int b, int c, int area);
