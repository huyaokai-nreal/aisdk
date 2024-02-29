#pragma once

namespace aisdk::algorithm {

struct HandStateInternal {
    bool lhand_valid = false;
    bool rhand_valid = false;

    void clear() {
        lhand_valid = false;
        rhand_valid = false;
    }
};

}  // namespace aisdk::algorithm
