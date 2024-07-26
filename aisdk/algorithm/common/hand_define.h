#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

#include "Eigen/Dense"
#include "aisdk/algorithm/common/nrcore_define.h"
#include "aisdk/base/type.h"
namespace aisdk::algorithm {
constexpr int kAlgoKeypointNum = 21;
#if defined(ENABLE_OPENXR_HANDJOINT_FORMAT)
constexpr int k3DAlgoStdKeypointNum = 26;
#else
constexpr int k3DAlgoStdKeypointNum = 23;
#endif
constexpr int kKeypointRootId = 21;
#if defined(ENABLE_OPENXR_HANDJOINT_FORMAT)
const std::set<int> kPalmKeypointIndexSet {0,1,5,9,13,17,22,23,24,25};
#else
const std::set<int> kPalmKeypointIndexSet {0, 1, 5, 9, 13, 17, 22};
#endif
enum class HandGesture { Invalid = 0, Pinch, Click, Grab, ThumbUp, OpenHand, Victory, Call, Home, MaxNum };
const std::vector<std::string> HandGestureNames{"Invalid",  "Pinch", "Click", "Grab", "ThumbUp", "OpenHand", "Victory", "Call", "Home"};
struct SingleHandData {
    std::vector<Vec3f_t> kpt3d;
    std::vector<Eigen::Matrix3f> rotation;
    Vec3f_t root_v{0, 0, 0};
    float score = 0;
    HandGesture gesture = HandGesture::Invalid;
    CamType source = CamType::UNKNOWN;
};

}  // namespace aisdk::algorithm