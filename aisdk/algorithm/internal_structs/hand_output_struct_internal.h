#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "Eigen/Dense"

namespace aisdk::algorithm {

struct HandOutputInternal {
    uint64_t timestamp;

    std::vector<cv::Vec3f> lhand_kpt;
    std::vector<cv::Vec3f> rhand_kpt;

    std::vector<Eigen::Matrix3f> lhand_rot;
    std::vector<Eigen::Matrix3f> rhand_rot;

    std::string lhand_gesture = "Invalid";
    std::string rhand_gesture = "Invalid";

    float lhand_score = 0.0;
    float rhand_score = 0.0;

    bool lhand_valid = false;
    bool rhand_valid = false;

    void clear() {
        lhand_kpt.clear();
        rhand_kpt.clear();

        lhand_rot.clear();
        rhand_rot.clear();

        lhand_valid = false;
        rhand_valid = false;

        lhand_score = 0.0;
        rhand_score = 0.0;
    }

    bool isValid() const {
        if (lhand_valid && lhand_kpt.size() != 23 && lhand_rot.size() != 23) {
            return false;
        }
        if (rhand_valid && rhand_kpt.size() != 23 && rhand_rot.size() != 23) {
            return false;
        }
        return true;
    }
};

}  // namespace aisdk::algorithm
