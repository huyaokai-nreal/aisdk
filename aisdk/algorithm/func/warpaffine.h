#pragma once
#include <opencv2/opencv.hpp>
namespace aisdk::algorithm {

cv::Mat generate_roi_image(const cv::Mat& input_img, cv::Rect input_bbox, int output_width, int output_height);
}
