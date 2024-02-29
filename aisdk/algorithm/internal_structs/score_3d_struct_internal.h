#pragma once

namespace aisdk::algorithm {

struct Score3dInternal {
    float lhand_score = 0.0;
    float rhand_score = 0.0;

    void clear() {
        lhand_score = 0.0;
        rhand_score = 0.0;
    }
};

}  // namespace aisdk::algorithm
