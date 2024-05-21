#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include "aisdk/algorithm/common/hand_define.h"
#include "Eigen/Dense"
#include "aisdk/base/type.h"

namespace aisdk::algorithm {
struct HandsData {
    SingleHandData left_hand;
    SingleHandData right_hand;
    bool lhand_valid = false;
    bool rhand_valid = false;
};
struct LiftNetInputs {
    std::vector<Vec2f_t> input_kpt_lcam;  // 单手 左目
    std::vector<Vec2f_t> input_kpt_rcam;  // 单手 右目
    double timestamp;
    float is_left;
};

struct LiftNetOutputs {
    std::vector<Vec3f_t> res3d;
    float kpt3d_score = 0;
};

}  // namespace aisdk::algorithm
