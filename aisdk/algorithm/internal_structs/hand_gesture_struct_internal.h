#pragma once
#include <opencv2/opencv.hpp>
#include "Eigen/Dense"
#include "aisdk/algorithm/common/hand_define.h"
namespace aisdk::algorithm {
struct HandGestureInternal {
    HandGesture lhand_gesture =  HandGesture::Invalid;
    HandGesture rhand_gesture = HandGesture::Invalid;
};

}  // namespace aisdk::algorithm
