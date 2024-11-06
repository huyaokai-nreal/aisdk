#pragma once

#include "nr_plugin_types.h"

#ifdef NRSDK

#include "nr_plugin_grayscale_camera_types.inc"

#else

NR_PLUGIN_ENUM8(NRGrayscaleCameraPixelFormat) {
    NR_GRAYSCALE_CAMERA_PIXEL_FORMAT_UNKNOWN = 0,
    NR_GRAYSCALE_CAMERA_PIXEL_FORMAT_YUV_420_888,
    NR_GRAYSCALE_CAMERA_PIXEL_FORMAT_RGB_BAYER_8BPP,
};


NR_PLUGIN_ENUM8(NRGrayscaleCameraID) {
    NR_GRAYSCALE_CAMERA_ID_0 = 0x0001,
    NR_GRAYSCALE_CAMERA_ID_1 = 0x0002,
    NR_GRAYSCALE_CAMERA_ID_2 = 0x0004,
    NR_GRAYSCALE_CAMERA_ID_3 = 0x0008,
};


#pragma pack(1)
typedef struct NRGrayscaleCameraUnitData {
    union {
        struct {
            uint32_t offset;
            NRGrayscaleCameraID camera_id;
            uint32_t width;
            uint32_t height;
            uint32_t stride;
            uint64_t exposure_start_time_device;
            uint32_t exposure_duration;
            uint32_t rolling_shutter_time;
            uint32_t gain;
            uint64_t exposure_start_time_system;
        };
        uint8_t padding[64];
    };

} NRGrayscaleCameraUnitData;

typedef struct NRGrayscaleCameraFrameData {
    union {
        struct {
            NRGrayscaleCameraUnitData cameras[4];
            uint8_t * data;
            uint32_t data_bytes;
            uint8_t camera_count;
            uint32_t frame_id;
            NRGrayscaleCameraPixelFormat pixel_format;
        };
        uint8_t padding[320];
    };

} NRGrayscaleCameraFrameData;

#pragma pack()

#endif // NRSDK