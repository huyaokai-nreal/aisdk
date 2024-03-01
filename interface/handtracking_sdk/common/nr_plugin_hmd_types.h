#pragma once

#include "public/nr_plugin_types.h"

#ifdef NRSDK

#include "nr_plugin_hmd_types.inc"

#else

NR_PLUGIN_ENUM(NRCameraModel) {
    NR_CAMERA_MODEL_RADIAL = 1,
    NR_CAMERA_MODEL_FISHEYE = 2,
    NR_CAMERA_MODEL_FISHEYE624 = 3,
};


#pragma pack(1)
#define NR_DISTORTION_PARAMS_COUNT 16
typedef struct NRCameraDistortion {
    NRCameraModel camera_model;
    union {
        // case camera_model == NR_CAMERA_MODEL_RADIAL:
        struct {
            float radial_k1;
            float radial_k2;
            float radial_p1;
            float radial_p2;
            float radial_k3;
        };
        // case camera_model == NR_CAMERA_MODEL_FISHEYE:
        // case camera_model == NR_CAMERA_MODEL_FISHEYE624:
        struct{
            float fisheye_k1;
            float fisheye_k2;
            float fisheye_k3;
            float fisheye_k4;
            float fisheye_k5;
            float fisheye_k6;
            float fisheye_p1;
            float fisheye_p2;
            float fisheye_s1;
            float fisheye_s2;
            float fisheye_s3;
            float fisheye_s4;
        };
        float camera_distortion[NR_DISTORTION_PARAMS_COUNT];
    };
} NRCameraDistortion;

#pragma pack()

#endif // NRSDK