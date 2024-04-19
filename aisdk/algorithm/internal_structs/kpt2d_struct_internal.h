#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include "aisdk/algorithm/common/hand_define.h"
#include "Eigen/Dense"
#include "aisdk/base/type.h"


namespace aisdk::algorithm {
struct Kpt2dResult {
    std::vector<std::vector<Vec2f_t>> kpts;  // 单手 左目，右目
    std::vector<std::vector<float>> scores;
};
struct Kpt2dInternal {
    Kpt2dInternal() {
        lhand_lcam.resize(kKeypointNum);
        lhand_rcam.resize(kKeypointNum);
        rhand_lcam.resize(kKeypointNum);
        rhand_rcam.resize(kKeypointNum);
    }

    // Hand 2d output data, a single eand data size should be 21.
    std::vector<Vec2f_t> lhand_lcam;
    std::vector<Vec2f_t> lhand_rcam;
    std::vector<Vec2f_t> rhand_lcam;
    std::vector<Vec2f_t> rhand_rcam;

    bool lhand_valid = false;
    bool rhand_valid = false;

    void clear() {
        lhand_lcam.clear();
        lhand_rcam.clear();
        rhand_lcam.clear();
        rhand_rcam.clear();
        lhand_valid = false;
        rhand_valid = false;
    }
};

}  // namespace aisdk::algorithm
