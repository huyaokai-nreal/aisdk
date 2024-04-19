#include <memory>
#include <vector>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "aisdk/base/camera_model.h"
using namespace aisdk::base;
TEST_CASE("testing the opencv pinhole camera") {
    CameraIntrinsics cam_k{240.47993898902308, 240.45010798807022, 238.24292414176563, 318.920557320675};
    OpenCVPinholeCameraDistortion cam_d{0.012542161517124128, 0.04662863296034774, -0.04361866666639336, 0,
                                        0.009913181928564089};
    OpenCVPinholeCameraModel camera_model{cam_k, cam_d, Eigen::Isometry3f::Identity(), aisdk::base::CameraType::PINHOLE, 480, 640};
    Eigen::Vector3f world_pt{10, 10, 100};
    std::vector<Eigen::Vector3f> world_pts{world_pt};
    std::vector<Eigen::Vector3f> camera_pts = camera_model.world_to_eye(world_pts);
    CHECK_EQ(camera_pts.size(), 1);
    CHECK_EQ(camera_pts[0].x(), 10);
    CHECK_EQ(camera_pts[0].y(), 10);
    CHECK_EQ(camera_pts[0].z(), 100);
    std::vector<Eigen::Vector2f> image_pts = camera_model.eye_to_window(world_pts);
    CHECK_EQ(image_pts.size(), 1);
    CHECK_LT(abs(image_pts[0].x() - 262.088), 1e-3);
    CHECK_LT(abs(image_pts[0].y() - 342.553), 1e-3);
    std::vector<Eigen::Vector2f> undistort_pts = camera_model.undistort(image_pts);
    CHECK_EQ(undistort_pts.size(), 1);
    CHECK_LT(abs(undistort_pts[0].x() - 262.291), 1e-3);
    CHECK_LT(abs(undistort_pts[0].y() - 342.966), 1e-3);
    std::vector<Eigen::Vector3f> norm_pts = camera_model.window_to_eye(image_pts);
    CHECK_EQ(norm_pts.size(), 1);
    CHECK_LT(abs(norm_pts[0].x() - 0.0990148), 1e-3);
    CHECK_LT(abs(norm_pts[0].y() - 0.0990148), 1e-3);
    CHECK_LT(abs(norm_pts[0].z() - 0.990148), 1e-3);
}

TEST_CASE("testing the opencv fisheye camera") {
    CameraIntrinsics cam_k{240.47993898902308, 240.45010798807022, 238.24292414176563, 318.920557320675};
    OpenCVFisheyeCameraDistortion cam_d{0.012542161517124128, 0.04662863296034774, -0.04361866666639336,
                                        0.009913181928564089};
    OpenCVFisheyeCameraModel camera_model{cam_k, cam_d, Eigen::Isometry3f::Identity(), aisdk::base::CameraType::FISHEYE400, 480, 640};
    Eigen::Vector3f world_pt{100, 100, 100};
    std::vector<Eigen::Vector3f> world_pts{world_pt};
    std::vector<Eigen::Vector3f> camera_pts = camera_model.world_to_eye(world_pts);
    CHECK_EQ(camera_pts.size(), 1);
    CHECK_EQ(camera_pts[0].x(), 100);
    CHECK_EQ(camera_pts[0].y(), 100);
    CHECK_EQ(camera_pts[0].z(), 100);
    std::vector<Eigen::Vector2f> image_pts = camera_model.eye_to_window(world_pts);
    CHECK_EQ(image_pts.size(), 1);
    CHECK_LT(abs(image_pts[0].x() - 404.589), 1e-3);
    CHECK_LT(abs(image_pts[0].y() - 485.246), 1e-3);
    std::vector<Eigen::Vector2f> undistort_pts = camera_model.undistort(image_pts);
    CHECK_EQ(undistort_pts.size(), 1);
    CHECK_LT(abs(undistort_pts[0].x() - 478.723), 1e-3);
    CHECK_LT(abs(undistort_pts[0].y() - 559.371), 1e-3);
    std::vector<Eigen::Vector3f> norm_pts = camera_model.window_to_eye(image_pts);
    CHECK_EQ(norm_pts.size(), 1);
    CHECK_LT(abs(norm_pts[0].x() - 0.577), 1e-3);
    CHECK_LT(abs(norm_pts[0].y() - 0.577), 1e-3);
    CHECK_LT(abs(norm_pts[0].z() - 0.577), 1e-3);
}

TEST_CASE("testing the opencv fisheye624 camera") {
    CameraIntrinsics cam_k{240.47993898902308, 240.45010798807022, 238.24292414176563, 318.920557320675};
    Fisheye624CameraDistortion cam_d{
        0.012542161517124128, 0.04662863296034774, -0.04361866666639336, 0.009913181928564089, 0, 0, 0, 0, 0, 0, 0, 0};
    Fisheye624CameraModel camera_model{cam_k, cam_d, Eigen::Isometry3f::Identity(), aisdk::base::CameraType::FISHEYE624, 480, 640};
    Eigen::Vector3f world_pt{100, 100, 100};
    std::vector<Eigen::Vector3f> world_pts{world_pt};
    std::vector<Eigen::Vector3f> camera_pts = camera_model.world_to_eye(world_pts);
    CHECK_EQ(camera_pts.size(), 1);
    CHECK_EQ(camera_pts[0].x(), 100);
    CHECK_EQ(camera_pts[0].y(), 100);
    CHECK_EQ(camera_pts[0].z(), 100);
    std::vector<Eigen::Vector2f> image_pts = camera_model.eye_to_window(world_pts);
    CHECK_EQ(image_pts.size(), 1);
    CHECK_LT(abs(image_pts[0].x() - 404.589), 1e-3);
    CHECK_LT(abs(image_pts[0].y() - 485.246), 1e-3);
    std::vector<Eigen::Vector2f> undistort_pts = camera_model.undistort(image_pts);
    CHECK_EQ(undistort_pts.size(), 1);
    CHECK_LT(abs(undistort_pts[0].x() - 478.723), 1e-3);
    CHECK_LT(abs(undistort_pts[0].y() - 559.371), 1e-3);
    std::vector<Eigen::Vector3f> norm_pts = camera_model.window_to_eye(image_pts);
    CHECK_EQ(norm_pts.size(), 1);
    CHECK_LT(abs(norm_pts[0].x() - 0.577), 1e-3);
    CHECK_LT(abs(norm_pts[0].y() - 0.577), 1e-3);
    CHECK_LT(abs(norm_pts[0].z() - 0.577), 1e-3);
}
TEST_CASE("testing the opencv fisheye624 camera with base class") {
    CameraIntrinsics cam_k{240.47993898902308, 240.45010798807022, 238.24292414176563, 318.920557320675};
    Fisheye624CameraDistortion cam_d{
        0.012542161517124128, 0.04662863296034774, -0.04361866666639336, 0.009913181928564089, 0, 0, 0, 0, 0, 0, 0, 0};
    std::unique_ptr<aisdk::base::BaseCameraModel> camera_model_ptr = std::make_unique<Fisheye624CameraModel>(cam_k, cam_d, Eigen::Isometry3f::Identity(), aisdk::base::CameraType::FISHEYE624, 480, 640);
    Eigen::Vector3f world_pt{100, 100, 100};
    std::vector<Eigen::Vector3f> world_pts{world_pt};
    std::vector<Eigen::Vector3f> camera_pts = camera_model_ptr->world_to_eye(world_pts);
    CHECK_EQ(camera_pts.size(), 1);
    CHECK_EQ(camera_pts[0].x(), 100);
    CHECK_EQ(camera_pts[0].y(), 100);
    CHECK_EQ(camera_pts[0].z(), 100);
    std::vector<Eigen::Vector2f> image_pts = camera_model_ptr->eye_to_window(world_pts);
    CHECK_EQ(image_pts.size(), 1);
    CHECK_LT(abs(image_pts[0].x() - 404.589), 1e-3);
    CHECK_LT(abs(image_pts[0].y() - 485.246), 1e-3);
    std::vector<Eigen::Vector2f> undistort_pts = camera_model_ptr->undistort(image_pts);
    CHECK_EQ(undistort_pts.size(), 1);
    CHECK_LT(abs(undistort_pts[0].x() - 478.723), 1e-3);
    CHECK_LT(abs(undistort_pts[0].y() - 559.371), 1e-3);
    std::vector<Eigen::Vector3f> norm_pts = camera_model_ptr->window_to_eye(image_pts);
    CHECK_EQ(norm_pts.size(), 1);
    CHECK_LT(abs(norm_pts[0].x() - 0.577), 1e-3);
    CHECK_LT(abs(norm_pts[0].y() - 0.577), 1e-3);
    CHECK_LT(abs(norm_pts[0].z() - 0.577), 1e-3);
}
