#pragma once
#include "public/nr_plugin_interface.h"
#include "public/nr_plugin_types.h"

NR_PLUGIN_ENUM(TrackingReason){
    TRACKING_REASON_NONE = 0,
    TRACKING_REASON_INITIALIZING = 1,
    TRACKING_REASON_EXCESSIVE_MOTION = 2,
    TRACKING_REASON_INSUFFICIENT_FEATURES = 3,
    TRACKING_REASON_RELOCALIZING = 4,
    TRACKING_REASON_LOW_LIGHT = 5,
};

#pragma pack(1)

typedef struct DevicePose {
    union {
        struct {
            NRTransform transform;
            uint64_t hmd_time_nanos;
            TrackingReason tracking_reason;
            NRVector3f linear_velocity;
            NRVector3f angular_velocity;
            NRVector3f acc_bias;
            NRVector3f gyro_bias;
        };
        uint8_t padding[128];
    };
} DevicePose;

#pragma pack()
