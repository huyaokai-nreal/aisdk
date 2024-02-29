#pragma once
#include <vector>

#include "Eigen/Dense"
#include <opencv2/opencv.hpp>

struct Kpt3dInternal {
    // Hand 3d output data, a single hand data size should be 21.
    std::vector<cv::Vec3f> lhand;
    std::vector<cv::Vec3f> rhand;

    bool lhand_valid = false;
    bool rhand_valid = false;

    void clear() {
        lhand.clear();
        rhand.clear();

        lhand_valid = false;
        rhand_valid = false;
    }
};
