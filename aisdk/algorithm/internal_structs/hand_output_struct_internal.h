#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "Eigen/Dense"
#include "aisdk/base/type.h"

namespace aisdk::algorithm {

struct HandOutputInternal {
    double timestamp; // seconds

    std::vector<Vec3f_t> lhand_kpt;
    std::vector<Vec3f_t> rhand_kpt;
    
    Vec3f_t lhand_v{0,0,0};
    Vec3f_t rhand_v{0,0,0};


    std::vector<Eigen::Matrix3f> lhand_rotation;
    std::vector<Eigen::Matrix3f> rhand_rotation;

    std::string lhand_gesture = "Invalid";
    std::string rhand_gesture = "Invalid";

    float lhand_score = 0.0;
    float rhand_score = 0.0;

    bool lhand_valid = false;
    bool rhand_valid = false;

    void clear() {
        lhand_kpt.clear();
        rhand_kpt.clear();

        lhand_rotation.clear();
        rhand_rotation.clear();
        
        lhand_gesture = "Invalid";
        rhand_gesture = "Invalid";

        lhand_valid = false;
        rhand_valid = false;

        lhand_score = 0.0;
        rhand_score = 0.0;
    }

};

}  // namespace aisdk::algorithm
