#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

#include "Eigen/Dense"

namespace aisdk::algorithm {

struct NimbleInternal {
    // Hand 3d output data, a single hand data size should be 21.
    std::vector<Eigen::Matrix<float, 1, 9>> lhand_angle;
    std::vector<float> lhand_shape;
    std::vector<cv::Vec3f> lhand_trans;
    std::vector<cv::Vec3f> lhand;

    std::vector<Eigen::Matrix<float, 1, 9>> rhand_angle;
    std::vector<float> rhand_shape;
    std::vector<cv::Vec3f> rhand_trans;
    std::vector<cv::Vec3f> rhand;

    bool lhand_valid = false;
    bool rhand_valid = false;

    void clear() {
        lhand_angle.clear();
        lhand_shape.clear();
        lhand_trans.clear();
        lhand.clear();
        
        rhand_angle.clear();
        rhand_shape.clear();
        rhand_trans.clear();
        rhand.clear();

        lhand_valid = false;
        rhand_valid = false;
    }
};

}  // namespace aisdk::algorithm
