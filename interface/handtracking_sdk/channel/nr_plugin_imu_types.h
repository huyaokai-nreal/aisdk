#pragma once

#include "nr_plugin_types.h"

#ifdef NRSDK

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


#pragma pack(1)
typedef struct NRImuData {
    union {
        struct {
            uint64_t hmd_time_nanos;
            uint64_t hmd_hw_time_nanos;
            uint64_t hmd_sensor_time_nanos;
            union {
                struct {
                    uint64_t gyro_valid : 1;
                    uint64_t accel_valid : 1;
                    uint64_t mag_valid : 1;
                    uint64_t temperature_valid : 1;
                };
                int32_t data_mask;
            };
            NRVector3f gyro;
            NRVector3f accel;
            NRVector3f mag;
            float temperature;
        };
        uint8_t padding[128];
    };

} NRImuData;

#pragma pack()

#endif // NRSDK