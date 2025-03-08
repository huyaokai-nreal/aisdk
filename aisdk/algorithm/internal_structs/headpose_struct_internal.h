#pragma once

#include <vector>

#include "interface/handtracking_sdk/include/public/nr_plugin_types.h"

namespace aisdk::algorithm {

struct HeadPoseInternal {
    HeadPoseInternal() {
        transform.rotation.qw = 1.0;
        transform.rotation.qx = 0.0;
        transform.rotation.qy = 0.0;
        transform.rotation.qz = 0.0;

        transform.position.x = 0.0;
        transform.position.y = 0.0;
        transform.position.z = 0.0;
    }

    HeadPoseInternal(NRTransform init_transform) : transform(init_transform) {}

    NRTransform transform;

    void clear() {
        transform.rotation.qw = 1.0;
        transform.rotation.qx = 0.0;
        transform.rotation.qy = 0.0;
        transform.rotation.qz = 0.0;

        transform.position.x = 0.0;
        transform.position.y = 0.0;
        transform.position.z = 0.0;
    }
};

}  // namespace aisdk::algorithm
