#pragma once

#include <vector>

#include "aisdk/algorithm/common/nrnet_define.h"

namespace aisdk::algorithm {

struct DetOutputInternal {
    // 内部使用, 长度为2包长度为1的DetectRect
    std::vector<std::vector<DetectRect>> images_lhand_rects;            // 左手 左目，右目
    std::vector<std::vector<DetectRect>> images_rhand_rects;            // 右手 左目，右目

    // 外部使用, 左右手x左右目 全部是单独的DetectRect
    DetectRect lhand_lcam_rect;
    DetectRect lhand_rcam_rect;
    DetectRect rhand_lcam_rect;
    DetectRect rhand_rcam_rect;

    bool lhand_lcam_valid = false;
    bool lhand_rcam_valid = false;
    bool rhand_lcam_valid = false;
    bool rhand_rcam_valid = false;

    bool det_flag = true;  // det or track

    void clear() {
        lhand_lcam_valid = false;
        lhand_rcam_valid = false;
        rhand_lcam_valid = false;
        rhand_rcam_valid = false;
    }
};

}  // namespace aisdk::algorithm
