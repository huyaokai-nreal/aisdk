      
#pragma once

#include <stdint.h>

#include "nr_plugin_types.h"

enum NRSensorParamType : uint16_t {
  NR_SENSOR_PARAM_INVALID = 0,

  NR_SENSOR_EXTRINSIC = 11,  // 两个传感器间的外参

  NR_CAMERA_INTERINSIC = 21,  // 相机内参

  NR_IMU_GYRO_BIAS = 31,  // 某个IMU在某个温度下的gyro 和 gyro bias，取时需传入温度
  NR_IMU_ACCL_BIAS = 32,
  NR_IMU_INTERINSIC= 33,       // IMU的内参矩阵

  NR_MAGNETOMETER_BIAS = 41,  // 磁力计的bias
  NR_GYRO_Q_MAG = 42, //

  NR_INTERAL_SELF_DEFINED_START = 10000,
  NR_INTERAL_GYRO_BIAS_DRIFT = 10001, //
};

#pragma pack(1)

typedef struct NRCameraIntrinsic {
    NRCameraModel camera_model;
    union {
        struct {
            float fx;
            float fy;
            float cx;
            float cy;
        };
        float intrinsic[4];
    };
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
        float distortion[16];
    };
} NRCameraIntrinsic;

typedef struct NRSensorExtrinsic {
  NRComponent source_type;
  NRComponent target_type;
  uint8_t source_index;
  uint8_t target_index;
  NRTransform t_T_s;
} NRSensorExtrinsic;

typedef struct NRIMUTempBias {
  bool temperature_valid;
  float temperature;
  NRVector3f bias;
} NRIMUGyroTempBias;

typedef struct NRIMUIntrinsic {
  bool temperature_valid;
  float temperature;
  float gyro_matrix[9];
  float acc_matrix[9];
  NRVector3f static_gyro_std;
  NRVector3f static_acc_std;
  NRVector3f static_gyro_peak;
  NRVector3f static_acc_peak;
  uint32_t static_detection_win_size;
} NRIMUIntrinsicMatrix;

typedef struct NRMagnetometerBias {
  NRVector3f bias;
} NRMagnetometerBias;

typedef struct NRParamId {
  union {
    struct {
      uint8_t  sensor_index; /// sensor index starts from 0
      uint16_t param_type;   /// NRSensorParamType
      uint8_t  device_id;    /// default 0
    };
    uint32_t id;
  };
} NRParamId;

typedef struct NRTrackingSensorParam {
  union {
    struct {
      NRParamId param_id;
      union {
        NRCameraIntrinsic camera_intrinsic;
        NRSensorExtrinsic sensor_extrinsic;
        NRIMUTempBias gyro_bias;
        NRIMUTempBias accl_bias;
        NRIMUIntrinsic imu_intrinsic;
        NRIMUTempBias magnetometer_bias;
      };
    };
    uint8_t padding[128];
  };
} NRTRACKINGSensorParam;

#pragma pack()

    