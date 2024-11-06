#pragma once

#include <nr_plugin_hmd.h>
#include <nr_plugin_hmd.h>
#include <nr_plugin_types.h>
#include "nr_plugin_interface.h"
#include "nr_plugin_grayscale_camera_types.h"
#include "nr_plugin_imu_types.h"
#include "nr_perception_sensor_param.h"

NR_PLUGIN_ENUM(PluginDefinition){
    PLUGIN_DEFAULT = -1,
    PLUGIN_6DOF = 0,
    PLUGIN_3DOF,
    PLUGIN_0DOF,
    PLUGIN_EIS,
    PLUGIN_3DOF_EIS,
    PLUGIN_EXT_3DOF,
    PLUGIN_EXT_6DOF,
    PLUGIN_EXT_EIS,
    PLUGIN_EXT_3DOF_EIS,
    PLUGIN_ANDROID_CONTROLLER,
    PLUGIN_BMW_WORLD_LOCK,
    PLUGIN_BMW_CAR_LOCK,
    PLUGIN_NIO_3DOF,
    PLUGIN_NIO_6DOF,
    PLUGIN_NIO_EIS,
    PLUGIN_NIO_3DOF_EIS,
    PLUGIN_REMOTE,
};

struct ControllerProvider {
    NRPluginResult (*GetControllerCalibrationData)(const char**, uint32_t*);
    NRPluginResult (*GetCalibrationData)(uint8_t* out_data, uint32_t* in_out_size);
    NRPluginResult (*SetCalibrationData)(const uint8_t* data, uint32_t size);
};

#pragma pack(1)

typedef struct Landmark3D {
    uint64_t id;
    double uncertainty;
    NRVector3f position;
} Landmark3D;

typedef struct LandmarkBlockData {
    union {
        struct {
            NRTransform transform;
            uint32_t marks_count;
            Landmark3D * landmarks; // 内存由设置方管理, 外部只读
        };
        uint8_t padding[64];
    };
} LandmarkBlockData;

// TODO 内部各种Pose Struct统一使用PerceptionDevicePose; 新代码建议使用PerceptionDevicePose
typedef struct NRPoseWithState {
    union {
        struct {
            uint64_t hmd_time_nanos_device;
            NRTransform g_T_i;
            NRVector3f gyro_bias;
            NRVector3f velocity;
            NRVector3f acc_bias;
        };
        uint8_t padding[128];
    };
} NRPoseWithState;

NR_PLUGIN_ENUM(TrackingReason) {
    TRACKING_REASON_NONE = 0,
    TRACKING_REASON_INITIALIZING = 1,
    TRACKING_REASON_EXCESSIVE_MOTION = 2,
    TRACKING_REASON_INSUFFICIENT_FEATURES = 3,
    TRACKING_REASON_RELOCALIZING = 4,
    TRACKING_REASON_LOW_LIGHT = 5,
    TRACKING_REASON_SENSOR_ABNORML = 6,
    TRACKING_REASON_NOT_ENOUGH_SENSOR = 7,
};
typedef struct PerceptionDevicePose {
    union {
        struct {
            NRTransform transform;
            uint64_t hmd_time_nanos_device;
            uint64_t hmd_time_nanos;
            TrackingReason tracking_reason;
            NRVector3f linear_accl;
            NRVector3f linear_velocity;
            NRVector3f angular_velocity;
            NRVector3f acc_bias;
            NRVector3f gyro_bias;
        };
        uint8_t padding[128];
    };
} PerceptionDevicePose;

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


NR_DECLARE_INTERFACE(PluginCommonInterface) {
    NRPluginResult(NR_INTERFACE_API *GetCameraIntrinsic)(NRPluginHandle handle,
                                                         NRCameraIntrinsic *intrinsic,
                                                         NRComponent camera_type);
    NRPluginResult(NR_INTERFACE_API *GetExtrinsic)(NRPluginHandle handle, NRComponent base,
                                                   NRComponent target,
                                                   NRTransform *transform);

    NRPluginResult(NR_INTERFACE_API *GetCalibrationData)(NRPluginHandle handle,
                                                         uint8_t *out_data,
                                                         uint32_t *in_out_size);
    NRPluginResult(NR_INTERFACE_API *SetCalibrationData)(NRPluginHandle handle,
                                                         const uint8_t *data,
                                                         uint32_t size);
    NRPluginResult(NR_INTERFACE_API *GetControllerCalibrationData)(
        NRPluginHandle handle,
        const char ** data,
        uint32_t * data_size);

    NRPluginResult(NR_INTERFACE_API *SysTimeToImuTime)(NRPluginHandle handle,
                                                       const uint64_t sys_time_ns,
                                                       uint64_t *imu_time_ns);
    NRPluginResult(NR_INTERFACE_API *ExternalChannelSetProperty)(
        NRHandle handle, int32_t property_type, const void *property_value,
        uint32_t property_value_size);
    NRPluginResult(NR_INTERFACE_API *ExternalChannelGetProperty)(
        NRHandle handle, int32_t property_type, void *property_value,
        uint32_t property_value_size);

    // 获取从DeviceConfig解析的值
    NRPluginResult(NR_INTERFACE_API *GetGyroBias)(
        NRPluginHandle handle,
        NRVector3f *gyro_bias
    );
    // 获取从DeviceConfig解析的值
    NRPluginResult(NR_INTERFACE_API *GetAccelBias)(
        NRPluginHandle handle,
        NRVector3f *accel_bias
    );
    // 获取从DeviceConfig解析的值
    NRPluginResult(NR_INTERFACE_API *GetMagBias)(
        NRPluginHandle handle,
        NRVector3f *mag_bias
    );
    // 获取从DeviceConfig解析的值
    NRPluginResult(NR_INTERFACE_API *GetGyroQMag)(
        NRPluginHandle handle,
        NRQuatf *gyro_q_mag
    );
    NRPluginResult(NR_INTERFACE_API *GetGlobalConfig)(
        NRPluginHandle handle,
        const char **data,
        uint32_t &size);

    NRPluginResult(NR_INTERFACE_API *SensorSetProperty)(NRPluginHandle handle,
                                                  NRSensorParamType property_type,
                                                  const void *property_value,
                                                  uint32_t property_value_size);
    NRPluginResult(NR_INTERFACE_API *SensorGetProperty)(NRPluginHandle handle,
                                                  NRSensorParamType property_type,
                                                  void *property_value,
                                                  uint32_t property_value_size);
};

NR_REGISTER_INTERFACE_GUID(0x5ede7c23ef6f4058ULL, 0x51c523842c7e1548ULL,
                            PluginCommonInterface)