#pragma once

#include <mutex>
#include <thread>

#include <opencv2/opencv.hpp>

cv::Mat generate_roi_image(const cv::Mat& input_img, std::vector<float> input_bbox);

cv::Mat generate_roi_image(const cv::Mat& input_img, cv::Rect input_bbox);
