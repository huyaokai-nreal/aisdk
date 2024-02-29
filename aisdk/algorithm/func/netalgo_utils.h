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
//简单的检查shape是那种分布
bool checkshapeformat(uint32_t rank, std::vector<uint32_t> &reality, std::vector<uint32_t> &expect);
aisdk::xengine::TensorFormat checkshapeformat(aisdk::xengine::VendorType &vendor, uint32_t m_rank, bool in_or_out);
Eigen::Matrix3f from_two_vectors(const Eigen::Vector3f& src_vec, const Eigen::Vector3f& dst_vec);
std::tuple<Eigen::Matrix3f, Eigen::Matrix3f, float> get_rotations_for_standard_stereo(Eigen::Matrix4f T);
}  // namespace NrNet

