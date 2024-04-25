#pragma once
#include <opencv2/opencv.hpp>
#include "Eigen/Dense"
namespace aisdk::algorithm {
struct HandGestureInternal {
    std::string lhand_gesture = "Invalid";
    std::string rhand_gesture = "Invalid";
};

}  // namespace aisdk::algorithm
