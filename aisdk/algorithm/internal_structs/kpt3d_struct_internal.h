#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

#include "Eigen/Dense"
#include "aisdk/base/type.h"

namespace aisdk::algorithm {

struct Kpt3dInternal {
    // Hand 3d output data, a single hand data size should be 21.
    std::vector<Vec3f_t> lhand_kpt;
    std::vector<Vec3f_t> rhand_kpt;
    std::vector<Eigen::Matrix3f> lhand_rotation;
    std::vector<Eigen::Matrix3f> rhand_rotation;
    Vec3f_t lhand_v{0, 0, 0};
    Vec3f_t rhand_v{0, 0, 0};
    float lhand_score = 0;
    float rhand_score = 0;
    bool lhand_valid = false;
    bool rhand_valid = false;

    void clear() {
        lhand_kpt.clear();
        rhand_kpt.clear();
        lhand_rotation.clear();
        rhand_rotation.clear();
        lhand_valid = false;
        rhand_valid = false;
    }
};
struct LiftNetInputs {
    std::vector<Vec2f_t> input_kpt_lcam;  // 单手 左目
    std::vector<Vec2f_t> input_kpt_rcam;  // 单手 右目
    double timestamp;
    float is_left;
};

struct LiftNetOutputs {
    std::vector<Vec3f_t> res3d;
};

}  // namespace aisdk::algorithm
