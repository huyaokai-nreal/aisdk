#pragma once

#include <vector>

#include "aisdk/algorithm/common/nrnet_define.h"

namespace aisdk::algorithm {

cv::Rect add_bbox_margin(int left, int top, int right, int bottom, int max_w, int max_h);
void nms(std::vector<DetectRect> &rect, float iou_threshold);
aisdk::xengine::TensorFormat checkshapeformat(aisdk::xengine::VendorType &vendor, uint32_t m_rank);
std::vector<cv::Vec3f> constrain_hand(const std::vector<cv::Vec3f>& input_kpt3d, bool is_left);
std::tuple<Eigen::Matrix3d, Eigen::Matrix3d, float> get_rotations_for_standard_stereo(const Eigen::Matrix4d &T);
std::vector<cv::Vec3f> convert_to_23points(const std::vector<cv::Vec3f>& input);
}  // namespace aisdk::algorithm
