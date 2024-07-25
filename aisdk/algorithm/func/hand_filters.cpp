#include "hand_filters.h"

#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/base/log.h"

namespace aisdk::algorithm {

bool HandFilters::init() {
    float freq = 60.;

    OneEuroParams palm_params, finger_params;

    palm_params.freq = freq;
    finger_params.freq = freq;
    if (glasses_type_ == "flora") {
        // 手掌
        palm_params.mincutoff = {0.4, 0.4, 0.2};  // 调静止状态下的稳定性,越小稳定性越好
        palm_params.beta = {25.0, 25.0, 10.0};    // 运动状态下alpha的变化速率，alpha越大，跟踪越及时
        palm_params.dcutoff = {2.0, 2.0, 1.0};    // 速度滤波的固定效果

        // 手指
        finger_params.mincutoff = {0.4, 0.4, 0.2};
        finger_params.beta = {25.0, 25.0, 10.0};
        finger_params.dcutoff = {2.0, 2.0, 1.0};
    } else if (glasses_type_ == "ella") {
        //  手掌
        palm_params.mincutoff = {0.2, 0.2, 0.1};  // 调静止状态下的稳定性,越小稳定性越好
        palm_params.beta = {15.0, 15.0, 10.0};    // 运动状态下alpha的变化速率，alpha越大，跟踪越及时
        palm_params.dcutoff = {2.0, 2.0, 1.0};    // 速度滤波的固定效果

        // 手指
        finger_params.mincutoff = {0.2, 0.2, 0.1};
        finger_params.beta = {15.0, 15.0, 10.0};
        finger_params.dcutoff = {2.0, 2.0, 1.0};
    } else {
        AISDK_LOG_ERROR("[Handfilters]: undedfined glasses type {}", glasses_type_);
    }

    // finger
    m_seq3d_lhand = std::make_shared<SeqManager3D>(15, finger_params);
    m_seq3d_rhand = std::make_shared<SeqManager3D>(15, finger_params);
    // palm
    m_seq3d_palm_lhand = std::make_shared<SeqManager3D>(PalmKeypointNum, palm_params);
    m_seq3d_palm_rhand = std::make_shared<SeqManager3D>(PalmKeypointNum, palm_params);

    return true;
}

void HandFilters::kpt_seq_3d_filter(int hand_side, std::vector<Vec3f_t>& point3d) {
    std::vector<Vec3f_t> rel_points, palm_points;
    const auto& root_point = point3d[kKeypointRootId];
    for (int i = 0; i < point3d.size(); i++) {
        if (i == kKeypointRootId) {
            continue;
        }
        if (kPalmKeypointIndexSet.find(i) != kPalmKeypointIndexSet.end()) {
            palm_points.emplace_back(point3d[i] - root_point);
        } else {
            rel_points.emplace_back(point3d[i] - root_point);
        }
    }
    if (hand_side == 0) {
        m_seq3d_lhand->getFilterHandData(rel_points);
        m_seq3d_palm_lhand->getFilterHandData(palm_points);
    } else {
        m_seq3d_rhand->getFilterHandData(rel_points);
        m_seq3d_palm_rhand->getFilterHandData(palm_points);
    }

    for (int i = 0, p = 0, q = 0; i < point3d.size(); i++) {
        if (i == kKeypointRootId)
            point3d[i] = root_point;
        else if (kPalmKeypointIndexSet.find(i) != kPalmKeypointIndexSet.end()) {
            point3d[i] = root_point + palm_points[q];
            q++;
        } else {
            point3d[i] = root_point + rel_points[p];
            p++;
        }
    }
};

bool HandFilters::reset(int hand_side) {
    if (hand_side == 0) {
        m_seq3d_lhand->reset();
        m_seq3d_palm_lhand->reset();
    } else {
        m_seq3d_rhand->reset();
        m_seq3d_palm_rhand->reset();
    }
    return true;
}

}  // namespace aisdk::algorithm
