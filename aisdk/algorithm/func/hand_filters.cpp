#include "hand_filters.h"

#include <vector>

#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/base/log.h"
#include "aisdk/base/type.h"

namespace aisdk::algorithm {

bool HandFilters::init() {
    float freq = 60.;

    OneEuroParams palm_params, other_finger_params, index_finger_params;

    palm_params.freq = freq;
    index_finger_params.freq = freq;
    other_finger_params.freq = freq;
    if (glasses_type_ == "flora") {
        // 手掌
        palm_params.mincutoff = {0.4, 0.4, 0.2};  // 调静止状态下的稳定性,越小稳定性越好
        palm_params.beta = {25.0, 25.0, 10.0};    // 运动状态下alpha的变化速率，alpha越大，跟踪越及时
        palm_params.dcutoff = {2.0, 2.0, 1.0};    // 速度滤波的固定效果

        // 手指
        other_finger_params.mincutoff = {0.4, 0.4, 0.2};
        other_finger_params.beta = {25.0, 25.0, 10.0};
        other_finger_params.dcutoff = {2.0, 2.0, 1.0};
        // 食指的滤波参数， 要稳定一些
        index_finger_params.mincutoff = {0.2, 0.2, 0.1};
        index_finger_params.beta = {20, 20, 10};
        index_finger_params.dcutoff = {2.0, 2.0, 1.0};
    } else if (glasses_type_ == "ella") {
        //  手掌
        palm_params.mincutoff = {0.2, 0.2, 0.1};  // 调静止状态下的稳定性,越小稳定性越好
        palm_params.beta = {15.0, 15.0, 10.0};    // 运动状态下alpha的变化速率，alpha越大，跟踪越及时
        palm_params.dcutoff = {2.0, 2.0, 1.0};    // 速度滤波的固定效果

        // 其他手指
        other_finger_params.mincutoff = {0.2, 0.2, 0.1};
        other_finger_params.beta = {15.0, 15.0, 10.0};
        other_finger_params.dcutoff = {2.0, 2.0, 1.0};
        // 食指
        index_finger_params.mincutoff = {0.2, 0.2, 0.1};
        index_finger_params.beta = {15.0, 15.0, 10.0};
        index_finger_params.dcutoff = {2.0, 2.0, 1.0};
    } else {
        AISDK_LOG_ERROR("[Handfilters]: undedfined glasses type {}", glasses_type_);
    }
    set_filter_param(palm_params, index_finger_params, other_finger_params);
    return true;
}

std::vector<Vec3f_t> HandFilters::process(int hand_side, const std::vector<Vec3f_t>& point3d) {
    std::vector<Vec3f_t> rel_points, palm_points, index_points;
    const auto& root_point = point3d[kKeypointRootId];
    for (int i = 0; i < point3d.size(); i++) {
        if (i == kKeypointRootId) {
            continue;
        }
        if (kPalmKeypointIndexSet.find(i) != kPalmKeypointIndexSet.end()) {
            palm_points.emplace_back(point3d[i] - root_point);
        } else if (kIndexFingerIndexSet.find(i) != kIndexFingerIndexSet.end()) {
            index_points.emplace_back(point3d[i] - root_point);

        } else {
            rel_points.emplace_back(point3d[i] - root_point);
        }
    }
    if (hand_side == 0) {
        m_seq3d_lindex->getFilterHandData(index_points);
        m_seq3d_lhand->getFilterHandData(rel_points);
        m_seq3d_palm_lhand->getFilterHandData(palm_points);
    } else {
        m_seq3d_rindex->getFilterHandData(index_points);
        m_seq3d_rhand->getFilterHandData(rel_points);
        m_seq3d_palm_rhand->getFilterHandData(palm_points);
    }
    std::vector<Vec3f_t> result(point3d.size());

    for (int i = 0, p = 0, q = 0, k = 0; i < point3d.size(); i++) {
        if (i == kKeypointRootId) {
            result[i] = root_point;
        } else if (kPalmKeypointIndexSet.find(i) != kPalmKeypointIndexSet.end()) {
            result[i] = root_point + palm_points[q];
            q++;
        } else if (kIndexFingerIndexSet.find(i) != kIndexFingerIndexSet.end()) {
            result[i] = root_point + index_points[k];
            k++;
        } else {
            result[i] = root_point + rel_points[p];
            p++;
        }
    }
    return result;
};

bool HandFilters::reset(int hand_side) {
    if (hand_side == 0) {
        m_seq3d_lhand->reset();
        m_seq3d_palm_lhand->reset();
        m_seq3d_lindex->reset();
    } else {
        m_seq3d_rhand->reset();
        m_seq3d_palm_rhand->reset();
        m_seq3d_rindex->reset();
    }
    return true;
}

}  // namespace aisdk::algorithm
