#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Eigen/Core>
#include <Eigen/Geometry>
#include "doctest/doctest.h"
#include "aisdk/base/camera_model.h"
#include "aisdk/algorithm/func/perspective_crop.h"

TEST_CASE("Test GetVirtualCameraFromBox") {
    // 定义输入，输出
    Eigen::Vector4f bbox_cs{165.3531, 287.06448, 117.96039, 117.96039};
    Eigen::Vector4f target_fc{261.287262186941, 261.287262186941, 63.5, 63.5};
    Eigen::Matrix3f target_matrix;
    target_matrix << 0.95517414,  -0.01956546, -0.29539727,
                       -0.01956546,  0.99146013,  -0.12893411,
                       0.29539727,  0.12893411,  0.94663427;

    // // 创建一个原始相机模型
    aisdk::base::CameraIntrinsics cam_k = {240.81756214390214, 240.79333795688873, 237.98274935930652,
                                            318.76245466163476};

    aisdk::base::OpenCVFisheyeCameraDistortion cam_d{0.023569054999727224, 0.021582533813185853,
                                                        -0.025507559689771916, 0.005611027745777131};
    aisdk::base::BaseCameraModel* camera_orig = new aisdk::base::OpenCVFisheyeCameraModel(
        cam_k, cam_d, Eigen::Isometry3f::Identity(), aisdk::base::CameraType::FISHEYE400, 480, 640);

    // 获取虚拟相机模型
    auto virtual_camera = aisdk::algorithm::GetVirtualCameraFromBox(camera_orig, bbox_cs, {128, 128});
    auto vir_K = virtual_camera->get_camera_intrinsics();
    auto vir_R = virtual_camera->get_cam_to_world_transform();

    // check
    CHECK_LT(abs(target_fc[0] - vir_K.fx_), 1.e-4);
    CHECK_LT(abs(target_fc[1] - vir_K.fy_), 1.e-4);
    CHECK_LT(abs(target_fc[2] - vir_K.cx_), 1.e-4);
    CHECK_LT(abs(target_fc[3] - vir_K.cy_), 1.e-4);
    CHECK_LT(abs(target_matrix(0,0) - vir_R(0,0)), 1.e-4);
    CHECK_LT(abs(target_matrix(0,1) - vir_R(0,1)), 1.e-4);
    CHECK_LT(abs(target_matrix(0,2) - vir_R(0,2)), 1.e-4);
    CHECK_LT(abs(target_matrix(1,0) - vir_R(1,0)), 1.e-4);
    CHECK_LT(abs(target_matrix(1,1) - vir_R(1,1)), 1.e-4);
    CHECK_LT(abs(target_matrix(1,2) - vir_R(1,2)), 1.e-4);
    CHECK_LT(abs(target_matrix(2,0) - vir_R(2,0)), 1.e-4);
    CHECK_LT(abs(target_matrix(2,1) - vir_R(2,1)), 1.e-4);
    CHECK_LT(abs(target_matrix(2,2) - vir_R(2,2)), 1.e-4);
    delete camera_orig;
}

