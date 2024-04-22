#include "hand_filters.h"

namespace aisdk::algorithm {

#define EZXR_DEFINED_JOINTS 23

bool HandFilters::init() {
    float freq = 60.;

    aisdk::algorithm::OneEuroParams center_params, palm_params, finger_params;

    center_params.freq = freq;
    palm_params.freq = freq;
    finger_params.freq = freq;
    // 手掌
    palm_params.mincutoff = {0.4, 0.4, 0.2};  // 调静止状态下的稳定性,越小稳定性越好
    palm_params.beta = {25.0, 25.0, 10.0};    // 运动状态下alpha的变化速率，alpha越大，跟踪越及时
    palm_params.dcutoff = {2.0, 2.0, 1.0};    // 速度滤波的固定效果

    // 手指
    finger_params.mincutoff = {0.4, 0.4, 0.2};
    finger_params.beta = {25.0, 25.0, 10.0};
    finger_params.dcutoff = {2.0, 2.0, 1.0};

    // finger
    m_seq3d_lhand = std::make_shared<aisdk::algorithm::SeqManager3D>(16, finger_params);
    m_seq3d_rhand = std::make_shared<aisdk::algorithm::SeqManager3D>(16, finger_params);
    // palm
    m_seq3d_palm_lhand = std::make_shared<aisdk::algorithm::SeqManager3D>(6, palm_params);
    m_seq3d_palm_rhand = std::make_shared<aisdk::algorithm::SeqManager3D>(6, palm_params);

    return true;
}

void HandFilters::kpt_seq_3d_filter(int hand_side, std::vector<Vec3f_t>& point3d) {
    std::vector<Vec3f_t> rel_points, palm_points;
    for (int i = 0; i < EZXR_DEFINED_JOINTS; i++) {
        if (i == 21) continue;
        if (i == 0 || i == 5 || i == 9 || i == 13 || i == 17 || i == 22) {
            palm_points.emplace_back(point3d[i] - point3d[21]);
        } else {
            rel_points.emplace_back(point3d[i] - point3d[21]);
        }
    }
    const auto& root_point = point3d[21];
    if (hand_side == 0) {
        m_seq3d_lhand->getFilterHandData(rel_points);
        m_seq3d_palm_lhand->getFilterHandData(palm_points);
    } else {
        m_seq3d_rhand->getFilterHandData(rel_points);
        m_seq3d_palm_rhand->getFilterHandData(palm_points);
    }

    for (int i = 0, p = 0, q = 0; i < EZXR_DEFINED_JOINTS; i++) {
        if (i == 21)
            point3d[i] = root_point;
        else if (i == 0 || i == 5 || i == 9 || i == 13 || i == 17 || i == 22) {
            point3d[i] = root_point + palm_points[q];
            q++;
        } else {
            point3d[i] = root_point + rel_points[p];
            p++;
        }
    }
};

}  // namespace aisdk::algorithm