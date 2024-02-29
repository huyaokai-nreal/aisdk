#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

#include "Eigen/Dense"

namespace aisdk::algorithm {

struct StandardKpt3dInternal {
    // Hand 3d standard output data, a single hand data size should be 23.
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

}  // namespace aisdk::algorithm
