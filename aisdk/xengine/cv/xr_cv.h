#pragma once

#include <map>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

namespace aisdk::xengine {
#if __aarch64__
void Nrlog(const float *_x, float *y, int n);
void Gaussian(cv::Mat _src, cv::Mat _dst, cv::Size ksize, double sigma1);
// dst = (dst/alpha - mean)/norm
// dst must mode 8
void NrResize(unsigned char *src, float *dst, int drows, int dcols, float alpha, float mean, float norm);
void softmax_last_dim_asm(float *input, float *output, const std::array<int, 3>& dims);
void warpaffine_bilinear_c1(const unsigned char *src, int srcw, int srch, unsigned char *dst, int w, int h, double *tm, int type, unsigned int v);
#endif
typedef union Cv32suf {
    int i;
    unsigned u;
    float f;
} Cv32suf;

}  // namespace aisdk::xengine
