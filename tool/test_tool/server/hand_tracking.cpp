#include "hand_tracking.h"

#include <assert.h>
#include <dlfcn.h>
#include <math.h>

#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <iostream>

#include "absl/strings/str_split.h"
#include "aisdk/base/file.h"
#include "interface/handtracking_sdk/channel/nr_plugin_grayscale_camera_types.h"
#include "interface/handtracking_sdk/common/nr_plugin_generic.h"
#include "interface/handtracking_sdk/common/nr_plugin_hmd.h"
#include "interface/handtracking_sdk/common/nr_plugin_types_ext.inl"
#include "interface/handtracking_sdk/perception/nr_perception_hand_tracking.h"
#include "interface/handtracking_sdk/public/nr_plugin_lifecycle.h"
#include "json/json.h"
#include "perception/nr_plugin_tracking_common.h"

HandTrackingInterface g_nr_handtracking_interface;
NRGenericInterface g_generic_interface;
NRHMDInterface g_hmd_interface;
NRInterfaces g_user_interface;

NRPluginLifecycleProvider g_lifecycle_provider;
HandTrackingProvider g_provider;

int g_camera_params_type = 0;
CameraParams g_camera_params;
CameraParams2 g_camera_params_2;

// #define JOINTS_COUNT 25
// #define KPT_NUMS 21
HandData g_out_hand_array[2];
// 发送数据到sdk内部市，中间变量
SendData g_readying_get_data;
/*********************************plugin**********************************************/

NRInterface* demo_GetInterface(NRInterfaceGUID guid, unsigned long long* out_interface_size) {
    // std::cout << "demo_GetInterface " << guid.high_value_ << " " <<
    // guid.low_value_ << std::endl;
    if (guid == GetNRInterfaceGUID<HandTrackingInterface>()) {
        return &g_nr_handtracking_interface;
    } else if (guid == GetNRInterfaceGUID<NRGenericInterface>()) {
        return &g_generic_interface;
    } else if (guid == GetNRInterfaceGUID<NRHMDInterface>()) {
        return &g_hmd_interface;
    }
    return nullptr;
}

NRPluginResult handtracking_RegisterLifecycleProvider(NRPluginHandle handle, const char* plugin_id,
                                                      const char* plugin_version,
                                                      const NRPluginLifecycleProvider* provider,
                                                      uint32_t provider_size) {
    g_lifecycle_provider = *provider;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult handtracking_RegisterProvider(NRPluginHandle handle, const HandTrackingProvider* provider,
                                             uint32_t provider_size) {
    g_provider = *provider;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult handtracking_GetDevicePose(NRPluginHandle handle, DevicePose* device_pose, uint64_t hmd_time_nanos) {
    device_pose->transform.position.x = g_readying_get_data.stream->headpose[4];
    device_pose->transform.position.y = g_readying_get_data.stream->headpose[5];
    device_pose->transform.position.z = g_readying_get_data.stream->headpose[6];

    device_pose->transform.rotation.qx = g_readying_get_data.stream->headpose[0];
    device_pose->transform.rotation.qy = g_readying_get_data.stream->headpose[1];
    device_pose->transform.rotation.qz = g_readying_get_data.stream->headpose[2];
    device_pose->transform.rotation.qw = g_readying_get_data.stream->headpose[3];
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult hmd_GetComponentFov(NRPluginHandle handle, NRComponent component, NRFov4f* out_fov) {
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult hmd_GetComponentResolution(NRPluginHandle handle, NRComponent device, NRSize2i* out_device_resolution) {
    if (g_camera_params_type == 1) {
        if (NR_COMPONENT_GRAYSCALE_CAMERA_LEFT == device) {
            out_device_resolution->width = g_camera_params.device1.resolution[0];
            out_device_resolution->height = g_camera_params.device1.resolution[1];
        } else if (NR_COMPONENT_GRAYSCALE_CAMERA_RIGHT == device) {
            out_device_resolution->width = g_camera_params.device2.resolution[0];
            out_device_resolution->height = g_camera_params.device2.resolution[1];
        }
    } else if (g_camera_params_type == 2) {
        if (NR_COMPONENT_GRAYSCALE_CAMERA_LEFT == device) {
            out_device_resolution->width = g_camera_params_2.cam0.resolution[0];
            out_device_resolution->height = g_camera_params_2.cam0.resolution[1];
        } else if (NR_COMPONENT_GRAYSCALE_CAMERA_RIGHT == device) {
            out_device_resolution->width = g_camera_params_2.cam1.resolution[0];
            out_device_resolution->height = g_camera_params_2.cam1.resolution[1];
        }
    }
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult hmd_GetComponentRefreshRate(NRPluginHandle handle, NRComponent component, uint32_t* out_refresh_rate) {
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult hmd_GetComponentIntrinsic(NRPluginHandle handle, NRComponent device, NRMat3f* out_intrinsic_matrix) {
    memset(out_intrinsic_matrix, 0, sizeof(NRMat3f));
    if (g_camera_params_type == 1) {
        if (NR_COMPONENT_GRAYSCALE_CAMERA_LEFT == device) {
            out_intrinsic_matrix->column0.x = g_camera_params.device1.fc[0];
            out_intrinsic_matrix->column1.y = g_camera_params.device1.fc[1];
            out_intrinsic_matrix->column2.x = g_camera_params.device1.cc[0];
            out_intrinsic_matrix->column2.y = g_camera_params.device1.cc[1];
            out_intrinsic_matrix->column2.z = 1.0f;
        } else if (NR_COMPONENT_GRAYSCALE_CAMERA_RIGHT == device) {
            out_intrinsic_matrix->column0.x = g_camera_params.device2.fc[0];
            out_intrinsic_matrix->column1.y = g_camera_params.device2.fc[1];
            out_intrinsic_matrix->column2.x = g_camera_params.device2.cc[0];
            out_intrinsic_matrix->column2.y = g_camera_params.device2.cc[1];
            out_intrinsic_matrix->column2.z = 1.0f;
        }
    } else if (g_camera_params_type == 2) {
        if (NR_COMPONENT_GRAYSCALE_CAMERA_LEFT == device) {
            out_intrinsic_matrix->column0.x = g_camera_params_2.cam0.intrinsic[0][0];
            out_intrinsic_matrix->column1.y = g_camera_params_2.cam0.intrinsic[1][1];
            out_intrinsic_matrix->column2.x = g_camera_params_2.cam0.intrinsic[0][2];
            out_intrinsic_matrix->column2.y = g_camera_params_2.cam0.intrinsic[1][2];
            out_intrinsic_matrix->column2.z = 1.0f;
        } else if (NR_COMPONENT_GRAYSCALE_CAMERA_RIGHT == device) {
            out_intrinsic_matrix->column0.x = g_camera_params_2.cam1.intrinsic[0][0];
            out_intrinsic_matrix->column1.y = g_camera_params_2.cam1.intrinsic[1][1];
            out_intrinsic_matrix->column2.x = g_camera_params_2.cam1.intrinsic[0][2];
            out_intrinsic_matrix->column2.y = g_camera_params_2.cam1.intrinsic[1][2];
            out_intrinsic_matrix->column2.z = 1.0f;
        }
    }
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult hmd_GetComponentDistortion(NRPluginHandle handle, NRComponent device, NRCameraDistortion* out_params) {
    memset(out_params, 0, sizeof(NRCameraDistortion));
    if (g_camera_params_type == 1) {
        out_params->camera_model = NRCameraModel(g_camera_params.device1.camera_model);
        if (NR_COMPONENT_GRAYSCALE_CAMERA_LEFT == device) {
            for (uint32_t i = 0; i < 12; i++) {
                out_params->camera_distortion[i] = g_camera_params.device1.kc[i];
            }
        } else if (NR_COMPONENT_GRAYSCALE_CAMERA_RIGHT == device) {
            for (uint32_t i = 0; i < 12; i++) {
                out_params->camera_distortion[i] = g_camera_params.device2.kc[i];
            }
        }
    } else if (g_camera_params_type == 2) {
        out_params->camera_model = NRCameraModel(g_camera_params_2.cam0.camera_model);
        if (NR_COMPONENT_GRAYSCALE_CAMERA_LEFT == device) {
            for (uint32_t i = 0; i < 12; i++) {
                out_params->camera_distortion[i] = g_camera_params_2.cam0.distortion[i];
            }
        } else if (NR_COMPONENT_GRAYSCALE_CAMERA_RIGHT == device) {
            for (uint32_t i = 0; i < 12; i++) {
                out_params->camera_distortion[i] = g_camera_params_2.cam1.distortion[i];
            }
        }
    }
    return NR_PLUGIN_RESULT_SUCCESS;
}
/*
https://nreal.feishu.cn/docs/doccnKxq9P7eftGbXfDUb6eVtgd

4. imu_q_cam 为相机外参旋转分量（JPL四元数格式）。
imu_p_cam为相机外参平移分量(单位为米)。
表示相机坐标系到IMU坐标系的转换(OPENCV坐标系下)。
5. leftcam_q_rightcam为相机外参旋转分量（JPL四元数格式）。
leftcam_p_rightcam为相机外参平移分量(单位为米)。
表示右目相机坐标系到左目相机坐标系的转换(OPENCV坐标系下)。

def quaternion_to_rotation_matrix(q, JPL_flag=False):
    # x, y ,z ,w
    if JPL_flag:
        # JPL
        q[0:3] = -q[0:3]
    # Hamilton
    rot_matrix = np.array(
        [[1.0 - 2 * (q[1] * q[1] + q[2] * q[2]), 2 * (q[0] * q[1] - q[3] *
q[2]), 2 * (q[3] * q[1] + q[0] * q[2])], [2 * (q[0] * q[1] + q[3] * q[2]), 1.0 -
2 * (q[0] * q[0] + q[2] * q[2]), 2 * (q[1] * q[2] - q[3] * q[0])], [2 * (q[0] *
q[2] - q[3] * q[1]), 2 * (q[1] * q[2] + q[3] * q[0]), 1.0 - 2 * (q[0] * q[0] +
q[1] * q[1])]], dtype=q.dtype) return rot_matrix
*/

void Hamilton_Quaternion_To_Rotation_Matrix(double* Quaternion, double* rt_mat) {
    rt_mat[0] = 1 - 2 * (Quaternion[2] * Quaternion[2]) - 2 * (Quaternion[3] * Quaternion[3]);
    rt_mat[1] = 2 * Quaternion[1] * Quaternion[2] - 2 * Quaternion[0] * Quaternion[3];
    rt_mat[2] = 2 * Quaternion[1] * Quaternion[3] + 2 * Quaternion[0] * Quaternion[2];
    rt_mat[3] = 2 * Quaternion[1] * Quaternion[2] + 2 * Quaternion[0] * Quaternion[3];
    rt_mat[4] = 1 - 2 * (Quaternion[1] * Quaternion[1]) - 2 * (Quaternion[3] * Quaternion[3]);
    rt_mat[5] = 2 * Quaternion[2] * Quaternion[3] - 2 * Quaternion[0] * Quaternion[1];
    rt_mat[6] = 2 * Quaternion[1] * Quaternion[3] - 2 * Quaternion[0] * Quaternion[2];
    rt_mat[7] = 2 * Quaternion[2] * Quaternion[3] + 2 * Quaternion[0] * Quaternion[1];
    rt_mat[8] = 1 - 2 * (Quaternion[1] * Quaternion[1]) - 2 * (Quaternion[2] * Quaternion[2]);
    // for (int i = 0; i < 9; i++) {
    //     printf("rt_mat[i]=%f \n", (float)rt_mat[i]);
    // }
}

struct Quaternion {
    float x;
    float y;
    float z;
    float w;
};

struct rotMatrix {
    float m11, m12, m13;
    float m21, m22, m23;
    float m31, m32, m33;
};

void Hamilton_Rotation_Matrix_To_Quaternion(Quaternion& q, rotMatrix& r) {
    float tr = r.m11 + r.m22 + r.m33;
    float temp = 0.0;
    if (tr > 0.0) {
        temp = 0.5f / sqrtf(tr + 1);
        q.w = 0.25f / temp;
        q.x = (r.m23 - r.m32) * temp;
        q.y = (r.m31 - r.m13) * temp;
        q.z = (r.m12 - r.m21) * temp;
    } else {
        if (r.m11 > r.m22 && r.m11 > r.m33) {
            temp = 2.0f * sqrtf(1.0f + r.m11 - r.m22 - r.m33);
            q.w = (r.m32 - r.m23) / temp;
            q.x = 0.25f * temp;
            q.y = (r.m12 + r.m21) / temp;
            q.z = (r.m13 + r.m31) / temp;
        } else if (r.m22 > r.m33) {
            temp = 2.0f * sqrtf(1.0f + r.m22 - r.m11 - r.m33);
            q.w = (r.m13 - r.m31) / temp;
            q.x = (r.m12 + r.m21) / temp;
            q.y = 0.25f * temp;
            q.z = (r.m23 + r.m32) / temp;
        } else {
            temp = 2.0f * sqrtf(1.0f + r.m33 - r.m11 - r.m22);
            q.w = (r.m21 - r.m12) / temp;
            q.x = (r.m13 + r.m31) / temp;
            q.y = (r.m23 + r.m32) / temp;
            q.z = 0.25f * temp;
        }
    }
}

NRPluginResult hmd_GetComponentExtrinsic(NRHandle token, NRComponent source_device_name, NRComponent target_device_name,
                                         NRTransform* out_extrinsic) {
    // memset(out_pose, 0, sizeof(NRMat4f));
    // if (NR_COMPONENT_HEAD == source_device_name && NR_COMPONENT_GRAYSCALE_CAMERA_LEFT == target_device_name) {
    //     double hamilton_imu_q_cam[4] = {g_camera_params.device1.imu_q_cam[3], -g_camera_params.device1.imu_q_cam[0],
    //                                     -g_camera_params.device1.imu_q_cam[1],
    //                                     -g_camera_params.device1.imu_q_cam[2]};
    //     double rt_mat[9];
    //     Hamilton_Quaternion_To_Rotation_Matrix(hamilton_imu_q_cam, rt_mat);
    //     out_pose->column0.x = rt_mat[0];
    //     out_pose->column1.x = rt_mat[1];
    //     out_pose->column2.x = rt_mat[2];
    //     out_pose->column0.y = rt_mat[3];
    //     out_pose->column1.y = rt_mat[4];
    //     out_pose->column2.y = rt_mat[5];
    //     out_pose->column0.z = rt_mat[6];
    //     out_pose->column1.z = rt_mat[7];
    //     out_pose->column2.z = rt_mat[8];
    //     out_pose->column3.x = g_camera_params.device1.imu_p_cam[0];
    //     out_pose->column3.y = g_camera_params.device1.imu_p_cam[1];
    //     out_pose->column3.z = g_camera_params.device1.imu_p_cam[2];
    //     out_pose->column3.w = 1.0f;
    // } else if (NR_COMPONENT_GRAYSCALE_CAMERA_LEFT == source_device_name &&
    //            NR_COMPONENT_GRAYSCALE_CAMERA_RIGHT == target_device_name) {
    //     double hamilton_leftcam_q_rightcam[4] = {
    //         g_camera_params.leftcam_q_rightcam[3], -g_camera_params.leftcam_q_rightcam[0],
    //         -g_camera_params.leftcam_q_rightcam[1], -g_camera_params.leftcam_q_rightcam[2]};
    //     double rt_mat[9];
    //     Hamilton_Quaternion_To_Rotation_Matrix(hamilton_leftcam_q_rightcam, rt_mat);
    //     out_pose->column0.x = rt_mat[0];
    //     out_pose->column1.x = rt_mat[1];
    //     out_pose->column2.x = rt_mat[2];
    //     out_pose->column0.y = rt_mat[3];
    //     out_pose->column1.y = rt_mat[4];
    //     out_pose->column2.y = rt_mat[5];
    //     out_pose->column0.z = rt_mat[6];
    //     out_pose->column1.z = rt_mat[7];
    //     out_pose->column2.z = rt_mat[8];
    //     out_pose->column3.x = g_camera_params.leftcam_p_rightcam[0];
    //     out_pose->column3.y = g_camera_params.leftcam_p_rightcam[1];
    //     out_pose->column3.z = g_camera_params.leftcam_p_rightcam[2];
    //     out_pose->column3.w = 1.0f;
    // }

    memset(out_extrinsic, 0, sizeof(NRTransform));
    if (g_camera_params_type == 1) {
        if (NR_COMPONENT_HEAD == source_device_name && NR_COMPONENT_GRAYSCALE_CAMERA_LEFT == target_device_name) {
            out_extrinsic->rotation.qx = -g_camera_params.device1.imu_q_cam[0];
            out_extrinsic->rotation.qy = -g_camera_params.device1.imu_q_cam[1];
            out_extrinsic->rotation.qz = -g_camera_params.device1.imu_q_cam[2];
            out_extrinsic->rotation.qw = g_camera_params.device1.imu_q_cam[3];
            out_extrinsic->position.x = g_camera_params.device1.imu_p_cam[0];
            out_extrinsic->position.y = g_camera_params.device1.imu_p_cam[1];
            out_extrinsic->position.z = g_camera_params.device1.imu_p_cam[2];
        } else if (NR_COMPONENT_GRAYSCALE_CAMERA_LEFT == source_device_name &&
                   NR_COMPONENT_GRAYSCALE_CAMERA_RIGHT == target_device_name) {
            out_extrinsic->rotation.qx = g_camera_params.leftcam_q_rightcam[0];
            out_extrinsic->rotation.qy = g_camera_params.leftcam_q_rightcam[1];
            out_extrinsic->rotation.qz = g_camera_params.leftcam_q_rightcam[2];
            out_extrinsic->rotation.qw = g_camera_params.leftcam_q_rightcam[3];
            out_extrinsic->position.x = g_camera_params.leftcam_p_rightcam[0];
            out_extrinsic->position.y = g_camera_params.leftcam_p_rightcam[1];
            out_extrinsic->position.z = g_camera_params.leftcam_p_rightcam[2];
        }
    } else if (g_camera_params_type == 2) {
        if (NR_COMPONENT_HEAD == source_device_name && NR_COMPONENT_GRAYSCALE_CAMERA_LEFT == target_device_name) {
            out_extrinsic->rotation.qx = 0.0f;
            out_extrinsic->rotation.qy = 0.0f;
            out_extrinsic->rotation.qz = 0.0f;
            out_extrinsic->rotation.qw = 1.0f;
            out_extrinsic->position.x = 0.0f;
            out_extrinsic->position.y = 0.0f;
            out_extrinsic->position.z = 0.0f;
        } else if (NR_COMPONENT_GRAYSCALE_CAMERA_LEFT == source_device_name &&
                   NR_COMPONENT_GRAYSCALE_CAMERA_RIGHT == target_device_name) {
            // Quaternion q;
            // rotMatrix rot_matrix;
            // rot_matrix.m11 = g_camera_params_2.cam1_to_cam0_extrinsic[0][0];
            // rot_matrix.m12 = g_camera_params_2.cam1_to_cam0_extrinsic[0][1];
            // rot_matrix.m13 = g_camera_params_2.cam1_to_cam0_extrinsic[0][2];
            // rot_matrix.m21 = g_camera_params_2.cam1_to_cam0_extrinsic[1][0];
            // rot_matrix.m22 = g_camera_params_2.cam1_to_cam0_extrinsic[1][1];
            // rot_matrix.m23 = g_camera_params_2.cam1_to_cam0_extrinsic[1][2];
            // rot_matrix.m31 = g_camera_params_2.cam1_to_cam0_extrinsic[2][0];
            // rot_matrix.m32 = g_camera_params_2.cam1_to_cam0_extrinsic[2][1];
            // rot_matrix.m33 = g_camera_params_2.cam1_to_cam0_extrinsic[2][2];
            // Hamilton_Rotation_Matrix_To_Quaternion(q, rot_matrix);
            // std::cout << "Hamilton_Rotation_Matrix_To_Quaternion " <<  q.x << "," <<  q.y << "," <<  q.z << "," <<
            // q.w << "," << std::endl; out_extrinsic->rotation.qx = -q.x; out_extrinsic->rotation.qy = -q.y;
            // out_extrinsic->rotation.qz = -q.z;
            // out_extrinsic->rotation.qw = q.w;
            // out_extrinsic->position.x = g_camera_params_2.cam1_to_cam0_extrinsic[0][3];
            // out_extrinsic->position.y = g_camera_params_2.cam1_to_cam0_extrinsic[1][3];
            // out_extrinsic->position.z = g_camera_params_2.cam1_to_cam0_extrinsic[2][3];

            out_extrinsic->rotation.qx = g_camera_params_2.cam1_to_cam0_rotation[0];
            out_extrinsic->rotation.qy = g_camera_params_2.cam1_to_cam0_rotation[1];
            out_extrinsic->rotation.qz = g_camera_params_2.cam1_to_cam0_rotation[2];
            out_extrinsic->rotation.qw = g_camera_params_2.cam1_to_cam0_rotation[3];
            out_extrinsic->position.x = g_camera_params_2.cam1_to_cam0_position[0];
            out_extrinsic->position.y = g_camera_params_2.cam1_to_cam0_position[1];
            out_extrinsic->position.z = g_camera_params_2.cam1_to_cam0_position[2];
        }
    }
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult hmd_GetComponentPoseFromHead(NRPluginHandle handle, NRComponent component, NRTransform* out_transform) {
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult hmd_GetComponentFovOverFill(NRPluginHandle handle, NRComponent component, NRFov4f* out_fov_overfill) {
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetSDKVersion(NRVersion* version) {
    version->major = 1;
    version->minor = 0;
    version->revision = 0;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetActivityInfo(void** java_vm, void** context, void** class_loader) {
    *java_vm = nullptr;
    *context = nullptr;
    *class_loader = nullptr;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetPluginConfig(const char** data, uint32_t* data_size) {
    *data = nullptr;
    *data_size = 0;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetDeviceConfig(NRPluginHandle handle, const char** data, uint32_t* data_size) {
    *data = nullptr;
    *data_size = 0;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetDeviceMiscConfig(NRPluginHandle handle, const char** data, uint32_t* data_size) {
    *data = nullptr;
    *data_size = 0;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetGlobalConfig(NRPluginHandle handle, const char** data, uint32_t* data_size) {
    static std::string global_config;
    std::string name("./sdk_global.json");
    if (0 == aisdk::base::ReadFromFile(name, global_config)) {
        *data = global_config.data();
        *data_size = global_config.size();
    } else {
        *data = nullptr;
        *data_size = 0;
    }
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetNetworkType(NRPluginHandle handle, NRNetworkType* network_type) {
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetDeviceType(NRPluginHandle handle, NRDeviceType* device_type) {
    if (g_camera_params_type == 1) {
        if (1 == g_camera_params.device1.camera_model) {
            *device_type = NR_DEVICE_TYPE_LIGHT;
        } else if (2 == g_camera_params.device1.camera_model) {
            *device_type = NR_DEVICE_TYPE_FLORA;
        } else if (3 == g_camera_params.device1.camera_model) {
            *device_type = NR_DEVICE_TYPE_FLORA;
        }
    } else if (g_camera_params_type == 2) {
        if (1 == g_camera_params_2.cam0.camera_model) {
            *device_type = NR_DEVICE_TYPE_LIGHT;
        } else if (2 == g_camera_params_2.cam0.camera_model) {
            *device_type = NR_DEVICE_TYPE_FLORA;
        } else if (3 == g_camera_params_2.cam0.camera_model) {
            *device_type = NR_DEVICE_TYPE_FLORA;
        }
    }
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetDynamicLibraryPath(NRPluginHandle handle, const char** out_path, uint32_t* path_size) {
    *out_path = nullptr;
    *path_size = 0;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetChannelIdentifier(NRPluginHandle handle, const char** channel_identifier,
                                            uint32_t* channel_identifier_size) {
    *channel_identifier = nullptr;
    *channel_identifier_size = 0;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_SetThreadPriority(NRPluginHandle handle, int32_t tid, int32_t policy, int32_t priority) {
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetPropertyFlag(NRPluginHandle handle, NRProperty property, int32_t* flag) {
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_UpdateMetricsui64(NRPluginHandle handle, NRMetricsType metrics_type, uint64_t metrics_data) {
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_UpdateMetricsui(NRPluginHandle handle, NRMetricsType metrics_type, uint32_t metrics_data) {
    return NR_PLUGIN_RESULT_SUCCESS;
}
/***********************************************************************************/

int StreamDataContrlAgent::AddAgentData(std::shared_ptr<StreamData>& data) {
    int ret = 0;
    // 源数据发送完毕
    if (nullptr == data) {
        finish_add_all_datas = true;
    } else {
        // printf("AddAgentData frame_id=%d, nano_time=%lu \n", (int)data->frame_id, data->nano_time);
        // 添加数据到缓存中
        std::lock_guard<std::mutex> lock(m_src_datas_mutex);
        if (current_cache_fn >= max_cache_fn) {
            // 缓存满回复等待
            ret = -100;
        } else {
            m_newsrc_datas.push_back(data);
            current_cache_fn++;
            if (current_cache_fn == max_cache_fn) {
                first_datas_full = true;
            }
        }
    }

    // 首次数据等待缓存满再开始计算
    // 数据发送完毕需要将剩余待测试数据发送计算
    if (first_datas_full || finish_add_all_datas) {
        int err = 0;
        while (0 == err) {
            err = ProduceSomeSimulationData();
        }
    }

    // 通知test_finish
    if (finish_add_all_datas) {
        finish_add_all_datas_2 = true;
    }

    return ret;
}

int StreamDataContrlAgent::GetAgentResult(uint64_t frame_id, std::shared_ptr<StreamResult>& result) {
    std::shared_ptr<StreamResult> tmp;
    {
        std::unique_lock<std::mutex> lock(m_agent_result_mutex);
        if (agent_result.size()) {
            tmp = agent_result.front();
            agent_result.pop_front();
            current_agent_result_fn++;
        }
    }

    if (tmp) {
        result = tmp;
        return 0;
    }

    // std::cout << "GetAgentResult finish_add_all_datas_2= " << finish_add_all_datas_2 << std::endl;
    // std::cout << "GetAgentResult current_agent_result_fn= " << current_agent_result_fn << std::endl;
    // std::cout << "GetAgentResult max_send_fn= " << max_send_fn << std::endl;
    // std::cout << "GetAgentResult current_send_fn= " << current_send_fn << std::endl;
    // std::cout << "GetAgentResult current_profiling_fn= " << current_profiling_fn << std::endl;
    // std::cout << "GetAgentResult max_recv_fn= " << max_recv_fn << std::endl;
    // std::cout << "GetAgentResult current_recv_fn= " << current_recv_fn << std::endl;

    // 检查全部数据都已经发送和结果都已经取回，回复测试完毕
    if (finish_add_all_datas_2 && max_send_fn == current_send_fn && max_recv_fn == current_recv_fn &&
        (max_send_fn + max_recv_fn) == current_agent_result_fn) {
        if (false == need_profiling) {
            return -100;
        }

        // std::cout << "GetAgentResult max_send_fn= " << max_send_fn << std::endl;
        // std::cout << "GetAgentResult current_profiling_fn= " << current_profiling_fn << std::endl;
        if (need_profiling && max_send_fn == current_profiling_fn) {
            return -100;
        }
    }

    return -1;
}

int StreamDataContrlAgent::ProduceSomeSimulationData() {
    uint64_t tp_s = 0;
    bool all_1s_ready = false;
    uint64_t all_1s_num = 0;
    uint64_t all_1s_nano_time[120] = {0};
    uint64_t simulation_all_1s_num = 0;
    uint64_t simulation_srcdatas_index[120] = {0};
    uint64_t simulation_all_1s_nano_time[120] = {0};
    uint64_t simulation_all_1s_recv_num = 0;

    if (m_newsrc_datas.size()) {
        // 获取当前1s内的全部可用数据
        std::lock_guard<std::mutex> lock(m_src_datas_mutex);
        tp_s = m_newsrc_datas.front()->nano_time / 1000000000;
        for (auto iter : m_newsrc_datas) {
            all_1s_nano_time[all_1s_num] = iter->nano_time;
            if (tp_s == all_1s_nano_time[all_1s_num] / 1000000000) {
                all_1s_num++;
            } else {
                all_1s_ready = true;
                break;
            }
        }

        // 最后剩余数据继续测试
        if (all_1s_num == m_newsrc_datas.size() && finish_add_all_datas) {
            all_1s_ready = true;
        }
    }

    if (all_1s_ready && all_1s_num) {
        // 构造仿真数据
        if (0 == m_send_simulation_policy) {
            // 完全使用原始数据
            simulation_all_1s_num = all_1s_num;
            for (uint32_t i = 0; i < simulation_all_1s_num; i++) {
                simulation_srcdatas_index[i] = i;
                simulation_all_1s_nano_time[i] = all_1s_nano_time[i];
            }
        } else {
            // 需要仿真不同帧率的输入和输出
            simulation_all_1s_num = m_sdk_config.simulation_sendframe_fps;
            for (uint32_t i = 0; i < simulation_all_1s_num; i++) {
                simulation_all_1s_nano_time[i] = tp_s * 1000000000 + i * m_send_idle_time * 1000000;

                uint64_t min_distance = UINT64_MAX;
                // 最近采样数据，进行匹配
                for (uint32_t j = 0; j < all_1s_num; j++) {
                    uint64_t a = 0;
                    if (simulation_all_1s_nano_time[i] >= all_1s_nano_time[j]) {
                        a = simulation_all_1s_nano_time[i] - all_1s_nano_time[j];
                    } else {
                        a = all_1s_nano_time[j] - simulation_all_1s_nano_time[i];
                    }
                    if (a < min_distance) {
                        min_distance = a;
                        simulation_srcdatas_index[i] = j;
                    }
                }
            }
        }

        // 添加发送队列
        {
            std::lock_guard<std::mutex> lock(m_src_datas_mutex);
            std::lock_guard<std::mutex> lock1(m_send_list_mutex);
            for (uint32_t i = 0; i < simulation_all_1s_num; i++) {
                auto iter1 = m_newsrc_datas.begin();
                SendData sd;
                sd.simulation_nano_time = simulation_all_1s_nano_time[i];
                std::advance(iter1, simulation_srcdatas_index[i]);
                sd.stream = *(iter1);
                sd.token = sd.stream->token;
                sd.increase_profiline_id = max_send_fn;
                send_list.push_back(sd);
                max_send_fn++;
            }

            // 这里是为了将使用过的数据最后释放做准备
            for (uint32_t j = 0; j < all_1s_num; j++) {
                auto tp = m_newsrc_datas.front();
                m_newsrc_datas.pop_front();
                m_usedsrc_datas.push_back(tp);
            }
        }

        // 添加接受队列
        {
            std::lock_guard<std::mutex> lock1(m_recv_list_mutex);
            if (0 == m_recv_simulation_policy) {
                simulation_all_1s_recv_num = all_1s_num;
            } else {
                simulation_all_1s_recv_num = m_sdk_config.simulation_getresult_fps;
            }

            for (uint32_t i = 0; i < simulation_all_1s_recv_num; i++) {
                RecvData rd;
                rd.simulation_nano_time = tp_s * 1000000000 + i * m_recv_idle_time * 1000000;
                recv_list.push_back(rd);
                max_recv_fn++;
            }
        }

    } else {
        return -1;
    }

    return 0;
}

int StreamDataContrlAgent::ProduceProfilingResult(SendData& send, ProfilingInfo& info, bool is_match) {
    Json::Value json_result;
    json_result["exec_status"] = "finish";
    json_result["exec_sequence"]["frame_id"] = send.frame_id;
    json_result["exec_sequence"]["server_actual_nano_time"] = send.server_actual_nano_time;
    json_result["exec_sequence"]["simulation_nano_time"] = send.simulation_nano_time;
    json_result["exec_sequence"]["src_data_nano_time"] = send.src_data_nano_time;

    if (is_match) {
        Json::Value result;
        Json::Reader reader;
        std::string noderesult(info.noderesult_jsonstring);
        if (reader.parse(noderesult, result)) {
            json_result["exec_result"] = result;
        }
    } else {
        json_result["exec_result"] = Json::Value();
    }

    Json::FastWriter fwriter;
    std::string a = fwriter.write(json_result);

    std::shared_ptr<StreamResult> data = std::make_shared<StreamResult>();
    data->frame_id = send.frame_id;
    data->token = send.token;
    data->profiling_exec_data = std::move(a);

    std::unique_lock<std::mutex> lock(m_agent_result_mutex);
    agent_result.push_back(data);

    return 0;
}

void StreamDataContrlAgent::ProfilingWork() {
    std::chrono::time_point<std::chrono::steady_clock> start;
    std::chrono::time_point<std::chrono::steady_clock> end;
    uint64_t free_time = 0;
    start = std::chrono::steady_clock::now();
    end = std::chrono::steady_clock::now();

    ProfilingInfo info;
    SendData senddata;
    info.struct_bytes = sizeof(ProfilingInfo);

    while (false == profiling_exit) {
        // 获取中间profiling结果
        // profiling结果和send的数据是1比1匹配的
        int ret = m_plugin_getprofiling(PROFILING_FLAG, &info);
        if (0 == ret) {
            bool is_match = false;
            {
                std::unique_lock<std::mutex> lock(m_match_mutex);
                auto iter = match_profiling.find(info.timestamp);
                if (iter != match_profiling.end()) {
                    is_match = true;
                    senddata = iter->second;
                    match_profiling.erase(iter);
                }
            }

            if (false == is_match) {
                // 异常
                std::cout << "[ERROR] ProfilingWork timestamp=" << info.timestamp << " is not match!!!!!" << std::endl;
                *(int*)0x0 = 0x0;
            }

            // 删除sdk漏的没有分析的数据
            {
                std::unique_lock<std::mutex> lock(m_match_mutex);
                uint64_t wanted_getprofiling_id = order_match_profiling_id.begin()->first;
                uint64_t real_getprofiling_id = senddata.increase_profiline_id;
                if (wanted_getprofiling_id != real_getprofiling_id) {
                    for (auto iter = order_match_profiling_id.begin(); iter != order_match_profiling_id.end();) {
                        uint64_t lost_profiling_id = iter->first;
                        uint64_t lost_timestamp = iter->second;

                        if (lost_profiling_id < real_getprofiling_id) {
                            ProfilingInfo lost_info;
                            SendData lost_senddata;
                            auto iter1 = match_profiling.find(lost_timestamp);
                            if (iter1 != match_profiling.end()) {
                                lost_senddata = iter1->second;
                                match_profiling.erase(iter1);
                            }

                            ProduceProfilingResult(lost_senddata, lost_info, false);
                            current_profiling_fn++;

                            iter = order_match_profiling_id.erase(iter);
                        } else if (lost_profiling_id == real_getprofiling_id) {
                            order_match_profiling_id.erase(lost_profiling_id);
                            break;
                        } else {
                            *(int*)0x0 = 0x0;
                        }
                    }
                } else {
                    order_match_profiling_id.erase(wanted_getprofiling_id);
                }
            }

            ProduceProfilingResult(senddata, info, is_match);
            current_profiling_fn++;

            if (info.noderesult_jsonstring) {
                free(info.noderesult_jsonstring);
                info.noderesult_jsonstring = nullptr;
            }

            start = std::chrono::steady_clock::now();
            end = std::chrono::steady_clock::now();
        } else {
            // 删除sdk漏的没有分析的超过1分钟以上的数据
            end = std::chrono::steady_clock::now();
            free_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            if (free_time > 60 * 1000) {
                std::cout << "[ERROR] ProfilingWork  60s is not match!!!!!" << std::endl;
                std::unique_lock<std::mutex> lock(m_match_mutex);
                for (auto iter = order_match_profiling_id.begin(); iter != order_match_profiling_id.end(); iter++) {
                    uint64_t lost_profiling_id = iter->first;
                    uint64_t lost_timestamp = iter->second;
                    ProfilingInfo lost_info;
                    SendData lost_senddata;
                    auto iter1 = match_profiling.find(lost_timestamp);
                    if (iter1 != match_profiling.end()) {
                        lost_senddata = iter1->second;
                        match_profiling.erase(iter1);
                    }

                    ProduceProfilingResult(lost_senddata, lost_info, false);
                    current_profiling_fn++;

                    iter = order_match_profiling_id.erase(iter);
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 若出现异常，将异常结果不匹配返回
    std::unique_lock<std::mutex> lock(m_match_mutex);
    for (auto iter : match_profiling) {
        senddata = iter.second;
        ProduceProfilingResult(senddata, info, false);
    }
}

void StreamDataContrlAgent::DeleteUsedData() {
    std::lock_guard<std::mutex> lock(m_src_datas_mutex);
    for (auto iter = m_usedsrc_datas.begin(); iter != m_usedsrc_datas.end(); iter++) {
        if (1 == (*iter).use_count()) {
            // 多重引用的数据都已经使用完毕
            m_usedsrc_datas.erase(iter);
            current_cache_fn--;
            break;
        }
    }
}

void SendWorkNotifyData() {
    NRGrayscaleCameraFrameData tmp;
    auto& data = g_readying_get_data.stream;
    tmp.camera_count = 2;
    tmp.data = (uint8_t*)data->left_right_frame.data();

    tmp.cameras[0].offset = 0;
    tmp.cameras[0].exposure_start_time_system = data->nano_time;
    if (g_camera_params_type == 1) {
        tmp.cameras[0].width = g_camera_params.device1.resolution[0];
        tmp.cameras[0].height = g_camera_params.device1.resolution[1];
        tmp.cameras[0].stride = g_camera_params.device1.resolution[0];
    } else if (g_camera_params_type == 2) {
        tmp.cameras[0].width = g_camera_params_2.cam0.resolution[0];
        tmp.cameras[0].height = g_camera_params_2.cam0.resolution[1];
        tmp.cameras[0].stride = g_camera_params_2.cam0.resolution[0];
    }

    tmp.cameras[1].offset = data->left_right_frame.size() / 2;
    tmp.cameras[1].exposure_start_time_system = data->nano_time;
    if (g_camera_params_type == 1) {
        tmp.cameras[1].width = g_camera_params.device2.resolution[0];
        tmp.cameras[1].height = g_camera_params.device2.resolution[1];
        tmp.cameras[1].stride = g_camera_params.device2.resolution[0];
    } else if (g_camera_params_type == 2) {
        tmp.cameras[1].width = g_camera_params_2.cam1.resolution[0];
        tmp.cameras[1].height = g_camera_params_2.cam1.resolution[1];
        tmp.cameras[1].stride = g_camera_params_2.cam1.resolution[0];
    }

    // sdk内部可能会失败，但是NotifyData无返回值，导致有些数据等待处理超时
    g_provider.NotifyData(256, NR_CHANNEL_DATA_TYPE_GLASSES_GRAYSCALE_CAMERA, (const void*)&tmp,
                          sizeof(NRGrayscaleCameraFrameData));
}

void StreamDataContrlAgent::SendWork() {
    std::chrono::time_point<std::chrono::steady_clock> start;
    std::chrono::time_point<std::chrono::steady_clock> end;
    uint64_t pre_send_consume_time = 0;
    while (false == send_exit) {
        if ((uint64_t)m_send_idle_time > pre_send_consume_time) {
            std::unique_lock<std::mutex> lock(m_send_mutex);
            uint64_t timeout_ms = (uint64_t)m_send_idle_time - pre_send_consume_time;
            m_send_cond.wait_for(lock, std::chrono::milliseconds(timeout_ms));
        }

        start = std::chrono::steady_clock::now();
        bool is_ok = false;
        if (1) {
            std::lock_guard<std::mutex> lock1(m_send_list_mutex);
            if (send_list.size()) {
                g_readying_get_data = send_list.front();
                send_list.pop_front();
                is_ok = true;
            }
        }

        if (is_ok) {
            // std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> start =
            //     std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
            // auto tp = start.time_since_epoch().count();
            // std::cout << "SendWork tp=" << tp << "sending over!!" << std::endl;
            // std::cout << "SendWork frame_id=" << g_readying_get_data.stream->frame_id << "sending over!!" <<
            // std::endl; std::cout << "SendWork nano_time=" << g_readying_get_data.stream->nano_time << "sending
            // over!!"
            //           << std::endl;
            // std::cout << "SendWork simulation_nano_time=" << g_readying_get_data.simulation_nano_time
            //           << "sending over!!" << std::endl;

            g_readying_get_data.frame_id = g_readying_get_data.stream->frame_id;
            g_readying_get_data.server_actual_nano_time = 0;
            g_readying_get_data.src_data_nano_time = g_readying_get_data.stream->nano_time;

            if (need_profiling) {
                std::lock_guard<std::mutex> lock2(m_match_mutex);
                match_profiling[g_readying_get_data.simulation_nano_time] = g_readying_get_data;
                match_profiling[g_readying_get_data.simulation_nano_time].stream = nullptr;
                order_match_profiling_id[g_readying_get_data.increase_profiline_id] =
                    g_readying_get_data.simulation_nano_time;
            }

            // SendWorkNotifyData发送失败但不报错
            SendWorkNotifyData();
            g_readying_get_data.stream = nullptr;
            DeleteUsedData();
            current_send_fn++;

            if (false == need_profiling) {
                ProfilingInfo info;
                ProduceProfilingResult(g_readying_get_data, info, false);
            }
        }

        end = std::chrono::steady_clock::now();
        pre_send_consume_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
}

int StreamDataContrlAgent::ProduceHandTrackingResult(uint64_t simulation_nano_time) {
    uint64_t hmd_time_nanos = simulation_nano_time + m_sdk_config.prediction_forward_ms * 1000000;
    uint32_t out_hand_num = 2;
    Json::Value json_result;
    if (NR_PLUGIN_RESULT_SUCCESS == g_provider.GetHandData(256, hmd_time_nanos, g_out_hand_array, &out_hand_num)) {
        json_result["exec_status"] = "finish";
        for (uint32_t i = 0; i < 2; i++) {
            Json::Value root;
            root["version"] = g_out_hand_array[i].version;
            root["is_tracked"] = g_out_hand_array[i].is_tracked;
            root["confidence"] = g_out_hand_array[i].confidence;
            root["hand_type"] = g_out_hand_array[i].hand_type;
            root["hand_type"] = g_out_hand_array[i].hand_type;
            root["gesture_type"] = g_out_hand_array[i].gesture_type;
            root["handjoint_count"] = g_out_hand_array[i].hand_joint_count;
            for (uint32_t j = 0; j < g_out_hand_array[i].hand_joint_count; j++) {
                root["hand_joint_data"][j]["version"] = g_out_hand_array[i].hand_joint_data[j].version;
                root["hand_joint_data"][j]["hand_joint_type"] = g_out_hand_array[i].hand_joint_data[j].hand_joint_type;

                root["hand_joint_data"][j]["hand_joint_pose"]["rotation"].append(
                    g_out_hand_array[i].hand_joint_data[j].hand_joint_pose.rotation.qx);
                root["hand_joint_data"][j]["hand_joint_pose"]["rotation"].append(
                    g_out_hand_array[i].hand_joint_data[j].hand_joint_pose.rotation.qy);
                root["hand_joint_data"][j]["hand_joint_pose"]["rotation"].append(
                    g_out_hand_array[i].hand_joint_data[j].hand_joint_pose.rotation.qz);
                root["hand_joint_data"][j]["hand_joint_pose"]["rotation"].append(
                    g_out_hand_array[i].hand_joint_data[j].hand_joint_pose.rotation.qw);

                root["hand_joint_data"][j]["hand_joint_pose"]["position"].append(
                    g_out_hand_array[i].hand_joint_data[j].hand_joint_pose.position.x);
                root["hand_joint_data"][j]["hand_joint_pose"]["position"].append(
                    g_out_hand_array[i].hand_joint_data[j].hand_joint_pose.position.y);
                root["hand_joint_data"][j]["hand_joint_pose"]["position"].append(
                    g_out_hand_array[i].hand_joint_data[j].hand_joint_pose.position.z);
            }
            root["reference_image_nano_time"] = g_out_hand_array[i].image_timestamp_nanos;

            if (0 == i) {
                json_result["exec_result"]["left_hand"] = root;
            } else if (1 == i) {
                json_result["exec_result"]["right_hand"] = root;
            }
        }
    } else {
        json_result["exec_status"] = "miss";
        json_result["exec_result"] = Json::Value();
    }

    json_result["exec_sequence"]["server_actual_nano_time"] = 0;
    json_result["exec_sequence"]["simulation_nano_time"] = simulation_nano_time;
    json_result["exec_sequence"]["simulation_prediction_nano_time"] = hmd_time_nanos;

    Json::FastWriter fwriter;
    std::string a = fwriter.write(json_result);

    std::shared_ptr<StreamResult> data = std::make_shared<StreamResult>();
    data->hand_tracking_data = std::move(a);

    std::unique_lock<std::mutex> lock(m_agent_result_mutex);
    agent_result.push_back(data);
    return 0;
}
void StreamDataContrlAgent::RecvWork() {
    std::chrono::time_point<std::chrono::steady_clock> start;
    std::chrono::time_point<std::chrono::steady_clock> end;
    uint64_t pre_recv_consume_time = 0;
    while (false == recv_exit) {
        if ((uint64_t)m_recv_idle_time > pre_recv_consume_time) {
            std::unique_lock<std::mutex> lock(m_recv_mutex);
            uint64_t timeout_ms = (uint64_t)m_recv_idle_time - pre_recv_consume_time;
            m_recv_cond.wait_for(lock, std::chrono::milliseconds(timeout_ms));
        }

        start = std::chrono::steady_clock::now();
        bool is_ok = false;
        RecvData recv_data;
        if (1) {
            std::lock_guard<std::mutex> lock1(m_recv_list_mutex);
            if (recv_list.size()) {
                recv_data = recv_list.front();
                recv_list.pop_front();
                is_ok = true;
            }
        }

        if (is_ok) {
            ProduceHandTrackingResult(recv_data.simulation_nano_time);
            current_recv_fn++;
        }

        end = std::chrono::steady_clock::now();
        pre_recv_consume_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
}

int StreamDataContrlAgent::StartAgent() {
    if (m_sdk_config.stream_sampling_fps != m_sdk_config.simulation_sendframe_fps) {
        m_send_simulation_policy = 1;
    } else {
        m_send_simulation_policy = 0;
    }

    if (m_sdk_config.stream_sampling_fps != m_sdk_config.simulation_getresult_fps) {
        m_recv_simulation_policy = 1;
    } else {
        m_recv_simulation_policy = 0;
    }

    m_send_idle_time = 1000.0f / float(m_sdk_config.simulation_sendframe_fps);
    m_recv_idle_time = 1000.0f / float(m_sdk_config.simulation_getresult_fps);

    max_cache_fn = m_sdk_config.max_cache_fn;
    current_cache_fn = 0;
    first_datas_full = false;
    finish_add_all_datas = false;
    finish_add_all_datas_2 = false;

    send_exit = false;
    recv_exit = false;
    profiling_exit = false;
    m_send_th = std::move(std::thread(&StreamDataContrlAgent::SendWork, this));
    m_recv_th = std::move(std::thread(&StreamDataContrlAgent::RecvWork, this));

    need_profiling = false;
    if (m_profiling_option.export_pipeline_exec_info_jsonstring > 0 && m_plugin_getprofiling) {
        need_profiling = true;
        m_profiling_th = std::move(std::thread(&StreamDataContrlAgent::ProfilingWork, this));
    }

    max_send_fn = 0;
    current_send_fn = 0;
    current_profiling_fn = 0;
    max_recv_fn = 0;
    current_recv_fn = 0;
    current_agent_result_fn = 0;

    return 0;
}

int StreamDataContrlAgent::StopAgent() {
    send_exit = true;
    if (m_send_th.joinable()) {
        m_send_th.join();
    }

    recv_exit = true;
    if (m_recv_th.joinable()) {
        m_recv_th.join();
    }

    profiling_exit = true;
    if (m_profiling_th.joinable()) {
        m_profiling_th.join();
    }

    return 0;
}
/***********************************************************************************/

std::shared_ptr<HandTrackingSdk> g_shared_sdk = nullptr;
std::shared_ptr<HandTrackingSdk> GetHandTrackingInstance() {
    if (nullptr == g_shared_sdk) {
        g_shared_sdk = std::make_shared<HandTrackingSdk>();
    }
    return g_shared_sdk;
}

void DestroyHandTrackingInstance() {
    if (g_shared_sdk) {
        g_shared_sdk = nullptr;
    }
}

HandTrackingSdk::HandTrackingSdk() {
    g_user_interface.GetInterface = &demo_GetInterface;

    g_generic_interface.GetSDKVersion = &generic_GetSDKVersion;
    g_generic_interface.GetActivityInfo = &generic_GetActivityInfo;
    g_generic_interface.GetPluginConfig = &generic_GetPluginConfig;
    g_generic_interface.GetDeviceConfig = &generic_GetDeviceConfig;
    g_generic_interface.GetDeviceMiscConfig = &generic_GetDeviceMiscConfig;
    g_generic_interface.GetGlobalConfig = &generic_GetGlobalConfig;
    g_generic_interface.GetNetworkType = &generic_GetNetworkType;
    g_generic_interface.GetDeviceType = &generic_GetDeviceType;
    g_generic_interface.GetDynamicLibraryPath = &generic_GetDynamicLibraryPath;
    g_generic_interface.GetChannelIdentifier = &generic_GetChannelIdentifier;
    g_generic_interface.SetThreadPriority = &generic_SetThreadPriority;
    g_generic_interface.GetPropertyFlag = &generic_GetPropertyFlag;
    g_generic_interface.UpdateMetricsui64 = &generic_UpdateMetricsui64;
    g_generic_interface.UpdateMetricsui = &generic_UpdateMetricsui;

    g_nr_handtracking_interface.RegisterLifecycleProvider = &handtracking_RegisterLifecycleProvider;
    g_nr_handtracking_interface.RegisterProvider = &handtracking_RegisterProvider;
    g_nr_handtracking_interface.GetDevicePose = &handtracking_GetDevicePose;

    g_hmd_interface.GetComponentFov = &hmd_GetComponentFov;
    g_hmd_interface.GetComponentResolution = &hmd_GetComponentResolution;
    g_hmd_interface.GetComponentRefreshRate = &hmd_GetComponentRefreshRate;
    g_hmd_interface.GetComponentIntrinsic = &hmd_GetComponentIntrinsic;
    g_hmd_interface.GetComponentDistortion = &hmd_GetComponentDistortion;
    g_hmd_interface.GetComponentExtrinsic = &hmd_GetComponentExtrinsic;
    g_hmd_interface.GetComponentPoseFromHead = &hmd_GetComponentPoseFromHead;
    g_hmd_interface.GetComponentFovOverFill = &hmd_GetComponentFovOverFill;
}

HandTrackingSdk::~HandTrackingSdk() {
    if (nullptr != m_plugin_fd) {
        std::cout << "dlclose HandTrackingSdk over1" << std::endl;
        dlclose(m_plugin_fd);
        std::cout << "dlclose HandTrackingSdk over2" << std::endl;
        m_plugin_fd = nullptr;
    }

    std::cout << "dlclose HandTrackingSdk over" << std::endl;
}

int HandTrackingSdk::ProfilingParamsParse(std::string& json_string) {
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(json_string, root)) {
        return -1;
    }
    memset(&m_profiling_option, 0, sizeof(ProfilingOption));
    if (root.isMember("pipeline_debug")) {
        m_profiling_option.pipeline_debug = root["pipeline_debug"].asInt();
    }
    if (root.isMember("pipeline_node_time_statistics")) {
        m_profiling_option.pipeline_node_time_statistics = root["pipeline_node_time_statistics"].asInt();
    }
    if (root.isMember("export_pipeline_node_data")) {
        m_profiling_option.export_pipeline_exec_info_jsonstring = root["export_pipeline_node_data"].asInt();
    }
    if (root.isMember("enable_detect_boxtracker")) {
        m_profiling_option.handtracking_pipeline_exec_enable_detect_boxtracker =
            root["enable_detect_boxtracker"].asInt();
    }
    if (root.isMember("enable_detect_boxsmooth")) {
        m_profiling_option.handtracking_pipeline_exec_enable_detect_boxsmooth = root["enable_detect_boxsmooth"].asInt();
    }
    if (root.isMember("enable_sync_kfpredictor")) {
        m_profiling_option.handtracking_pipeline_exec_enable_sync_kfpredictor = root["enable_sync_kfpredictor"].asInt();
    }
    if (root.isMember("enable_sync_kfpredictor_timems")) {
        m_profiling_option.handtracking_pipeline_exec_enable_sync_kfpredictor_timems =
            root["enable_sync_kfpredictor_timems"].asInt();
    }
    if (root.isMember("enable_sync_world_seqfilter")) {
        m_profiling_option.handtracking_pipeline_exec_enable_sync_world_seqfilter =
            root["enable_sync_world_seqfilter"].asInt();
    }
    return 0;
}

int HandTrackingSdk::CameraParamsParse(std::string& json_string) {
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(json_string, root)) {
        return -1;
    }

    // 2种内外参配置文件
    if (root.isMember("SLAM_camera") && root["SLAM_camera"].isObject()) {
        memset(&m_camera_params, 0, sizeof(m_camera_params));
        auto& SLAM_camera = root["SLAM_camera"];
        bool has_leftcam_2_rightcam = false;

        m_camera_params.num_of_cameras = SLAM_camera["num_of_cameras"].asInt();

        if (SLAM_camera.isMember("leftcam_p_rightcam") && SLAM_camera["leftcam_p_rightcam"].isArray()) {
            auto& leftcam_p_rightcam = SLAM_camera["leftcam_p_rightcam"];

            if (leftcam_p_rightcam.isArray() && leftcam_p_rightcam.size() == 3) {
                for (uint32_t i = 0; i < leftcam_p_rightcam.size(); i++)
                    m_camera_params.leftcam_p_rightcam[i] = (float)leftcam_p_rightcam[i].asDouble();
            }
            has_leftcam_2_rightcam = true;
        }

        if (SLAM_camera.isMember("leftcam_q_rightcam") && SLAM_camera["leftcam_q_rightcam"].isArray()) {
            auto& leftcam_q_rightcam = SLAM_camera["leftcam_q_rightcam"];

            if (leftcam_q_rightcam.isArray() && leftcam_q_rightcam.size() == 4) {
                for (uint32_t i = 0; i < leftcam_q_rightcam.size(); i++)
                    m_camera_params.leftcam_q_rightcam[i] = (float)leftcam_q_rightcam[i].asDouble();
            }
            has_leftcam_2_rightcam = true;
        }

        if (SLAM_camera.isMember("device_1") && SLAM_camera["device_1"].isObject()) {
            auto& device_1 = SLAM_camera["device_1"];
            auto& cc = device_1["cc"];
            auto& fc = device_1["fc"];
            auto& imu_p_cam = device_1["imu_p_cam"];
            auto& imu_q_cam = device_1["imu_q_cam"];
            auto& kc = device_1["kc"];
            auto& resolution = device_1["resolution"];
            auto& camera_model = device_1["camera_model"];

            if (cc.isArray() && cc.size() == 2) {
                for (uint32_t i = 0; i < cc.size(); i++) m_camera_params.device1.cc[i] = (float)cc[i].asDouble();
            }

            if (fc.isArray() && fc.size() == 2) {
                for (uint32_t i = 0; i < fc.size(); i++) m_camera_params.device1.fc[i] = (float)fc[i].asDouble();
            }

            if (imu_p_cam.isArray() && imu_p_cam.size() == 3) {
                for (uint32_t i = 0; i < imu_p_cam.size(); i++)
                    m_camera_params.device1.imu_p_cam[i] = (float)imu_p_cam[i].asDouble();
            }

            if (imu_q_cam.isArray() && imu_q_cam.size() == 4) {
                for (uint32_t i = 0; i < imu_q_cam.size(); i++)
                    m_camera_params.device1.imu_q_cam[i] = (float)imu_q_cam[i].asDouble();
            }

            if (kc.isArray()) {
                for (uint32_t i = 0; i < kc.size() && i < 12; i++)
                    m_camera_params.device1.kc[i] = (float)kc[i].asDouble();
            }

            if (resolution.isArray() && resolution.size() == 2) {
                for (uint32_t i = 0; i < resolution.size(); i++)
                    m_camera_params.device1.resolution[i] = resolution[i].asInt();
            }

            if (camera_model.asString() == "radial") {
                m_camera_params.device1.camera_model = 1;
            } else if (camera_model.asString() == "fisheye") {
                m_camera_params.device1.camera_model = 2;
            } else if (camera_model.asString() == "fisheye624") {
                m_camera_params.device1.camera_model = 3;
            }
        }

        if (SLAM_camera.isMember("device_2") && SLAM_camera["device_2"].isObject()) {
            auto& device_2 = SLAM_camera["device_2"];
            auto& cc = device_2["cc"];
            auto& fc = device_2["fc"];
            auto& imu_p_cam = device_2["imu_p_cam"];
            auto& imu_q_cam = device_2["imu_q_cam"];
            auto& kc = device_2["kc"];
            auto& resolution = device_2["resolution"];
            auto& camera_model = device_2["camera_model"];

            if (cc.isArray() && cc.size() == 2) {
                for (uint32_t i = 0; i < cc.size(); i++) m_camera_params.device2.cc[i] = (float)cc[i].asDouble();
            }

            if (fc.isArray() && fc.size() == 2) {
                for (uint32_t i = 0; i < fc.size(); i++) m_camera_params.device2.fc[i] = (float)fc[i].asDouble();
            }

            if (imu_p_cam.isArray() && imu_p_cam.size() == 3) {
                for (uint32_t i = 0; i < imu_p_cam.size(); i++)
                    m_camera_params.device2.imu_p_cam[i] = (float)imu_p_cam[i].asDouble();
            }

            if (imu_q_cam.isArray() && imu_q_cam.size() == 4) {
                for (uint32_t i = 0; i < imu_q_cam.size(); i++)
                    m_camera_params.device2.imu_q_cam[i] = (float)imu_q_cam[i].asDouble();
            }

            if (kc.isArray()) {
                for (uint32_t i = 0; i < kc.size() && i < 12; i++)
                    m_camera_params.device2.kc[i] = (float)kc[i].asDouble();
            }

            if (resolution.isArray() && resolution.size() == 2) {
                for (uint32_t i = 0; i < resolution.size(); i++)
                    m_camera_params.device2.resolution[i] = resolution[i].asInt();
            }

            if (camera_model.asString() == "radial") {
                m_camera_params.device2.camera_model = 1;
            } else if (camera_model.asString() == "fisheye") {
                m_camera_params.device2.camera_model = 2;
            } else if (camera_model.asString() == "fisheye624") {
                m_camera_params.device2.camera_model = 3;
            }
        }

        if (false == has_leftcam_2_rightcam) {
            Eigen::Quaternion<float> left2head_rotate(
                m_camera_params.device1.imu_q_cam[3], -m_camera_params.device1.imu_q_cam[0],
                -m_camera_params.device1.imu_q_cam[1], -m_camera_params.device1.imu_q_cam[2]);

            Eigen::Isometry3f left2head = Eigen::Isometry3f::Identity();
            left2head.rotate(left2head_rotate);
            left2head.pretranslate(Eigen::Vector3f(m_camera_params.device1.imu_p_cam[0],
                                                   m_camera_params.device1.imu_p_cam[1],
                                                   m_camera_params.device1.imu_p_cam[2]));

            Eigen::Quaternion<float> right2head_rotate(
                m_camera_params.device2.imu_q_cam[3], -m_camera_params.device2.imu_q_cam[0],
                -m_camera_params.device2.imu_q_cam[1], -m_camera_params.device2.imu_q_cam[2]);

            Eigen::Isometry3f right2head = Eigen::Isometry3f::Identity();
            right2head.rotate(right2head_rotate);
            right2head.pretranslate(Eigen::Vector3f(m_camera_params.device2.imu_p_cam[0],
                                                    m_camera_params.device2.imu_p_cam[1],
                                                    m_camera_params.device2.imu_p_cam[2]));

            auto right2left = left2head.inverse() * right2head;

            Eigen::Matrix3f rotation_matrix = right2left.linear();
            Eigen::Vector3f translation = right2left.translation();

            Eigen::Quaternionf quaternion(rotation_matrix);
            std::cout << "Hamilton Quaternion: " << quaternion.w() << " " << quaternion.x() << " " << quaternion.y()
                      << " " << quaternion.z() << std::endl;
            std::cout << "Translation: " << translation.transpose() << std::endl;

            for (uint32_t i = 0; i < 3; i++) {
                m_camera_params.leftcam_p_rightcam[i] = translation[i];
            }
            m_camera_params.leftcam_q_rightcam[0] = quaternion.x();
            m_camera_params.leftcam_q_rightcam[1] = quaternion.y();
            m_camera_params.leftcam_q_rightcam[2] = quaternion.z();
            m_camera_params.leftcam_q_rightcam[3] = quaternion.w();
        }

        g_camera_params_type = 1;
        g_camera_params = m_camera_params;

        return 0;
    } else if (root.isMember("frame_list") && root["frame_list"].isArray() && root.isMember("meta") &&
               root["meta"].isObject()) {
        memset(&m_camera_params_2, 0, sizeof(m_camera_params_2));

        auto& frame_list = root["frame_list"];
        if (frame_list.size()) {
            // 这里仅支持读取第1帧的参数，作为全局参数
            auto& first_frame = frame_list[0];

            if (first_frame.isMember("left") && first_frame["left"].isObject()) {
                auto& left = first_frame["left"];
                auto& image = left["image"];
                m_camera_params_2.cam0.resolution[0] = image["width"].asInt();
                m_camera_params_2.cam0.resolution[1] = image["height"].asInt();
            }

            if (first_frame.isMember("right") && first_frame["right"].isObject()) {
                auto& right = first_frame["right"];
                auto& image = right["image"];
                m_camera_params_2.cam1.resolution[0] = image["width"].asInt();
                m_camera_params_2.cam1.resolution[1] = image["height"].asInt();
            }
        }

        bool camera_param_abnormal = false;
        auto& meta = root["meta"];
        if (meta.isMember("cam0_K") && meta["cam0_K"].isString()) {
            auto& intrinsi = meta["cam0_K"];
            std::vector<std::string> ss = absl::StrSplit(intrinsi.asString(), ',');
            for (uint32_t i = 0; i < 3; i++) {
                for (uint32_t j = 0; j < 3; j++) {
                    m_camera_params_2.cam0.intrinsic[i][j] = std::atof(ss[i * 3 + j].c_str());
                }
            }
        } else {
            std::vector<std::string> ss = absl::StrSplit(
                "240.81756214390214,0.0,237.98274935930652,0.0,240.79333795688873,318.76245466163476,0.0,0.0,1.0", ',');
            for (uint32_t i = 0; i < 3; i++) {
                for (uint32_t j = 0; j < 3; j++) {
                    m_camera_params_2.cam0.intrinsic[i][j] = std::atof(ss[i * 3 + j].c_str());
                }
            }
            camera_param_abnormal = true;
        }

        int fake_camera_model = 2;
        if (meta.isMember("cam0_D") && meta["cam0_D"].isString()) {
            auto& distortion = meta["cam0_D"];
            std::vector<std::string> ss = absl::StrSplit(distortion.asString(), ",");
            for (uint32_t i = 0; i < ss.size() && i < 12; i++) {
                m_camera_params_2.cam0.distortion[i] = std::atof(ss[i].c_str());
            }
            if (ss.size() == 12) {
                fake_camera_model = 3;
            }
        } else {
            std::vector<std::string> ss = absl::StrSplit(
                "0.023569054999727224,0.021582533813185853,-0.025507559689771916,0.005611027745777131", ",");
            for (uint32_t i = 0; i < ss.size() && i < 12; i++) {
                m_camera_params_2.cam0.distortion[i] = std::atof(ss[i].c_str());
            }
            camera_param_abnormal = true;
        }

        if (meta.isMember("cam1_K") && meta["cam1_K"].isString()) {
            auto& intrinsi = meta["cam1_K"];
            std::vector<std::string> ss = absl::StrSplit(intrinsi.asString(), ",");
            for (uint32_t i = 0; i < 3; i++) {
                for (uint32_t j = 0; j < 3; j++) {
                    m_camera_params_2.cam1.intrinsic[i][j] = std::atof(ss[i * 3 + j].c_str());
                }
            }
        } else {
            std::vector<std::string> ss = absl::StrSplit(
                "240.58995609594845,0.0,240.28424769355726,0.0,240.76679262752728,321.00347968733934,0.0,0.0,1.0", ",");
            for (uint32_t i = 0; i < 3; i++) {
                for (uint32_t j = 0; j < 3; j++) {
                    m_camera_params_2.cam1.intrinsic[i][j] = std::atof(ss[i * 3 + j].c_str());
                }
            }
            camera_param_abnormal = true;
        }

        if (meta.isMember("cam1_D") && meta["cam1_D"].isString()) {
            auto& distortion = meta["cam1_D"];
            std::vector<std::string> ss = absl::StrSplit(distortion.asString(), ",");
            for (uint32_t i = 0; i < ss.size() && i < 12; i++) {
                m_camera_params_2.cam1.distortion[i] = std::atof(ss[i].c_str());
            }
        } else {
            std::vector<std::string> ss = absl::StrSplit(
                "0.01936875955032054,0.02901991322304378,-0.031047384937997684,0.007004692495123601", ",");
            for (uint32_t i = 0; i < ss.size() && i < 12; i++) {
                m_camera_params_2.cam1.distortion[i] = std::atof(ss[i].c_str());
            }
            camera_param_abnormal = true;
        }

        if (meta.isMember("stereo_param") && meta["stereo_param"].isString()) {
            auto& stereo_param = meta["stereo_param"];
            std::vector<std::string> ss = absl::StrSplit(stereo_param.asString(), ",");
            for (uint32_t i = 0; i < 4; i++) {
                for (uint32_t j = 0; j < 4; j++) {
                    m_camera_params_2.cam1_to_cam0_extrinsic[i][j] = std::atof(ss[i * 4 + j].c_str());
                }
            }
        } else {
            std::vector<std::string> ss = absl::StrSplit(
                "0.9873977883469949,0.004032815932308337,0.15820664955406175,0.13596588083005973,-0.002289591541374757,"
                "0.999934663805656,-0.011199370091501076,0.0023897405414088774,-0.15824147793179813,0."
                "010696004652618439,0.9873425090344452,-0.011478675275743533,0.0,0.0,0.0,1.0",
                ",");
            for (uint32_t i = 0; i < 4; i++) {
                for (uint32_t j = 0; j < 4; j++) {
                    m_camera_params_2.cam1_to_cam0_extrinsic[i][j] = std::atof(ss[i * 4 + j].c_str());
                }
            }
            camera_param_abnormal = true;
        }

        if (meta.isMember("leftcam_q_rightcam") && meta["leftcam_q_rightcam"].isString()) {
            auto& leftcam_q_rightcam = meta["leftcam_q_rightcam"];
            std::vector<std::string> ss = absl::StrSplit(leftcam_q_rightcam.asString(), ",");
            for (uint32_t j = 0; j < 4; j++) {
                m_camera_params_2.cam1_to_cam0_rotation[j] = std::atof(ss[j].c_str());
            }
        } else {
            std::vector<std::string> ss =
                absl::StrSplit("0.005491254567372182,0.0793636667881101,-0.001585629354253212,0.9968293436174384", ",");
            for (uint32_t j = 0; j < 4; j++) {
                m_camera_params_2.cam1_to_cam0_rotation[j] = std::atof(ss[j].c_str());
            }
            camera_param_abnormal = true;
        }

        if (meta.isMember("leftcam_p_rightcam") && meta["leftcam_p_rightcam"].isString()) {
            auto& leftcam_p_rightcam = meta["leftcam_p_rightcam"];
            std::vector<std::string> ss = absl::StrSplit(leftcam_p_rightcam.asString(), ",");
            for (uint32_t j = 0; j < 3; j++) {
                m_camera_params_2.cam1_to_cam0_position[j] = std::atof(ss[j].c_str());
            }
        } else {
            std::vector<std::string> ss =
                absl::StrSplit("0.13596588083005973,0.0023897405414088774,-0.011478675275743533", ",");
            for (uint32_t j = 0; j < 3; j++) {
                m_camera_params_2.cam1_to_cam0_position[j] = std::atof(ss[j].c_str());
            }
            camera_param_abnormal = true;
        }

        if (camera_param_abnormal) {
            std::cout << "[ERROR] camera_param_abnormal !!!" << std::endl;
            std::cout << "[ERROR] camera_param_abnormal !!!" << std::endl;
            std::cout << "[ERROR] camera_param_abnormal !!!" << std::endl;
            std::cout << "[ERROR] camera_param_abnormal !!!" << std::endl;
            return -1;
        }

        // 这里需要实现
        m_camera_params_2.cam0.camera_model = fake_camera_model;
        m_camera_params_2.cam1.camera_model = fake_camera_model;

        g_camera_params_type = 2;
        g_camera_params_2 = m_camera_params_2;
        return 0;
    }

    return -1;
}

int HandTrackingSdk::StartSdk(SdkConfigParams& sdk_config) {
    m_sdk_config = std::move(sdk_config);
    for (auto iter = m_sdk_config.config_params.begin(); iter != m_sdk_config.config_params.end(); iter++) {
        if (iter->first == "plugin_so") {
            m_plugin_so = iter->second;
        } else if (iter->first == "camera_param") {
            auto ok = CameraParamsParse(iter->second);
            if (0 != ok) {
                std::cout << "StartSdk CameraParamsParse" << std::endl;
                return -1;
            }
        } else if (iter->first == "highlevel_option") {
            ProfilingParamsParse(iter->second);
        }
    }

    m_plugin_fd = dlopen(m_plugin_so.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (nullptr == m_plugin_fd) {
        std::cout << "dlopen m_plugin_so " << std::string(dlerror()) << std::endl;
        return -1;
    }

    m_plugin_create = (_NRPluginCreate)dlsym(m_plugin_fd, "NRPluginCreate");
    if (m_plugin_create == nullptr) {
        std::cout << "dlsym NRPluginCreate" << std::string(dlerror()) << std::endl;
        return -1;
    }

    m_plugin_unload = (_NRPluginUnload)dlsym(m_plugin_fd, "NRPluginDestroy");
    if (m_plugin_unload == nullptr) {
        std::cout << "dlsym NRPluginDestroy " << std::string(dlerror()) << std::endl;
        return -1;
    }

    m_plugin_setprofiling = (_NRPluginSetProfilingOption)dlsym(m_plugin_fd, "NRPluginSetProfilingOption");
    if (m_plugin_setprofiling == nullptr) {
        std::cout << "dlsym NRPluginSetProfilingOption " << std::string(dlerror()) << std::endl;
        return -1;
    }

    m_plugin_getprofiling = (_NRPluginGetProfilingInfo)dlsym(m_plugin_fd, "NRPluginGetProfilingInfo");
    if (m_plugin_getprofiling == nullptr) {
        std::cout << "dlsym NRPluginGetProfilingInfo " << std::string(dlerror()) << std::endl;
    }

    if (g_camera_params_type == 1) {
        m_profiling_option.camera_model = m_camera_params.device1.camera_model;
        m_profiling_option.generate_method = 2;
    } else if (g_camera_params_type == 2) {
        m_profiling_option.camera_model = m_camera_params_2.cam0.camera_model;
        m_profiling_option.generate_method = 2;
    }

    NRPluginHandle handle = 256;
    m_plugin_create(handle, &g_user_interface);
    if (m_plugin_setprofiling) {
        m_plugin_setprofiling(PROFILING_FLAG, &m_profiling_option);
    }

    NRPluginResult ret;
    ret = g_lifecycle_provider.Initialize(handle);
    if (ret != NR_PLUGIN_RESULT_SUCCESS) {
        g_lifecycle_provider.Release(handle);
        return -1;
    }

    ret = g_lifecycle_provider.Start(handle);
    if (ret != NR_PLUGIN_RESULT_SUCCESS) {
        std::cout << "g_lifecycle_provider.Start error " << std::endl;
        g_lifecycle_provider.Stop(handle);
        g_lifecycle_provider.Release(handle);
        return -1;
    }

    StartAgent();
    m_sdk_started = true;
    return 0;
}

int HandTrackingSdk::StopSdk() {
    if (m_sdk_started) {
        StopAgent();

        NRPluginHandle handle = 256;
        g_lifecycle_provider.Stop(handle);
        g_lifecycle_provider.Release(handle);
        m_sdk_started = false;
        std::cout << "Stop HandTracking Sdk over" << std::endl;
        return 0;
    }
    return 0;
}

int HandTrackingSdk::SendStream(std::shared_ptr<StreamData>& data) {
    if (false == m_sdk_started) {
        return -1;
    }

    return AddAgentData(data);
}

int HandTrackingSdk::RecvResult(uint64_t frame_id, std::shared_ptr<StreamResult>& result) {
    if (false == m_sdk_started) {
        return -1;
    }

    return GetAgentResult(frame_id, result);
}