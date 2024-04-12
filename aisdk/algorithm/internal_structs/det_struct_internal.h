#pragma once

#include <vector>

#include "aisdk/algorithm/common/nrnet_define.h"

namespace aisdk::algorithm {

struct DetOutputInternal {
    std::vector<std::vector<cv::Rect>> images_lhand_rects;                            // 左手 左目，右目
    std::vector<std::vector<cv::Rect>> images_rhand_rects;                            // 右手 左目，右目
    std::vector<std::vector<aisdk::algorithm::DetectRect>> images_lhand_model_rects;  // 左手 左目，右目
    std::vector<std::vector<aisdk::algorithm::DetectRect>> images_rhand_model_rects;  // 右手 左目，右目

    // std::vector<float> lcam_lhand;
    // std::vector<float> lcam_rhand;
    // std::vector<float> rcam_lhand;
    // std::vector<float> rcam_rhand;

    bool lhand_valid = false;
    bool rhand_valid = false;

    void clear() {
        // lcam_lhand.clear();
        // lcam_rhand.clear();
        // rcam_lhand.clear();
        // rcam_rhand.clear();

        lhand_valid = false;
        rhand_valid = false;
    }

    // bool isValid() const {
    //     if (lhand_valid) {
    //         return false;
    //     }
    //     if (rhand_valid && (lcam_rhand.size() != 4 || rcam_rhand.size() != 4)) {
    //         return false;
    //     }
    //     return true;
    // }
};

}  // namespace aisdk::aisdk::algorithm
