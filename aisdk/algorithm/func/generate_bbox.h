#pragma once
#include <opencv2/core/types.hpp>
#include "aisdk/base/type.h"
namespace aisdk::algorithm {
void bbox_to_center_and_scale(float* bbox, float* center, float* scale);
void center_scale_to_bbox(float* bbox, float* center, float* scale);
void adjust_bbox(float* bbox, float height, float width, float* bbox_res, float* center, float* scale);
cv::Rect generate_bbox(int max_width, int max_height, std::vector<std::vector<float>> kps, std::vector<float>& bbox_f);
cv::Rect generate_bbox(int max_width, int max_height, std::vector<aisdk::Vec2f_t> kps);

}
