#include "hand_filters.h"

#define EZXR_DEFINED_JOINTS 23

bool HandFilters::init() {
    float freq = 60.;

    aisdk::algorithm::OneEuroParams center_params, palm_params, finger_params;

    center_params.freq = freq;
    palm_params.freq = freq;
    finger_params.freq = freq;

    // 根节点
    center_params.mincutoff = {0.1, 0.1, 0.1};  // 调静止状态下的稳定性,越小稳定性越好
    center_params.beta = {10.0, 10.0, 10.0};    // 运动状态下alpha的变化速率，alpha越大，跟踪越及时
    center_params.dcutoff = {0.8, 0.8, 0.5};    // 速度滤波的固定效果

    // 手掌
    palm_params.mincutoff = {0.4, 0.4, 0.1};  // 调静止状态下的稳定性,越小稳定性越好
    palm_params.beta = {20.0, 20.0, 10.0};    // 运动状态下alpha的变化速率，alpha越大，跟踪越及时
    palm_params.dcutoff = {2.0, 2.0, 1.0};    // 速度滤波的固定效果

    // 手指
    finger_params.mincutoff = {0.4, 0.4, 0.1};
    finger_params.beta = {20.0, 20.0, 10.0};
    finger_params.dcutoff = {2.0, 2.0, 1.0};

    // finger
    m_seq3d_lhand = std::make_shared<aisdk::algorithm::SeqManager3D>(16, finger_params);
    m_seq3d_rhand = std::make_shared<aisdk::algorithm::SeqManager3D>(16, finger_params);
    // palm
    m_seq3d_palm_lhand = std::make_shared<aisdk::algorithm::SeqManager3D>(6, palm_params);
    m_seq3d_palm_rhand = std::make_shared<aisdk::algorithm::SeqManager3D>(6, palm_params);
    // center
    m_seq3d_center_lhand = std::make_shared<aisdk::algorithm::SeqManager3D>(1, center_params);
    m_seq3d_center_rhand = std::make_shared<aisdk::algorithm::SeqManager3D>(1, center_params);

    return true;
}

void HandFilters::kpt_seq_3d_filter(int hand_side, std::vector<cv::Vec3f>& point3d) {
    std::vector<cv::Vec3f> rel_points, palm_points, root_point;

    for (int i = 0; i < EZXR_DEFINED_JOINTS; i++) {
        if (i == 21) continue;
        if (i == 0 || i == 5 || i == 9 || i == 13 || i == 17 || i == 22) {
            palm_points.emplace_back(point3d[i] - point3d[21]);
        } else {
            rel_points.emplace_back(point3d[i] - point3d[21]);
        }
    }
    root_point.emplace_back(point3d[21]);
    if (hand_side == 0) {
        m_seq3d_lhand->getFilterHandData(rel_points);
        m_seq3d_palm_lhand->getFilterHandData(palm_points);
        m_seq3d_center_lhand->getFilterHandData(root_point);
    } else {
        m_seq3d_rhand->getFilterHandData(rel_points);
        m_seq3d_palm_rhand->getFilterHandData(palm_points);
        m_seq3d_center_rhand->getFilterHandData(root_point);
    }

    for (int i = 0, p = 0, q = 0; i < EZXR_DEFINED_JOINTS; i++) {
        if (i == 21)
            point3d[i] = root_point[0];
        else if (i == 0 || i == 5 || i == 9 || i == 13 || i == 17 || i == 22) {
            point3d[i] = root_point[0] + palm_points[q];
            q++;
        } else {
            point3d[i] = root_point[0] + rel_points[p];
            p++;
        }
    }
    return;
};
