#include <vector>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "aisdk/base/camera_model.h"
using namespace aisdk;
TEST_CASE("testing the opencv pinhole camera") {
    CameraIntrinsics cam_k{240.47993898902308, 240.45010798807022, 238.24292414176563, 318.920557320675};
    OpenCVPinholeCameraDistortion cam_d{0.012542161517124128, 0.04662863296034774, -0.04361866666639336, 0,
                                        0.009913181928564089};
    OpenCVPinholeCameraModel camera_model{cam_k, cam_d, Eigen::Isometry3f::Identity()};
    Eigen::Vector3f world_pt{100, 100, 100};
    std::vector<Eigen::Vector3f> world_pts{world_pt};
    std::vector<Eigen::Vector3f> camera_pts = camera_model.world_to_eye(world_pts);
    CHECK_EQ(camera_pts.size(), 1);
    CHECK_EQ(camera_pts[0].x(), 100);
    CHECK_EQ(camera_pts[0].y(), 100);
    CHECK_EQ(camera_pts[0].z(), 100);
    std::vector<Eigen::Vector2f> image_pts = camera_model.eye_to_window(world_pts);
    CHECK_EQ(image_pts.size(), 1);
    CHECK_LT(abs(image_pts[0].x()-527.701), 1e-3);
    CHECK_LT(abs(image_pts[0].y()-587.366), 1e-3);
    std::vector<Eigen::Vector2f> undistort_pts = camera_model.undistort(image_pts);
    CHECK_EQ(undistort_pts.size(), 1);
    CHECK_LT(abs(undistort_pts[0].x()-465.931), 1e-3);
    CHECK_LT(abs(undistort_pts[0].y()-548.259), 1e-3);
    std::vector<Eigen::Vector3f> norm_pts = camera_model.window_to_eye(image_pts);
    CHECK_EQ(norm_pts.size(), 1); 
    CHECK_LT(abs(norm_pts[0].x()-0.5652), 1e-3);
    CHECK_LT(abs(norm_pts[0].y()-0.5694), 1e-3);
    CHECK_LT(abs(norm_pts[0].z()-0.597), 1e-3);
}