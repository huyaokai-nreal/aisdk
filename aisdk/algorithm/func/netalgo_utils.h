#pragma once

#include <vector>

#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/base/type.h"

namespace aisdk::algorithm {

void nms(std::vector<DetectRect> &rect, float iou_threshold);

aisdk::xengine::TensorFormat checkshapeformat(aisdk::xengine::VendorType &vendor, uint32_t m_rank);
std::vector<Vec3f_t> constrain_hand(const std::vector<Vec3f_t>& input_kpt3d, bool is_left);
std::tuple<Eigen::Matrix3d, Eigen::Matrix3d, float> get_rotations_for_standard_stereo(const Eigen::Matrix4d &T);
std::vector<Vec3f_t> convert_to_23points(const std::vector<Vec3f_t>& input);
std::vector<Vec3f_t> convert_to_26points(const std::vector<Vec3f_t>& input);
}  // namespace aisdk::algorithm
