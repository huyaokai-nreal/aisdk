#include <glog/logging.h>

#include <opencv2/core/types.hpp>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <opencv2/opencv.hpp>
#include <string>

#include "aisdk/algorithm/common/bbox.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/perspective_crop.h"
#include "aisdk/algorithm/func/warpaffine.h"
#include "aisdk/base/camera_model.h"
#include "aisdk/xengine/cv/xr_cv.h"
using namespace aisdk::base;
#if defined(__ANDROID__)
std::string test_data_root = "/data/local/tmp/test/";
#else
std::string test_data_root = TEST_DATA_ROOT;
#endif
TEST_CASE("testing the get_roi_image func") {
    cv::Mat raw_image = cv::imread(test_data_root + "flora_test_hand.png", cv::IMREAD_GRAYSCALE);
    float ori_fx = 240.81756214390214;
    float ori_fy = 240.79333795688873;
    float ori_cx = 237.98274935930652;
    float ori_cy = 318.76245466163476;

    float d_fisheye_k1 = 0.023569054999727224;
    float d_fisheye_k2 = 0.021582533813185853;
    float d_fisheye_k3 = -0.025507559689771916;
    float d_fisheye_k4 = 0.005611027745777131;

    float cam_to_world_qw = 1.;
    float cam_to_world_qx = 0.;
    float cam_to_world_qy = 0.;
    float cam_to_world_qz = 0.;

    // CameraIntrinsics ori_intrinsics;
    // CameraDistortion ori_distortion;
    CameraIntrinsics cam_k{240.47993898902308, 240.45010798807022, 238.24292414176563, 318.920557320675};
    OpenCVPinholeCameraDistortion cam_d{0.012542161517124128, 0.04662863296034774, -0.04361866666639336, 0,
                                        0.009913181928564089};
    // Eigen::Isometry3f cam_to_world = Eigen::Isometry3f::Identity();
    // OpenCVPinholeCameraModel ori_camera(ori_intrinsics, ori_distortion, cam_to_world.inverse());
    OpenCVPinholeCameraModel ori_camera(cam_k, cam_d, Eigen::Isometry3f::Identity(), aisdk::base::CameraType::PINHOLE,
                                        480, 640);
    // center and scale calculated from bbox
    // Eigen::Vector2f center = {361.74403381, 327.10635376};
    // Eigen::Vector2f bbox_scale = {114.01742554, 104.48443604};
    Eigen::Vector2i input_size = {128, 128};

    // float scale = input_size(0) / bbox_scale(0);

    aisdk::Vec4f_t bbox_cs = {361.74403381, 327.10635376, 114.01742554, 104.48443604};

    // OpenCVPinholeCameraModel virtual_camera =
    //     gen_crop_parameters_from_points(ori_camera, center, input_size, false, 0, scale);

    std::shared_ptr<PerspectiveCameraModel> virtual_camera =
        aisdk::algorithm::GetVirtualCameraFromBox(&ori_camera, bbox_cs, {128, 128});
    const int COUNT = 3000;
    auto start1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < COUNT; i++) {
        cv::Mat cropped_image0 = aisdk::xengine::perspective_crop_image_raw(
            &ori_camera, virtual_camera.get(), input_size(0), input_size(1), raw_image, cv::INTER_LINEAR, true);
    }

    auto start2 = std::chrono::high_resolution_clock::now();
    cv::Mat cropped_image0 = aisdk::xengine::perspective_crop_image_raw(
        &ori_camera, virtual_camera.get(), input_size(0), input_size(1), raw_image, cv::INTER_LINEAR, true);

    auto start3 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < COUNT; i++) {
        cv::Mat cropped_image1 = aisdk::xengine::perspective_crop_image(
            &ori_camera, virtual_camera.get(), input_size(0), input_size(1), raw_image, cv::INTER_LINEAR, true);
    }
    cv::Mat cropped_image1 = aisdk::xengine::perspective_crop_image(&ori_camera, virtual_camera.get(), input_size(0),
                                                                    input_size(1), raw_image, cv::INTER_LINEAR, true);
    auto start4 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::micro> elasped0 = start2 - start1;
    std::chrono::duration<double, std::micro> elasped1 = start4 - start3;
    printf("cv::remap time = %fμs\n", elasped0.count() / COUNT);
    printf("remap_neon_u8_f32_f32_c1_linear_const0 time = %fμs\n", elasped1.count() / COUNT);

    // double diff = cv::norm(cropped_image0, cropped_image1, cv::NORM_L2);
    // CHECK_EQ(diff, 0.0);
    uchar *test0 = (uchar *)cropped_image0.data;
    uchar *test1 = (uchar *)cropped_image1.data;
    double diff = 0.f;
    for (int i = 0; i < 128; i++) {
        for (int j = 0; j < 128; j++) {
            diff += std::abs(test0[i * 128 + j] - test1[i * 128 + j]);
        }
        // printf("\n");
    }
    printf("%f\n", diff);
    CHECK_LE(diff / 128 / 128, 1e-3);

    // for (int i = 0; i < 128; i++) {
    //     for (int j = 0; j < 128; j++) {
    //         if (test0[i * 128 + j] != test1[i * 128 + j]) {
    //             printf("i = %d\tj = %d\t%d\t%d\n", i, j, test0[i * 128 + j], test1[i * 128 + j]);
    //         }
    //     }
    //     // printf("\n");
    // }
    // printf("\n");
}
