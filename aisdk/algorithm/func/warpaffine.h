#pragma once
#include <opencv2/opencv.hpp>
#include "aisdk/base/type.h"
#include "aisdk/algorithm/common/nrnet_define.h"
namespace aisdk::algorithm {

cv::Mat generate_roi_image(const cv::Mat& input_img, const Vec4f_t&, int output_width, int output_height);
}
