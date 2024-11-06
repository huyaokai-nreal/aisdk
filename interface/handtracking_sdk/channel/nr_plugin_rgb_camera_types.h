#pragma once

#include "nr_plugin_types.h"

#ifdef NRSDK

#include "nr_plugin_rgb_camera_types.inc"

#else

NR_PLUGIN_ENUM8(NRRgbCameraPixelFormat) {
    NR_RGB_CAMERA_PIXEL_FORMAT_UNKNOWN = 0,
    NR_RGB_CAMERA_PIXEL_FORMAT_YUV_420_888,
    NR_RGB_CAMERA_PIXEL_FORMAT_RGB_888,
};


#pragma pack(1)
typedef struct NRRgbCameraFrameData {
    union {
        struct {
            uint8_t * data;
            uint32_t data_bytes;
            uint32_t width;
            uint32_t height;
            uint32_t stride;
            uint64_t exposure_start_time_device;
            uint32_t exposure_duration;
            uint32_t rolling_shutter_time;
            uint32_t gain;
            uint32_t frame_id;
            NRRgbCameraPixelFormat pixel_format;
            uint64_t exposure_start_time_system;
        };
        uint8_t padding[96];
    };

} NRRgbCameraFrameData;

#pragma pack()

#endif // NRSDK