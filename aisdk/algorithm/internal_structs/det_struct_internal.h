#pragma once

#include <vector>

#include "aisdk/algorithm/common/nrnet_define.h"

namespace aisdk::algorithm {

struct DetOutputInternal {
    std::vector<std::vector<DetectRect>> images_lhand_rects;  // 左手 左目，右目
    std::vector<std::vector<DetectRect>> images_rhand_rects;  // 右手 左目，右目

    bool lhand_valid = false;
    bool rhand_valid = false;

    void clear() {
        lhand_valid = false;
        rhand_valid = false;
    }
};

}  // namespace aisdk::algorithm
