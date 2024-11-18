#pragma once

#ifdef NRAPP

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
    NR_COMPONENT_NECK,
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


NR_PLUGIN_ENUM(NRSceneMode) {
    NR_SCENE_MODE_SPACE_SCREEN = 0,
    NR_SCENE_MODE_WITH_NEBULA = 1,
};


NR_PLUGIN_ENUM(NRPerceptionType) {
    NR_PERCEPTION_TYPE_6DOF = 0,
    NR_PERCEPTION_TYPE_3DOF,
    NR_PERCEPTION_TYPE_0DOF,
    NR_PERCEPTION_TYPE_EIS,
    NR_PERCEPTION_TYPE_OSD_EIS,
    NR_PERCEPTION_TYPE_REMOTE = 100,
};


NR_PLUGIN_ENUM(NRKeyEvent) {
    NR_KEY_EVENT_NULL = 0,
    NR_KEY_EVENT_CLICK,
    NR_KEY_EVENT_DOUBLE_CLICK,
    NR_KEY_EVENT_PRESS,
};


NR_PLUGIN_ENUM(NRFrameBufferFormat) {
    NR_FRAME_BUFFER_FORMAT_RGBA_PLANAR = 0,
    NR_FRAME_BUFFER_FORMAT_ARGB_PLANAR = 1,
    NR_FRAME_BUFFER_FORMAT_YUV420_PLANAR = 2,
    NR_FRAME_BUFFER_FORMAT_BGRA_8888 = 3,
    NR_FRAME_BUFFER_FORMAT_BGRA_4444 = 4,
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


NR_PLUGIN_ENUM(NRDirection) {
    NR_DIRECTION_UP = 0,
    NR_DIRECTION_DOWN = 1,
    NR_DIRECTION_LEFT = 2,
    NR_DIRECTION_RIGHT = 3,
};


NR_PLUGIN_ENUM(NRDisplayUsage) {
    NR_DISPLAY_USAGE_LEFT = 0,
    NR_DISPLAY_USAGE_RIGHT,
};


NR_PLUGIN_ENUM(NRStepType) {
    NR_STEP_TYPE_NORMAL = 0,
    NR_STEP_TYPE_EXTENDED = 1,
};


NR_PLUGIN_ENUM(NRImageFormat) {
    NR_IMAGE_FORMAT_UNKNOWN = 0,
    NR_IMAGE_FORMAT_BGR_888_packed = 1,
    NR_IMAGE_FORMAT_RGB_888_packed,
    NR_IMAGE_FORMAT_BGRA_8888_packed,
    NR_IMAGE_FORMAT_RGBA_8888_packed,
    NR_IMAGE_FORMAT_BGR_888_planar,
    NR_IMAGE_FORMAT_RGB_888_planar,
    NR_IMAGE_FORMAT_BGRA_8888_planar,
    NR_IMAGE_FORMAT_RGBA_8888_planar,
    NR_IMAGE_FORMAT_ARGB_8888_planar,
    NR_IMAGE_FORMAT_GRAY_8,
    NR_IMAGE_FORMAT_YUV_420_888,
    NR_IMAGE_FORMAT_YV12,
    NR_IMAGE_FORMAT_NV21,
    NR_IMAGE_FORMAT_NV12,
    NR_IMAGE_FORMAT_BGRA_4444_packed,
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


NR_PLUGIN_ENUM(NRSpaceMode) {
    NR_SPACE_MODE_HOVER = 0,
    NR_SPACE_MODE_FOLLOW = 1,
    NR_SPACE_MODE_THUMBNAIL = 2,
    NR_SPACE_MODE_ULTRA_WIDE = 3,
};


NR_PLUGIN_ENUM(NRAvailableValue) {
    NR_AVAILABLE_VALUE_NOT_AVAILABLE = 0,
    NR_AVAILABLE_VALUE_AVAILABLE = 1,
};


NR_PLUGIN_ENUM(NROperationType) {
    NR_OPERATION_INCREASE = 0,
    NR_OPERATION_DECREASE = 1,
};


NR_PLUGIN_ENUM(NRThumbnailPositionType) {
    NR_THUMBNAIL_POSE_TYPE_LEFT_TOP = 0,
    NR_THUMBNAIL_POSE_TYPE_RIGHT_TOP = 1,
};


NR_PLUGIN_ENUM(NRCalibrationTriggerType) {
    NR_CALIBRATION_TRIGGER_TYPE_UNKNOWN = 0,
    NR_CALIBRATION_TRIGGER_TYPE_SLIENT = 1,
    NR_CALIBRATION_TRIGGER_TYPE_MANUAL = 2,
};


NR_PLUGIN_ENUM(NRCalibrationResult) {
    NR_CALIBRATION_RESULT_FAILED = -1,
    NR_CALIBRATION_RESULT_SUCCESS = 0,
    NR_CALIBRATION_RESULT_DATA_ERROR = 1,
    NR_CALIBRATION_RESULT_BIAS_ERROR = 2,
    NR_CALIBRATION_RESULT_TIMEOUT = 3,
    NR_CALIBRATION_RESULT_NOT_STILL = 4,
};


NR_PLUGIN_ENUM(NRDeviceConnectMode) {
    NR_DEVICE_CONNECT_MODE_USB = 0,
    NR_DEVICE_CONNECT_MODE_SOCKET = 1,
};


NR_PLUGIN_ENUM(NRStatsCategoryID) {
    NR_STATS_CATEGORY_ID_NORMAL = 5,
};


NR_PLUGIN_ENUM(NRStatsEventID) {
    NR_STATS_EVENT_ID_EC_LEVEL = 0x0030,
    NR_STATS_EVENT_ID_CLICK_SPACE_MODE = 0x0047,
    NR_STATS_EVENT_ID_DOUBLE_CLICK_ENTER_OSD_MENU = 0x0048,
    NR_STATS_EVENT_ID_BRIGHTNESS_ADJUST = 0x0049,
    STATS_EVENT_ID_PRESS_RECENTER = 0x004A,
    NR_STATS_EVENT_ID_OSD_VOLUME_ADJUST = 0x004B,
    NR_STATS_EVENT_ID_SWITCH_AUDIO_MODE = 0x0028,
    NR_STATS_EVENT_ID_OSD_CANVAS_DEPTH_ADJUSTMENT = 0x004C,
    NR_STATS_EVENT_ID_OSD_CANVAS_DIAGONAL_SIZE_ADJUST = 0x004D,
    NR_STATS_EVENT_ID_ENABLE_3D_MODE = 0x004E,
    NR_STATS_EVENT_ID_OSD_SWITCH_DISPLAY_COLOR_CALIBRATION = 0x004F,
    NR_STATS_EVENT_ID_OSD_COLOR_TEMPERATURE_ADJUST = 0x0050,
    NR_STATS_EVENT_ID_OSD_CLICK_HOST_KEY = 0x0051,
    NR_STATS_EVENT_ID_OSD_PRESS_HOST_KEY = 0x0052,
    NR_STATS_EVENT_ID_TRANSPARENT_STATE_BEGIN = 0x0053,
    NR_STATS_EVENT_ID_RGB_CAMERA_PLUGIN = 0x0054,
    NR_STATS_EVENT_ID_RGB_CAMERA_PLUGOUT = 0x0055,
    NR_STATS_EVENT_ID_STARTUP_BEGIN = 0x0059,
    NR_STATS_EVENT_ID_STARTUP_END = 0x005A,
    NR_STATS_EVENT_ID_TRANSPARENT_STATE_END = 0x005B,
    NR_STATS_EVENT_ID_EXIT_OSD = 0x005C,
    NR_STATS_EVENT_ID_ENABLE_ULTRA_WIDE = 0x005D,
    NR_STATS_EVENT_ID_DISABLE_ULTRA_WIDE = 0x005E,
    NR_STATS_EVENT_ID_AUTO_SLEEP_DURATION_SETTING = 0x005F,
    NR_STATS_EVENT_ID_OSD_PUPIL_ADJUST = 0x0060,
    NR_STATS_EVENT_ID_CLICK_TAKE_PICTURE = 0x0061,
    NR_STATS_EVENT_ID_PRESS_TAKE_VIDEO = 0x0062,
    NR_STATS_EVENT_ID_END_TAKE_VIDEO = 0x0063,
    NR_STATS_EVENT_ID_OSD_SET_VIDEO_DURATION = 0x0064,
    NR_STATS_EVENT_ID_OSD_SET_VIDEO_FRAME = 0x0065,
    NR_STATS_EVENT_ID_OSD_CLEANUP_STORAGE = 0x0066,
    NR_STATS_EVENT_ID_OSD_KEY_TUTORIAL_ENTRY = 0x0067,
    NR_STATS_EVENT_ID_OSD_STATIC_CALIBRATION_ENTRY = 0x0068,
    NR_STATS_EVENT_ID_OSD_MANUAL_STATIC_CALIBRATION_END = 0x0069,
    NR_STATS_EVENT_ID_AUTO_STATIC_CALIBRATION_END = 0x0070,
    NR_STATS_EVENT_ID_OSD_LANGUAGE_SETTING = 0x0071,
    NR_STATS_EVENT_ID_DISPLAY_SCREEN_ENABLE_DURATION = 0x0072,
    NR_STATS_EVENT_ID_HIGH_TEMPERATURE = 0x0073,
    NR_STATS_EVENT_ID_OVER_TEMPERATURE = 0x0074,
    NR_STATS_EVENT_ID_OSD_BRIGHTNESS_ENHANCE_TOGGLE = 0x0075,
    NR_STATS_EVENT_ID_OSD_FOLLOW_EIS_TOGGLE = 0x0076,
    NR_STATS_EVENT_ID_OSD_THUMBNAIL_MODE_TOGGLE = 0x0077,
    NR_STATS_EVENT_ID_OSD_THUMBNAIL_POSITION_SELECTION = 0x0078,
    NR_STATS_EVENT_ID_OSD_AUTO_EC_TOGGLE = 0x0079,
};


NR_PLUGIN_ENUM(NRTrackingReason) {
    NR_TRACKING_REASON_NONE = 0,
    NR_TRACKING_REASON_INITIALIZING = 1,
    NR_TRACKING_REASON_EXCESSIVE_MOTION = 2,
    NR_TRACKING_REASON_INSUFFICIENT_FEATURES = 3,
    NR_TRACKING_REASON_RELOCALIZING = 4,
    NR_TRACKING_REASON_LOW_LIGHT = 5,
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

typedef struct NRPluginMessage {
    uint64_t plugin_category;
    const char * plugin_description_data;
    uint32_t plugin_description_len;
    const void * plugin_message_data;
    uint32_t plugin_message_len;

} NRPluginMessage;

typedef struct NRFrameBufferAllocateInfo {
    union {
        struct {
            uint32_t width;
            uint32_t height;
            NRFrameBufferFormat format;
        };
        uint8_t padding[96];
    };

} NRFrameBufferAllocateInfo;

typedef struct NRFrameBuffer {
    union {
        struct {
            void* left_buffer;
            void* right_buffer;
        };
        uint8_t padding[48];
    };

} NRFrameBuffer;

typedef struct NRFrameBufferQueue {
    union {
        struct {
            uint32_t buffer_count;
            void** buffer_queue;
        };
        uint8_t padding[96];
    };

} NRFrameBufferQueue;

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

typedef struct NRDisplayInfo {
    NRSize2i resolution;
    uint32_t refresh_rate;
    NRDisplay2D3DMode mode;

} NRDisplayInfo;

typedef struct NRDevicePose {
    union {
        struct {
            NRTransform transform;
            uint64_t hmd_time_nanos_system;
            NRTrackingReason tracking_reason;
            NRVector3f linear_velocity;
            NRVector3f angular_velocity;
            NRVector3f acc_bias;
            NRVector3f gyro_bias;
            uint64_t hmd_time_nanos_device;
        };
        uint8_t padding[128];
    };

} NRDevicePose;

#pragma pack()

#endif // NRAPP