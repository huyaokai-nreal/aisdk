#pragma once

#include "public/nr_plugin_types.h"

#ifdef NRSDK

#include "nr_plugin_grayscale_camera_types.inc"

#else

NR_PLUGIN_ENUM(NRGrayscaleCameraAutoExposureType) {
    NR_GRAYSCALE_CAMERA_AUTO_EXPOSURE_TYPE_OFF = 0,
    NR_GRAYSCALE_CAMERA_AUTO_EXPOSURE_TYPE_ISP,
    NR_GRAYSCALE_CAMERA_AUTO_EXPOSURE_TYPE_SOFTWARE,
};


NR_PLUGIN_ENUM(NRGrayscaleCameraProperty) {
    NR_GRAYSCALE_CAMERA_PROPERTY_AUTO_EXPOSURE_TYPE = 0,
    NR_GRAYSCALE_CAMERA_PROPERTY_EXPOSURE_TIME,
    NR_GRAYSCALE_CAMERA_PROPERTY_GAIN,
};


#pragma pack(1)
typedef struct NRGrayscaleCameraUnitData {
    union {
        struct {
            uint32_t offset;
            uint64_t hmd_time_nanos;
            uint32_t width;
            uint32_t height;
            uint32_t step;
            uint32_t exposure_time;
            uint32_t gain;
        };
        uint8_t padding[48];
    };

} NRGrayscaleCameraUnitData;

typedef struct NRGrayscaleCameraFrameData {
    union {
        struct {
            NRGrayscaleCameraUnitData cameras[4];
            const void * data;
            uint32_t data_bytes;
            uint8_t camera_count;
            uint8_t pixel_format;
            uint64_t timestamp_raw;
            uint8_t timer_bits;
            uint64_t hmd_hw_time_nanos;
        };
        uint8_t padding[256];
    };

} NRGrayscaleCameraFrameData;

#pragma pack()

#endif // NRSDK