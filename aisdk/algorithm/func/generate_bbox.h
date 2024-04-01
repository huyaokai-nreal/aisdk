/*
 * @Author: Zhang Junsong
 * @Date: 2023-02-28 08:50:42
 * @Last Modified by: Zhang Junsong
 * @Last Modified time: 2023-02-28 08:51:10
 */
#pragma once

#include <mutex>
#include <opencv2/opencv.hpp>
#include <thread>

cv::Rect generate_bbox(int max_width, int max_height, std::vector<std::vector<float>> kps, std::vector<float>& bbox_f);
cv::Rect generate_bbox(int max_width, int max_height, std::vector<cv::Vec2f> kps);
