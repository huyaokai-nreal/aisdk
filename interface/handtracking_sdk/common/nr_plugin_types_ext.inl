#pragma once

#ifdef NRSDK

#include "nr_plugin_types_ext.inc"

#else

NR_PLUGIN_ENUM(NRNetworkType) {
    NR_NETWORK_TYPE_SINGLE = 0,
    NR_NETWORK_TYPE_SERVER = 1,
    NR_NETWORK_TYPE_CLIENT = 2,
    NR_NETWORK_TYPE_QVR = 3,
};


NR_PLUGIN_ENUM(NRWorkingMode) {
    NR_WORKING_MODE_HEAVY = 0,
    NR_WORKING_MODE_LIGHT = 1,
};


NR_PLUGIN_ENUM(NRDeviceType) {
    NR_DEVICE_TYPE_INVALID = 0,
    NR_DEVICE_TYPE_LIGHT = 1,
    NR_DEVICE_TYPE_AIR = 2,
    NR_DEVICE_TYPE_P55E = 3,
    NR_DEVICE_TYPE_P55F = 4,
    NR_DEVICE_TYPE_FLORA = 5,
    NR_DEVICE_TYPE_GINA_FLORA = 10,
    NR_DEVICE_TYPE_GINA_M = 11,
    NR_DEVICE_TYPE_GINA_L = 12,
    NR_DEVICE_TYPE_HONOR_AIR = 1001,
};


NR_PLUGIN_ENUM(NRComponent) {
    NR_COMPONENT_INVALID = -1,
    NR_COMPONENT_DISPLAY_LEFT = 0,
    NR_COMPONENT_DISPLAY_RIGHT,
    NR_COMPONENT_RGB_CAMERA,
    NR_COMPONENT_GRAYSCALE_CAMERA_LEFT,
    NR_COMPONENT_GRAYSCALE_CAMERA_RIGHT,
    NR_COMPONENT_MAGNETIC,
    NR_COMPONENT_HEAD,
    NR_COMPONENT_IMU,
    NR_COMPONENT_NUM,
    NR_COMPONENT_DISPLAY_NUM = 2,
};


NR_PLUGIN_ENUM(NRChannelType) {
    NR_CHANNEL_TYPE_INVALID = -1,
    NR_CHANNEL_TYPE_MCU = 0,
    NR_CHANNEL_TYPE_RGB_CAMERA,
    NR_CHANNEL_TYPE_IMU,
    NR_CHANNEL_TYPE_GRAYSCALE_CAMERA,
    NR_CHANNEL_TYPE_GLASSES_CONFIG,
    NR_CHANNEL_TYPE_VSYNC,
    NR_CHANNEL_TYPE_DP,
    NR_CHANNEL_TYPE_EXTERNAL_SENSOR,
    NR_CHANNEL_TYPE_NUM,
};


NR_PLUGIN_ENUM64(NRChannelDataType) {
    NR_CHANNEL_DATA_TYPE_NONE = 0x00LL,
    NR_CHANNEL_DATA_TYPE_GLASSES_GRAYSCALE_CAMERA = 0x01LL,
    NR_CHANNEL_DATA_TYPE_GLASSES_IMU = 0x02LL,
    NR_CHANNEL_DATA_TYPE_ANDROID_SYSTEM_IMU = 0x04LL,
    NR_CHANNEL_DATA_TYPE_EXTERNAL_SENSOR = 0x08LL,
};


NR_PLUGIN_ENUM(NRLatencyType) {
    NR_LATENCY_TYPE_IMU_POSE = 0,
};


NR_PLUGIN_ENUM(NRTrackingPoseType) {
    NR_TRACKING_POSE_TYPE_NORMAL = 0,
    NR_TRACKING_POSE_TYPE_LOW_LATENCY = 1,
};


NR_PLUGIN_ENUM(NRDisplay2D3DMode) {
    NR_DISPLAY_MODE_UNKNOWN = 0,
    NR_DISPLAY_MODE_2D = 1,
    NR_DISPLAY_MODE_3D = 2,
};


NR_PLUGIN_ENUM(NRProperty) {
    NR_PROPERTY_UNKNOWN = 0,
    NR_PROPERTY_START = 1,
    NR_PROPERTY_LOADER = NR_PROPERTY_START,
    NR_PROPERTY_HARDWARE_TYPE,
    NR_PROPERTY_SINGLE_BUFFER,
    NR_PROPERTY_NUM,
};


NR_PLUGIN_ENUM(NRMetricsType) {
    NR_METRICS_TYPE_NULL = 0x00,
    NR_METRICS_TYPE_FRAME_PRESENT_COUNT = 0x01,
    NR_METRICS_TYPE_TEARED_FRAME_COUNT = 0x02,
    NR_METRICS_TYPE_EARLY_FRAME_COUNT = 0x04,
    NR_METRICS_TYPE_DROPPED_FRAME_COUNT = 0x08,
    NR_METRICS_TYPE_FRAME_COMPOSITE_TIME = 0x10,
    NR_METRICS_TYPE_MOTION_TO_PHOTON = 0x20,
    NR_METRICS_TYPE_EXTENDED_FRAME_COUNT = 0x40,
    NR_METRICS_TYPE_TW_FPS = 0x80,
};


NR_PLUGIN_ENUM(NRCameraModel) {
    NR_CAMERA_MODEL_RADIAL = 1,
    NR_CAMERA_MODEL_FISHEYE = 2,
    NR_CAMERA_MODEL_FISHEYE_RTTP = 3,
};


NR_PLUGIN_ENUM(NRAvailableValue) {
    NR_AVAILABLE_VALUE_NOT_AVAILABLE = 0,
    NR_AVAILABLE_VALUE_AVAILABLE = 1,
};


NR_PLUGIN_ENUM(NREnableValue) {
    NR_ENABLE_VALUE_DISABLE = 0,
    NR_ENABLE_VALUE_ENABLE = 1,
};


NR_PLUGIN_ENUM(NRResolution) {
    NR_RESOLUTION_UNKNOWN = 0,
    NR_RESOLUTION_1920_1080_60 = 1,
    NR_RESOLUTION_1920_1080_72 = 2,
    NR_RESOLUTION_1920_1080_90 = 3,
    NR_RESOLUTION_1920_1080_120 = 4,
    NR_RESOLUTION_3840_1080_60 = 5,
    NR_RESOLUTION_3840_1080_72 = 6,
    NR_RESOLUTION_3840_1080_90 = 7,
    NR_RESOLUTION_3840_1080_120 = 8,
};


NR_PLUGIN_ENUM(NRDisplayUsage) {
    NR_DISPLAY_USAGE_LEFT = 0,
    NR_DISPLAY_USAGE_RIGHT,
};


NR_PLUGIN_ENUM(NREdid) {
    NR_EDID_UNKNOWN = 0,
    NR_EDID_1920_1080_60 = 1,
    NR_EDID_1920_1080_72 = 2,
    NR_EDID_1920_1080_90 = 3,
    NR_EDID_1920_1080_120 = 4,
    NR_EDID_3840_1080_60 = 5,
    NR_EDID_3840_1080_72 = 6,
    NR_EDID_3840_1080_90 = 7,
    NR_EDID_3840_1080_120 = 8,
    NR_EDID_1920_1080_60_DEFAULT = 9,
};


NR_PLUGIN_ENUM(NRDeviceConnectMode) {
    NR_DEVICE_CONNECT_MODE_USB = 0,
    NR_DEVICE_CONNECT_MODE_SOCKET = 1,
};


#pragma pack(1)
typedef struct NRDisplayTimingInfo {
    union {
        struct {
            uint64_t hardware_display_interval;
            uint64_t last_frame_time;
            uint64_t next_frame_time;
            uint64_t cur_time;
        };
        uint8_t padding[64];
    };

} NRDisplayTimingInfo;

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
        float camera_distortion[16];
    };

} NRCameraDistortion;

typedef struct NRGUID {
    uint64_t high;
    uint64_t low;

} NRGUID;

typedef struct NRDisplayInfo {
    NRSize2i resolution;
    uint32_t refresh_rate;
    NRDisplay2D3DMode mode;

} NRDisplayInfo;

typedef struct NRResolutionInfo {
    union {
        struct {
            int32_t width;
            int32_t height;
            int32_t refresh_rate;
        };
        uint8_t padding[32];
    };

} NRResolutionInfo;

typedef struct NRPluginMessage {
    uint32_t plugin_category;
    const char * plugin_description_data;
    uint32_t plugin_description_len;
    const void * plugin_message_data;
    uint32_t plugin_message_len;

} NRPluginMessage;

#pragma pack()

#endif // NRSDK