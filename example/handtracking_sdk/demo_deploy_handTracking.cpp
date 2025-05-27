#include "demo_deploy_handTracking.h"

// c std
#include <dlfcn.h>

// c++ std
#include <iostream>

// other lib .h
#include <Eigen/Dense>
#include <Eigen/Geometry>

#include "json/json.h"

// sort from a-z
#include "channel/nr_plugin_grayscale_camera_types.h"
#include "common/nr_plugin_generic.h"
#include "common/nr_plugin_hmd.h"
#include "common/nr_plugin_types_ext.inl"
#include "plugin/nr_perception_hand_tracking.h"
#include "plugin/nr_plugin_tracking_common.h"
#include "public/nr_plugin_lifecycle.h"

HandTrackingInterface g_nr_handtracking_interface;
NRGenericInterface g_generic_interface;
NRHMDInterface g_hmd_interface;
NRInterfaces g_user_interface;

NRPluginLifecycleProvider g_lifecycle_provider;
HandTrackingProvider g_provider;

CameraParams g_camera_params;
NRDeviceType g_device_type;

// #define JOINTS_COUNT 25
// #define KPT_NUMS 21
HandData g_out_hand_array[2];
/*********************************plugin**********************************************/

NRInterface *demo_GetInterface(NRInterfaceGUID guid, unsigned long long *out_interface_size) {
    (void)out_interface_size;
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

NRPluginResult handtracking_RegisterLifecycleProvider(NRPluginHandle handle, const char *plugin_id,
                                                      const char *plugin_version,
                                                      const NRPluginLifecycleProvider *provider,
                                                      uint32_t provider_size) {
    (void)handle;
    (void)plugin_id;
    (void)plugin_version;
    (void)provider_size;
    g_lifecycle_provider = *provider;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult handtracking_RegisterProvider(NRPluginHandle handle, const HandTrackingProvider *provider,
                                             uint32_t provider_size) {
    (void)handle;
    (void)provider_size;
    g_provider = *provider;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult handtracking_GetDevicePose(NRPluginHandle handle, DevicePose *device_pose, uint64_t hmd_time_nanos) {
    (void)handle;
    (void)hmd_time_nanos;

    device_pose->transform.position.x = 0.0f;
    device_pose->transform.position.y = 0.0f;
    device_pose->transform.position.z = 0.0f;

    device_pose->transform.rotation.qx = 0.0f;
    device_pose->transform.rotation.qy = 0.0f;
    device_pose->transform.rotation.qz = 0.0f;
    device_pose->transform.rotation.qw = 1.0f;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult hmd_GetComponentFov(NRPluginHandle handle, NRComponent component, NRFov4f *out_fov) {
    (void)handle;
    (void)component;
    (void)out_fov;

    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult hmd_GetComponentResolution(NRPluginHandle handle, NRComponent device, NRSize2i *out_device_resolution) {
    (void)handle;

    if (NR_COMPONENT_GRAYSCALE_CAMERA_LEFT == device) {
        out_device_resolution->width = g_camera_params.device1.resolution[0];
        out_device_resolution->height = g_camera_params.device1.resolution[1];
    } else if (NR_COMPONENT_GRAYSCALE_CAMERA_RIGHT == device) {
        out_device_resolution->width = g_camera_params.device2.resolution[0];
        out_device_resolution->height = g_camera_params.device2.resolution[1];
    }
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult hmd_GetComponentRefreshRate(NRPluginHandle handle, NRComponent component, uint32_t *out_refresh_rate) {
    (void)handle;
    (void)component;
    (void)out_refresh_rate;

    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult hmd_GetComponentIntrinsic(NRPluginHandle handle, NRComponent device, NRMat3f *out_intrinsic_matrix) {
    (void)handle;

    memset(out_intrinsic_matrix, 0, sizeof(NRMat3f));
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

    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult hmd_GetComponentDistortion(NRPluginHandle handle, NRComponent device, NRCameraDistortion *out_params) {
    (void)handle;
    memset(out_params, 0, sizeof(NRCameraDistortion));
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

// 接口Quaternion的顺序是qw,qx,qy,qz
void Hamilton_Quaternion_To_Rotation_Matrix(double *Quaternion, double *rt_mat) {
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

NRPluginResult hmd_GetComponentExtrinsic(NRHandle token, NRComponent source_device_name, NRComponent target_device_name,
                                         NRTransform *out_extrinsic) {
    (void)token;
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
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult hmd_GetComponentPoseFromHead(NRPluginHandle handle, NRComponent component, NRTransform *out_transform) {
    (void)handle;
    (void)component;
    (void)out_transform;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult hmd_GetComponentFovOverFill(NRPluginHandle handle, NRComponent component, NRFov4f *out_fov_overfill) {
    (void)handle;
    (void)component;
    (void)out_fov_overfill;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetSDKVersion(NRVersion *version) {
    version->major = 1;
    version->minor = 0;
    version->revision = 0;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetActivityInfo(void **java_vm, void **context, void **class_loader) {
    *java_vm = nullptr;
    *context = nullptr;
    *class_loader = nullptr;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetPluginConfig(const char **data, uint32_t *data_size) {
    *data = nullptr;
    *data_size = 0;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetDeviceConfig(NRPluginHandle handle, const char **data, uint32_t *data_size) {
    (void)handle;
    *data = nullptr;
    *data_size = 0;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetDeviceMiscConfig(NRPluginHandle handle, const char **data, uint32_t *data_size) {
    (void)handle;
    *data = nullptr;
    *data_size = 0;
    return NR_PLUGIN_RESULT_SUCCESS;
}

extern int ReadFromFile(std::string &file_name, std::string &content);
NRPluginResult generic_GetGlobalConfig(NRPluginHandle handle, const char **data, uint32_t *data_size) {
    (void)handle;
    static std::string global_config;
    std::string name("./sdk_global.json");
    if (0 == ReadFromFile(name, global_config)) {
        *data = global_config.data();
        *data_size = global_config.size();
    } else {
        *data = nullptr;
        *data_size = 0;
    }
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetNetworkType(NRPluginHandle handle, NRNetworkType *network_type) {
    (void)handle;
    (void)network_type;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetDeviceType(NRPluginHandle handle, NRDeviceType *device_type) {
    (void)handle;
    *device_type = g_device_type;
    std::cout << "device_type: " << static_cast<int>(*device_type) << std::endl;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetDynamicLibraryPath(NRPluginHandle handle, const char **out_path, uint32_t *path_size) {
    (void)handle;
    *out_path = nullptr;
    *path_size = 0;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetChannelIdentifier(NRPluginHandle handle, const char **channel_identifier,
                                            uint32_t *channel_identifier_size) {
    (void)handle;
    *channel_identifier = nullptr;
    *channel_identifier_size = 0;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_SetThreadPriority(NRPluginHandle handle, int32_t tid, int32_t policy, int32_t priority) {
    (void)handle;
    (void)tid;
    (void)policy;
    (void)priority;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_GetPropertyFlag(NRPluginHandle handle, NRProperty property, int32_t *flag) {
    (void)handle;
    (void)property;
    (void)flag;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_UpdateMetricsui64(NRPluginHandle handle, NRMetricsType metrics_type, uint64_t metrics_data) {
    (void)handle;
    (void)metrics_type;
    (void)metrics_data;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult generic_UpdateMetricsui(NRPluginHandle handle, NRMetricsType metrics_type, uint32_t metrics_data) {
    (void)handle;
    (void)metrics_type;
    (void)metrics_data;
    return NR_PLUGIN_RESULT_SUCCESS;
}

/***********************************************************************************/
int HandTrackingSdk::m_camera_number = 2;  //默认情况下是双目

HandTrackingSdk &HandTrackingSdk::GetHandTrackingInstance() {
    static HandTrackingSdk instance;
    return instance;
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
        dlclose(m_plugin_fd);
        m_plugin_fd = nullptr;
    }

    std::cout << "dlclose HandTrackingSdk over" << std::endl;
}

std::vector<std::string> SplitString(const std::string &str, const std::string &pattern) {
    std::vector<std::string> resultVec;
    uint32_t prestart = 0;
    uint32_t start = 0;
    uint32_t end = 0;
    for (start = 0; start < str.size();) {
        end = start + pattern.size();
        if (end <= str.size()) {
            if (str.substr(start, end - start) == pattern) {
                resultVec.push_back(str.substr(prestart, start - prestart));
                prestart = start = end;
                continue;
            } else {
                start++;
                end = start + pattern.size();
            }
        }

        if (start == str.size() || end >= str.size()) {
            resultVec.push_back(str.substr(prestart));
            break;
        }
    }

    return resultVec;
};

void processDevice(CameraDevice &device, const Json::Value &meta, const std::string &K_key, const std::string &D_key) {
    // camera_model
    auto &camera_model = meta["camera_model"];
    if (camera_model.asString() == "radial") {
        device.camera_model = 1;
    } else if (camera_model.asString() == "fisheye") {
        device.camera_model = 2;
    } else if (camera_model.asString() == "fisheye624") {
        device.camera_model = 3;
    }

    // resolution
    auto &resolution = meta["resolution"];
    device.resolution[0] = resolution[0].asInt();
    device.resolution[1] = resolution[1].asInt();

    // K -> fc cc
    if (meta.isMember(K_key) && meta[K_key].isString()) {
        auto &K = meta[K_key];
        std::vector<std::string> ss = SplitString(K.asString(), ",");

        device.fc[0] = std::atof(ss[0].c_str());
        device.fc[1] = std::atof(ss[4].c_str());

        device.cc[0] = std::atof(ss[2].c_str());
        device.cc[1] = std::atof(ss[5].c_str());
    }

    // D -> kc
    if (meta.isMember(D_key) && meta[D_key].isString()) {
        auto &D = meta[D_key];
        std::vector<std::string> ss = SplitString(D.asString(), ",");
        for (uint32_t i = 0; i < ss.size() && i < 12; i++) {
            device.kc[i] = std::atof(ss[i].c_str());
        }
    }

    for (uint32_t i = 0; i < 3; i++) {
        device.imu_p_cam[i] = 0.0f;
        device.imu_q_cam[i] = 0.0f;
    }
    device.imu_q_cam[3] = 1.0f;
}

int HandTrackingSdk::CameraParamsParse(std::string &json_string) {
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(json_string, root)) {
        return -1;
    }

    bool has_leftcam_2_rightcam = false;
    memset(&m_camera_params, 0, sizeof(m_camera_params));
    if (root.isMember("SLAM_camera") && root["SLAM_camera"].isObject()) {
        auto &SLAM_camera = root["SLAM_camera"];

        m_camera_params.num_of_cameras = SLAM_camera["num_of_cameras"].asInt();

        if (SLAM_camera.isMember("leftcam_p_rightcam") && SLAM_camera["leftcam_p_rightcam"].isArray()) {
            auto &leftcam_p_rightcam = SLAM_camera["leftcam_p_rightcam"];

            if (leftcam_p_rightcam.isArray() && leftcam_p_rightcam.size() == 3) {
                for (uint32_t i = 0; i < leftcam_p_rightcam.size(); i++)
                    m_camera_params.leftcam_p_rightcam[i] = (float)leftcam_p_rightcam[i].asDouble();
            }
            has_leftcam_2_rightcam = true;
        }

        if (SLAM_camera.isMember("leftcam_q_rightcam") && SLAM_camera["leftcam_q_rightcam"].isArray()) {
            auto &leftcam_q_rightcam = SLAM_camera["leftcam_q_rightcam"];

            if (leftcam_q_rightcam.isArray() && leftcam_q_rightcam.size() == 4) {
                for (uint32_t i = 0; i < leftcam_q_rightcam.size(); i++)
                    m_camera_params.leftcam_q_rightcam[i] = (float)leftcam_q_rightcam[i].asDouble();
            }
            has_leftcam_2_rightcam = true;
        }

        if (SLAM_camera.isMember("device_1") && SLAM_camera["device_1"].isObject()) {
            auto &device_1 = SLAM_camera["device_1"];
            auto &cc = device_1["cc"];
            auto &fc = device_1["fc"];
            auto &imu_p_cam = device_1["imu_p_cam"];
            auto &imu_q_cam = device_1["imu_q_cam"];
            auto &kc = device_1["kc"];
            auto &resolution = device_1["resolution"];
            auto &camera_model = device_1["camera_model"];

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
            auto &device_2 = SLAM_camera["device_2"];
            auto &cc = device_2["cc"];
            auto &fc = device_2["fc"];
            auto &imu_p_cam = device_2["imu_p_cam"];
            auto &imu_q_cam = device_2["imu_q_cam"];
            auto &kc = device_2["kc"];
            auto &resolution = device_2["resolution"];
            auto &camera_model = device_2["camera_model"];

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
    } else if (root.isMember("meta") && root["meta"].isObject()) {
        auto &meta = root["meta"];

        // num_of_cameras
        m_camera_params.num_of_cameras = meta["num_of_cameras"].asInt();

        // leftcam_q_rightcam (R 4d)
        if (meta.isMember("leftcam_q_rightcam") && meta["leftcam_q_rightcam"].isString()) {
            auto &leftcam_q_rightcam = meta["leftcam_q_rightcam"];
            std::vector<std::string> ss = SplitString(leftcam_q_rightcam.asString(), ",");
            for (uint32_t j = 0; j < 4; j++) {
                m_camera_params.leftcam_q_rightcam[j] = std::atof(ss[j].c_str());
            }
        }

        // leftcam_p_rightcam (t 3d)
        if (meta.isMember("leftcam_p_rightcam") && meta["leftcam_p_rightcam"].isString()) {
            auto &leftcam_p_rightcam = meta["leftcam_p_rightcam"];
            std::vector<std::string> ss = SplitString(leftcam_p_rightcam.asString(), ",");
            for (uint32_t j = 0; j < 3; j++) {
                m_camera_params.leftcam_p_rightcam[j] = std::atof(ss[j].c_str());
            }
        }

        processDevice(m_camera_params.device1, meta, "cam0_K", "cam0_D");
        processDevice(m_camera_params.device2, meta, "cam1_K", "cam1_D");
    }

    // 获取眼镜平台
    if (root.isMember("device_type") && root["device_type"].isInt()) {
        g_device_type = static_cast<NRDeviceType>(root["device_type"].asInt());
    }

    g_camera_params = m_camera_params;
    return 0;
}

/// @brief 手势sdk启动接口
/// @param config_params 相关配置参数
/// @return 0/-1 0表示成功，-1表示失败
int HandTrackingSdk::StartSdk(std::map<std::string, std::string> &config_params) {
    for (auto iter = config_params.begin(); iter != config_params.end(); iter++) {
        if (iter->first == "plugin_so") {
            m_plugin_so = std::string("./handTracking/") + iter->second;
        } else if (iter->first == "camera_param") {
            CameraParamsParse(iter->second);
        }
    }

    //加载动态库，并加载对应接口
    std::cout << "m_plugin_so: " << m_plugin_so << std::endl;
    m_plugin_fd = dlopen(m_plugin_so.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (nullptr == m_plugin_fd) {
        std::cout << "dlopen m_plugin_so" << std::string(dlerror()) << std::endl;
        return -1;
    }

    m_plugin_create = (_NRPluginCreate)dlsym(m_plugin_fd, "NRPluginCreate");
    if (m_plugin_create == nullptr) {
        std::cout << "dlsym NRPluginCreate" << std::string(dlerror()) << std::endl;
        return -1;
    }

    m_plugin_unload = (_NRPluginUnload)dlsym(m_plugin_fd, "NRPluginDestroy");
    if (m_plugin_unload == nullptr) {
        std::cout << "dlsym NRPluginDestroy" << std::string(dlerror()) << std::endl;
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
        return -1;
    }

    // memset(&m_profiling_option, 0, sizeof(ProfilingOption));
    m_profiling_option.struct_bytes = 0;
    m_profiling_option.aisdk_init_report = 1;
    m_profiling_option.pipeline_debug = 0;
    // m_profiling_option.pipeline_node_time_statistics = 0;
    m_profiling_option.export_pipeline_exec_info_jsonstring = 0;
    // m_profiling_option.handtracking_pipeline_exec_enable_detect_boxtracker = 1;
    // m_profiling_option.handtracking_pipeline_exec_enable_detect_boxsmooth = 1;
    // m_profiling_option.handtracking_pipeline_exec_enable_sync_kfpredictor = 0;
    // m_profiling_option.handtracking_pipeline_exec_enable_sync_kfpredictor_timems = 0;
    // m_profiling_option.handtracking_pipeline_exec_enable_sync_world_seqfilter = 0;
    m_profiling_option.developer_test_all = 0;

    m_profiling_option.camera_model = m_camera_params.device1.camera_model;
    m_profiling_option.generate_method = 2;

    std::cout << "create handle" << std::endl;
    NRPluginHandle handle = 256;
    m_plugin_create(handle, &g_user_interface);
    if (m_plugin_setprofiling) {
        m_plugin_setprofiling(PROFILING_FLAG, &m_profiling_option);
    }

    std::cout << "init handle" << std::endl;
    NRPluginResult ret = g_lifecycle_provider.Initialize(handle);
    if (ret != NR_PLUGIN_RESULT_SUCCESS) {
        std::cout << "initialize handle failed, ret: " << static_cast<int>(ret) << std::endl;
        return -1;
    }

    std::cout << "start handle" << std::endl;
    ret = g_lifecycle_provider.Start(handle);
    if (ret != NR_PLUGIN_RESULT_SUCCESS) {
        std::cout << "start handle failed, ret: " << static_cast<int>(ret) << std::endl;
        return -1;
    }

    m_sdk_started = true;
    std::cout << "Start HandTracking Sdk over" << std::endl;
    return 0;
}

int HandTrackingSdk::StopSdk() {
    if (m_sdk_started) {
        NRPluginHandle handle = 256;
        g_lifecycle_provider.Stop(handle);
        g_lifecycle_provider.Release(handle);
        m_plugin_unload();
        m_sdk_started = false;
        std::cout << "Stop HandTracking Sdk over" << std::endl;
        return 0;
    }
    return 0;
}

int HandTrackingSdk::SendStream(std::shared_ptr<StreamData> &data) {
    std::cout << "SendStream frame_id=" << data->frame_id << " is sending!!" << std::endl;
    NRGrayscaleCameraFrameData tmp;
    tmp.camera_count = 1;
    tmp.data = (uint8_t *)data->left_right_frame.data();

    tmp.cameras[0].offset = 0;
    tmp.cameras[0].exposure_start_time_system = data->nano_time;
    tmp.cameras[0].width = g_camera_params.device1.resolution[0];
    tmp.cameras[0].height = g_camera_params.device1.resolution[1];
    tmp.cameras[0].stride = g_camera_params.device1.resolution[0];

    if (2 == GetCameraNum()) {
        tmp.cameras[1].offset = data->left_right_frame.size() / 2;
        tmp.cameras[1].exposure_start_time_system = data->nano_time;
        tmp.cameras[1].width = g_camera_params.device2.resolution[0];
        tmp.cameras[1].height = g_camera_params.device2.resolution[1];
        tmp.cameras[1].stride = g_camera_params.device2.resolution[0];

        tmp.camera_count = 2;
    }

    g_provider.NotifyData(256, NR_CHANNEL_DATA_TYPE_GLASSES_GRAYSCALE_CAMERA, (const void *)&tmp,
                          sizeof(NRGrayscaleCameraFrameData));
    // std::cout << "SendStream frame_id=" << data->frame_id << "sending over!!" << std::endl;

    // 60帧延时释放
    m_wait_free_datas[data->frame_id] = data;
    for (auto iter = m_wait_free_datas.begin(); iter != m_wait_free_datas.end();) {
        if ((iter->first + MAX_FIFO_LENS) < data->frame_id) {
            iter = m_wait_free_datas.erase(iter);
        } else {
            break;
        }
    }

    return 0;
}

int HandTrackingSdk::RecvResult(uint64_t frame_id, std::shared_ptr<StreamResult> &result) {
    auto iter = m_wait_free_datas.find(frame_id);
    if (m_sdk_started && iter != m_wait_free_datas.end()) {
        uint64_t hmd_time_nanos = iter->second->nano_time + 100 * 1000 * 1000;
        uint32_t out_hand_num = 2;
        if (NR_PLUGIN_RESULT_SUCCESS == g_provider.GetHandData(256, hmd_time_nanos, g_out_hand_array, &out_hand_num)) {
            result = std::make_shared<StreamResult>();
            result->frame_id = frame_id;
            result->nano_time = hmd_time_nanos;

            Json::Value json_result;
            uint64_t quest_image_timetamp = iter->second->nano_time;
            for (uint32_t i = 0; i < 2; i++) {
                if (quest_image_timetamp != g_out_hand_array[i].image_timestamp_nanos) {
                    // std::cout << "RecvResult image_timetamp is not match!!" << std::endl;
                    return -1;
                }
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
                    root["hand_joint_data"][j]["hand_joint_type"] =
                        g_out_hand_array[i].hand_joint_data[j].hand_joint_type;

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

                if (0 == i) {
                    json_result["left_hand"] = root;
                } else if (1 == i) {
                    json_result["right_hand"] = root;
                }
            }

            //按特定格式输出内容到控制台，方便与interface中的结果进行比较
            std::cout << "output_data_record begin" << std::endl;
            for (int j = 0; j < 26; j++) {
                NRVector3f position = g_out_hand_array[0].hand_joint_data[j].hand_joint_pose.position;
                NRQuatf rotation = g_out_hand_array[0].hand_joint_data[j].hand_joint_pose.rotation;
                std::cout << "output_data_record left_hand index_" << j << " position: " << position.x << ", "
                          << position.y << ", " << position.z << " rotation: " << rotation.qw << ", " << rotation.qx
                          << ", " << rotation.qy << ", " << rotation.qz << std::endl;
            }

            for (int j = 0; j < 26; j++) {
                NRVector3f position = g_out_hand_array[1].hand_joint_data[j].hand_joint_pose.position;
                NRQuatf rotation = g_out_hand_array[1].hand_joint_data[j].hand_joint_pose.rotation;
                std::cout << "output_data_record right_hand index_" << j << " position: " << position.x << ", "
                          << position.y << ", " << position.z << " rotation: " << rotation.qw << ", " << rotation.qx
                          << ", " << rotation.qy << ", " << rotation.qz << std::endl;
            }

            std::cout << "output_data_record end" << std::endl;

            Json::FastWriter fwriter;
            result->hand_tracking_data = fwriter.write(json_result);
            std::cout << "RecvResult frame_id=" << frame_id << " finish!!" << std::endl;
            // std::cout << "RecvResult nano_time=" << result->nano_time << std::endl;
            // std::cout << "RecvResult hand_tracking_data" << result->hand_tracking_data << std::endl;
        } else {
            // std::cout << "RecvResult frame_id=" << frame_id << " is not exist!!" << std::endl;
            return -1;
        }
    } else {
        // std::cout << "RecvResult frame_id=" << frame_id << " is not exist!!" << std::endl;
        return -1;
    }

    return 0;
}