#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

#include "Eigen/Dense"

#define KPT_NUMS 21

namespace aisdk::algorithm {

struct Kpt2dInternal {
    Kpt2dInternal() {
        lhand_lcam.resize(KPT_NUMS);
        lhand_rcam.resize(KPT_NUMS);
        rhand_lcam.resize(KPT_NUMS);
        rhand_rcam.resize(KPT_NUMS);
    }

    // Hand 2d output data, a single eand data size should be 21.
    std::vector<cv::Vec2f> lhand_lcam;
    std::vector<cv::Vec2f> lhand_rcam;
    std::vector<cv::Vec2f> rhand_lcam;
    std::vector<cv::Vec2f> rhand_rcam;

    bool lhand_valid = false;
    bool rhand_valid = false;

    void clear() {
        // lhand_lcam.clear();
        // lhand_rcam.clear();
        // rhand_lcam.clear();
        // rhand_rcam.clear();

        lhand_valid = false;
        rhand_valid = false;
    }
};

}  // namespace aisdk::algorithm
