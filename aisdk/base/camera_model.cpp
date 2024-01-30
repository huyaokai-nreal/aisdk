#include "camera_model.h"

#include <opencv2/calib3d.hpp>
namespace aisdk {
CameraModel::~CameraModel() {}
std::vector<Eigen::Vector3f> CameraModel::world_to_eye(const std::vector<Eigen::Vector3f>& point_3d) {
    std::vector<Eigen::Vector3f> result;
    result.reserve(point_3d.size());
    for (const auto& point_world : point_3d) {
        Eigen::Vector3f point_eye = camera_to_world_xf_.inverse() * point_world;
        result.emplace_back(point_eye);
    }
    return result;
}

std::vector<Eigen::Vector3f> CameraModel::eye_to_world(const std::vector<Eigen::Vector3f>& point_3d) {
    std::vector<Eigen::Vector3f> result;
    result.reserve(point_3d.size());
    for (const auto& point_eye : point_3d) {
        Eigen::Vector3f poitn_world = camera_to_world_xf_ * point_eye;
        result.emplace_back(poitn_world);
    }
    return result;
}

std::vector<Eigen::Vector2f> OpenCVPinholeCameraModel::eye_to_window(const std::vector<Eigen::Vector3f>& point_3d) {
    std::vector<Eigen::Vector2f> result;
    auto projected_points = this->projection_model_.project(point_3d);
    result = this->distortion_model_.evaluate(projected_points);

    for (size_t i = 0; i < point_3d.size(); i++) {
        result[i][0] = result[i][0] * camera_intrinsics_.fx_ + camera_intrinsics_.cx_;
        result[i][1] = result[i][1] * camera_intrinsics_.fy_ + camera_intrinsics_.cy_;
    }

    return result;
}
cv::Mat OpenCVPinholeCameraModel::get_distortion_matrix_cv() {
    cv::Mat distCoeffs = cv::Mat::zeros(5, 1, CV_32F);
    auto D = distortion_model_.getDistortionParams();
    distCoeffs.at<float>(0) = D[0];
    distCoeffs.at<float>(1) = D[1];
    distCoeffs.at<float>(2) = D[2];
    distCoeffs.at<float>(3) = D[3];
    distCoeffs.at<float>(4) = D[4];

    return distCoeffs;
}

std::vector<Eigen::Vector2f> OpenCVPinholeCameraModel::undistort(const std::vector<Eigen::Vector2f>& point_2d) {
    auto K = get_intrinsic_matrix_cv();
    auto D = get_distortion_matrix_cv();

    std::vector<cv::Vec2f> point_2d_in(point_2d.size());
    std::vector<cv::Vec2f> point_2d_out;
    std::vector<Eigen::Vector2f> result(point_2d.size());

    for (size_t i = 0; i < point_2d.size(); i++) {
        point_2d_in[i][0] = point_2d[i](0);
        point_2d_in[i][1] = point_2d[i](1);
    }

    cv::undistortPoints(point_2d_in, point_2d_out, K, D, cv::noArray(), K);

    for (size_t i = 0; i < point_2d.size(); i++) {
        result[i](0) = point_2d_out[i][0];
        result[i](1) = point_2d_out[i][1];
    }
    return result;
}

std::vector<Eigen::Vector3f> OpenCVPinholeCameraModel::window_to_eye(const std::vector<Eigen::Vector2f>& point_2d) {
    std::vector<Eigen::Vector2f> normalized_points(point_2d.size());

    auto undistort_points = this->undistort(point_2d);

    for (size_t i = 0; i < point_2d.size(); i++) {
        normalized_points[i](0) = (undistort_points[i](0) - camera_intrinsics_.cx_) / camera_intrinsics_.fx_;
        normalized_points[i](1) = (undistort_points[i](1) - camera_intrinsics_.cy_) / camera_intrinsics_.fy_;
    }

    auto relative_3d = this->projection_model_.unproject(normalized_points);
    return relative_3d;
}

std::vector<Eigen::Vector3f> OpenCVPinholeCameraModel::window_to_eye(const std::vector<Eigen::Vector3f>& point_3d) {
    std::vector<Eigen::Vector2f> uv_points(point_3d.size());
    std::vector<Eigen::Vector2f> normalized_points(point_3d.size());
    std::vector<Eigen::Vector3f> absolute_3d(point_3d.size());

    for (size_t i = 0; i < point_3d.size(); i++) {
        uv_points[i] = point_3d[i].head<2>();
    }
    auto undistort_points = this->undistort(uv_points);
    for (size_t i = 0; i < point_3d.size(); i++) {
        normalized_points[i](0) = (undistort_points[i](0) - camera_intrinsics_.cx_) / camera_intrinsics_.fx_;
        normalized_points[i](1) = (undistort_points[i](1) - camera_intrinsics_.cy_) / camera_intrinsics_.fy_;
    }
    auto relative_3d = this->projection_model_.unproject(normalized_points);
    for (size_t i = 0; i < point_3d.size(); i++) {
        absolute_3d[i] = relative_3d[i] * (point_3d[i](2) / relative_3d[i](2));
    }
    return absolute_3d;
}

cv::Mat OpenCVFisheyeCameraModel::get_distortion_matrix_cv() {
    cv::Mat distCoeffs = cv::Mat::zeros(4, 1, CV_32F);

    auto D = distortion_model_.getDistortionParams();

    distCoeffs.at<float>(0) = D[0];
    distCoeffs.at<float>(1) = D[1];
    distCoeffs.at<float>(2) = D[2];
    distCoeffs.at<float>(3) = D[3];

    return distCoeffs;
}

std::vector<Eigen::Vector2f> OpenCVFisheyeCameraModel::eye_to_window(const std::vector<Eigen::Vector3f>& point_3d) {
    std::vector<Eigen::Vector2f> result;
    auto projected_points = this->projection_model_.project(point_3d);
    result = this->distortion_model_.evaluate(projected_points);

    for (size_t i = 0; i < point_3d.size(); i++) {
        result[i][0] = result[i][0] * camera_intrinsics_.fx_ + camera_intrinsics_.cx_;
        result[i][1] = result[i][1] * camera_intrinsics_.fy_ + camera_intrinsics_.cy_;
    }

    return result;
}

std::vector<Eigen::Vector2f> OpenCVFisheyeCameraModel::undistort(const std::vector<Eigen::Vector2f>& point_2d) {
    auto K = get_intrinsic_matrix_cv();
    auto D = get_distortion_matrix_cv();

    std::vector<cv::Vec2f> point_2d_in(point_2d.size());
    std::vector<cv::Vec2f> point_2d_out;
    std::vector<Eigen::Vector2f> result(point_2d.size());

    for (size_t i = 0; i < point_2d.size(); i++) {
        point_2d_in[i][0] = point_2d[i](0);
        point_2d_in[i][1] = point_2d[i](1);
    }

    cv::fisheye::undistortPoints(point_2d_in, point_2d_out, K, D, cv::noArray(), K);

    for (size_t i = 0; i < point_2d.size(); i++) {
        result[i](0) = point_2d_out[i][0];
        result[i](1) = point_2d_out[i][1];
    }
    return result;
}

std::vector<Eigen::Vector3f> OpenCVFisheyeCameraModel::window_to_eye(const std::vector<Eigen::Vector2f>& point_2d) {
    std::vector<Eigen::Vector2f> normalized_points(point_2d.size());

    auto undistort_points = this->undistort(point_2d);

    for (size_t i = 0; i < point_2d.size(); i++) {
        normalized_points[i](0) = (undistort_points[i](0) - camera_intrinsics_.cx_) / camera_intrinsics_.fx_;
        normalized_points[i](1) = (undistort_points[i](1) - camera_intrinsics_.cy_) / camera_intrinsics_.fy_;
    }

    auto relative_3d = this->projection_model_.unproject(normalized_points);
    return relative_3d;
}

std::vector<Eigen::Vector3f> OpenCVFisheyeCameraModel::window_to_eye(const std::vector<Eigen::Vector3f>& point_3d) {
    std::vector<Eigen::Vector2f> uv_points(point_3d.size());
    std::vector<Eigen::Vector2f> normalized_points(point_3d.size());
    std::vector<Eigen::Vector3f> absolute_3d(point_3d.size());

    for (size_t i = 0; i < point_3d.size(); i++) {
        uv_points[i] = point_3d[i].head<2>();
    }

    auto undistort_points = this->undistort(uv_points);

    for (size_t i = 0; i < point_3d.size(); i++) {
        normalized_points[i](0) = (undistort_points[i](0) - camera_intrinsics_.cx_) / camera_intrinsics_.fx_;
        normalized_points[i](1) = (undistort_points[i](1) - camera_intrinsics_.cy_) / camera_intrinsics_.fy_;
    }

    auto relative_3d = this->projection_model_.unproject(normalized_points);

    for (size_t i = 0; i < point_3d.size(); i++) {
        absolute_3d[i] = relative_3d[i] * (point_3d[i](2) / relative_3d[i](2));
    }

    return absolute_3d;
}

}  // namespace aisdk