#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

#include "Eigen/Dense"
#include "aisdk/base/type.h"
namespace aisdk::algorithm {
constexpr int kAlgoKeypointNum = 21;
enum class HandGesture { Invalid = 0, Click, Pinch, Grab, ThumbUp, OpenHand, Victory, Call, Home, MaxNum };
struct SingleHandData {
    std::vector<Vec3f_t> kpt3d;
    std::vector<Eigen::Matrix3f> rotation;
    Vec3f_t root_v{0, 0, 0};
    float score = 0;
    HandGesture gesture = HandGesture::Invalid;
};

}  // namespace aisdk::algorithm