#pragma once

#include <algorithm>
#include <vector>

#include "aisdk/xengine/nrhal_common.h"
#include "../core/nrnet_define.h"

namespace aisdk::algorithm {

struct GridAnchor {
    float grid_x;
    float grid_y;
    float anchor_rw;
    float anchor_rh;
};

cv::Rect add_bbox_margin(int left, int top, int right, int bottom, int max_w, int max_h);
void nms(std::vector<DetectRect> &rect, float iou_threshold);
aisdk::xengine::TensorFormat checkshapeformat(aisdk::xengine::VendorType &vendor, uint32_t m_rank);
Eigen::Matrix3d from_two_vectors(const Eigen::Vector3d& src_vec, const Eigen::Vector3d& dst_vec);
std::tuple<Eigen::Matrix3d, Eigen::Matrix3d, float> get_rotations_for_standard_stereo(const Eigen::Matrix4d& T);
}  // namespace NrNet

