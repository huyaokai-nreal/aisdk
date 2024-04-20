#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

#include "Eigen/Dense"

namespace aisdk::algorithm {

struct Kpt3dInternal {
    // Hand 3d output data, a single hand data size should be 21.
    std::vector<cv::Vec3f> lhand;
    std::vector<cv::Vec3f> rhand;
    float lscore = 0;
    float rscore = 0;

    bool lhand_valid = false;
    bool rhand_valid = false;

    void clear() {
        lhand.clear();
        rhand.clear();

        lhand_valid = false;
        rhand_valid = false;
    }
};

}  // namespace aisdk::algorithm
