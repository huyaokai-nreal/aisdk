#include <array>

#include "aisdk/base/type.h"
#pragma once
namespace aisdk::algorithm {
struct ResultBase {
    bool lhand_valid = false;
    bool rhand_valid = false;

};
struct HandKeypoint2DResult:public ResultBase {
    std::array<Mat21_2f_t, 2> lhand_keypoints;
    std::array<Mat21_2f_t, 2> rhand_keypoints;
};

struct HandDetTrackResult: public ResultBase {
    struct DetectResult {
        Vec4f_t bbox;
        float score;
    };
    std::array<DetectResult, 2> lhand_bbox;
    std::array<DetectResult, 2> rhand_bbox;
};

}  // namespace aisdk::algorithm