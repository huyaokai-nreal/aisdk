#include "camera_model_distortion.h"
#include <camera-models/CameraModelBase.h>

#include <vector>
namespace aisdk::base {
std::vector<Eigen::Vector2f> OpenCVPinholeCameraDistortion::distort(const std::vector<Eigen::Vector2f>& point_2d) {
    std::vector<Eigen::Vector2f> result;

    for (const auto& point : point_2d) {
        float r2 = point.squaredNorm();
        float r4 = r2 * r2;
        float r6 = r4 * r2;

        float x_1 = point(0);
        float y_1 = point(1);

        float x_2 = x_1 * (1 + k1_ * r2 + k2_ * r4 + k3_ * r6) + 2 * p1_ * x_1 * y_1 + p2_ * (r2 + 2 * x_1 * x_1);
        float y_2 = y_1 * (1 + k1_ * r2 + k2_ * r4 + k3_ * r6) + 2 * p2_ * x_1 * y_1 + p1_ * (r2 + 2 * y_1 * y_1);

        result.emplace_back(x_2, y_2);
    }

    return result;
}

std::vector<Eigen::Vector2f> OpenCVPinholeCameraDistortion::undistort(const std::vector<Eigen::Vector2f>& point_2d) {
    std::vector<Eigen::Vector2f> result;
    Eigen::Matrix<float, 12, 1> kc = Eigen::Matrix<float, 12, 1>::Zero();
    kc << k1_, k2_, p1_, p2_, k3_;
    Eigen::Vector2f fc{1, 1};
    Eigen::Vector2f cc{0, 0};
    for (const auto& point : point_2d) {
        Eigen::Vector2f undistort_pt;
        camera_models::CameraModelRadial<float>::StaticUndistort(point, fc, cc, kc, undistort_pt);
        result.push_back(undistort_pt);
    }
    return result;
}

std::vector<Eigen::Vector2f> OpenCVFisheyeCameraDistortion::distort(const std::vector<Eigen::Vector2f>& point_2d) {
    std::vector<Eigen::Vector2f> result;

    for (const auto& point : point_2d) {
        float r2 = point.squaredNorm();
        float theta = std::atan(std::sqrt(r2));
        float theta2 = theta * theta;
        float theta4 = theta2 * theta2;
        float theta6 = theta2 * theta4;
        float theta8 = theta4 * theta4;
        float thetad = theta * (1 + k1_ * theta2 + k2_ * theta4 + k3_ * theta6 + k4_ * theta8);
        Eigen::Vector2f uv = point * thetad / std::sqrt(r2);
        result.push_back(uv);
    }

    return result;
}

std::vector<Eigen::Vector2f> OpenCVFisheyeCameraDistortion::undistort(const std::vector<Eigen::Vector2f>& point_2d) {
    std::vector<Eigen::Vector2f> result;
    Eigen::Matrix<float, 12, 1> kc = Eigen::Matrix<float, 12, 1>::Zero();
    kc << k1_, k2_, k3_, k4_;
    Eigen::Vector2f fc{1, 1};
    Eigen::Vector2f cc{0, 0};
    for (const auto& point : point_2d) {
        Eigen::Vector2f undistort_pt;
        camera_models::CameraModelFisheye<float>::StaticUndistort(point, fc, cc, kc, undistort_pt);
        result.push_back(undistort_pt);
    }
    return result;
}

std::vector<Eigen::Vector2f> Fisheye624CameraDistortion::distort(const std::vector<Eigen::Vector2f>& point_2d) {
    std::vector<Eigen::Vector2f> result;
    Eigen::Matrix<float, 12, 1> kc;
    kc << k1_, k2_, k3_, k4_, k5_, k6_, p1_, p2_, s1_, s2_, s3_, s4_;
    Eigen::Vector2f fc{1, 1};
    Eigen::Vector2f cc{0, 0};
    for (const auto& point : point_2d) {
        Eigen::Vector2f undistort_pt;
        camera_models::CameraModelFisheye624<float>::StaticDistort(point, fc, cc, kc, undistort_pt);
        result.push_back(undistort_pt);
    }
    return result;
}
std::vector<Eigen::Vector2f> Fisheye624CameraDistortion::undistort(const std::vector<Eigen::Vector2f>& point_2d) {
    std::vector<Eigen::Vector2f> result;
    Eigen::Matrix<float, 12, 1> kc;
    kc << k1_, k2_, k3_, k4_, k5_, k6_, p1_, p2_, s1_, s2_, s3_, s4_;
    Eigen::Vector2f fc{1, 1};
    Eigen::Vector2f cc{0, 0};
    for (const auto& point : point_2d) {
        Eigen::Vector2f undistort_pt;
        camera_models::CameraModelFisheye624<float>::StaticUndistort(point, fc, cc, kc, undistort_pt);
        result.push_back(undistort_pt);
    }
    return result;
}

}  // namespace aisdk::base