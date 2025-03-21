#pragma once

#include "nr_plugin_types.h"

#ifdef NRAPP

#include "nr_plugin_imu_types.inc"

#else

NR_PLUGIN_ENUM(NRImuDataMask) {
    NR_IMU_DATA_MASK_NULL = 0x0000,
    NR_IMU_DATA_MASK_GYROSCOPE = 0x0001,
    NR_IMU_DATA_MASK_ACCELEROMETER = 0x0002,
    NR_IMU_DATA_MASK_MAGNETOMETER = 0x0004,
    NR_IMU_DATA_MASK_TEMPERATURE = 0x0008,
    NR_IMU_DATA_MASK_ALL = 0xFFFF,
};


NR_PLUGIN_ENUM8(NRImuID) {
    NR_IMU_ID_0 = 0x0001,
    NR_IMU_ID_1 = 0x0002,
    NR_IMU_ID_2 = 0x0004,
    NR_IMU_ID_3 = 0x0008,
};


#pragma pack(1)
typedef struct NRImuData {
    union {
        struct {
            uint64_t hmd_time_nanos_system;
            uint64_t hmd_time_nanos_device;
            uint64_t hmd_time_nanos_sensor;
            union {
                struct {
                    uint32_t gyro_valid : 1;
                    uint32_t accel_valid : 1;
                    uint32_t mag_valid : 1;
                    uint32_t temperature_valid : 1;
                };
            int32_t data_mask;
            };
            NRVector3f gyro;
            NRVector3f accel;
            NRVector3f mag;
            float temperature;
            int8_t imu_id;
            uint32_t frame_id;
        };
        uint8_t padding[128];
    };

} NRImuData;

#pragma pack()

#endif // NRAPP