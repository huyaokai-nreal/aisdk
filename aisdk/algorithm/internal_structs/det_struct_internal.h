#pragma once

#include <vector>

#include "aisdk/algorithm/common/nrnet_define.h"

namespace aisdk::algorithm {

struct DetOutputInternal {
    // 内部使用, 长度为2包长度为1的DetectRect
    std::vector<std::vector<DetectRect>> images_lhand_rects;            // 左手 左目，右目
    std::vector<std::vector<DetectRect>> images_rhand_rects;            // 右手 左目，右目

    // 外部使用, 长度为2的DetectRect
    std::vector<DetectRect> lhand_rects;                                // 左手 左目，右目
    std::vector<DetectRect> rhand_rects;                                // 右手 左目，右目

    bool lhand_valid = false;
    bool rhand_valid = false;

    bool det_flag = true;  // det or track

    void clear() {
        lhand_valid = false;
        rhand_valid = false;
    }
};

}  // namespace aisdk::algorithm
