#include "camera_model.h"

#include <camera-models/CameraModelBase.h>

#include <vector>

#include "aisdk/base/camera_model_distortion.h"
#include "aisdk/base/camera_model_projection.h"
namespace aisdk::base {
template <typename P, typename D>
std::vector<Eigen::Vector2f> CameraModel<P, D>::world_to_window(const std::vector<Eigen::Vector3f> &point_3d) {
    auto eye_kpts = world_to_eye(point_3d);
    auto result = eye_to_window(eye_kpts);
    return result;
}
template <typename P, typename D>
std::vector<Eigen::Vector3f> CameraModel<P, D>::world_to_eye(const std::vector<Eigen::Vector3f> &point_3d) {
    std::vector<Eigen::Vector3f> result;
    result.reserve(point_3d.size());
    for (const auto &point_world : point_3d) {
        Eigen::Vector3f point_eye = camera_to_world_xf_.inverse() * point_world;
        result.emplace_back(point_eye);
    }
    return result;
}

template <typename P, typename D>
std::vector<Eigen::Vector3f> CameraModel<P, D>::eye_to_world(const std::vector<Eigen::Vector3f> &point_3d) {
    std::vector<Eigen::Vector3f> result;
    result.reserve(point_3d.size());
    for (const auto &point_eye : point_3d) {
        Eigen::Vector3f poitn_world = camera_to_world_xf_ * point_eye;
        result.emplace_back(poitn_world);
    }
    return result;
}

template <typename P, typename D>
std::vector<Eigen::Vector2f> CameraModel<P, D>::eye_to_window(const std::vector<Eigen::Vector3f> &point_3d) {
    std::vector<Eigen::Vector2f> result;
    auto projected_points = this->projection_model_.project(point_3d);
    result = this->distortion_model_.distort(projected_points);

    for (size_t i = 0; i < point_3d.size(); i++) {
        result[i][0] = result[i][0] * camera_intrinsics_.fx_ + camera_intrinsics_.cx_;
        result[i][1] = result[i][1] * camera_intrinsics_.fy_ + camera_intrinsics_.cy_;
    }

    return result;
}
template <typename P, typename D>
std::vector<Eigen::Vector2f> CameraModel<P, D>::undistort(const std::vector<Eigen::Vector2f> &point_2d) {
    Eigen::Vector2f fc{camera_intrinsics_.fx_, camera_intrinsics_.fy_};
    Eigen::Vector2f cc{camera_intrinsics_.cx_, camera_intrinsics_.cy_};
    std::vector<Eigen::Vector2f> norm_point_2d(point_2d.size());
    for (size_t i = 0; i < point_2d.size(); i++) {
        const auto &point = point_2d[i];
        norm_point_2d[i] = (point.array() - cc.array()) / fc.array();
    }
    std::vector<Eigen::Vector2f> result = distortion_model_.undistort(norm_point_2d);
    for (auto &point : result) {
        point = point.array() * fc.array() + cc.array();
    }
    return result;
}

template <typename P, typename D>
std::vector<Eigen::Vector3f> CameraModel<P, D>::window_to_eye(const std::vector<Eigen::Vector2f> &point_2d) {
    std::vector<Eigen::Vector2f> normalized_points(point_2d.size());

    auto undistort_points = this->undistort(point_2d);

    for (size_t i = 0; i < point_2d.size(); i++) {
        normalized_points[i](0) = (undistort_points[i](0) - camera_intrinsics_.cx_) / camera_intrinsics_.fx_;
        normalized_points[i](1) = (undistort_points[i](1) - camera_intrinsics_.cy_) / camera_intrinsics_.fy_;
    }

    auto relative_3d = this->projection_model_.unproject(normalized_points);
    return relative_3d;
}
template <typename P, typename D>
std::vector<Eigen::Vector3f> CameraModel<P, D>::window_to_eye(const std::vector<Eigen::Vector3f> &point_3d) {
    std::vector<Eigen::Vector2f> uv_points(point_3d.size());
    std::vector<Eigen::Vector3f> absolute_3d(point_3d.size());

    for (size_t i = 0; i < point_3d.size(); i++) {
        uv_points[i] = point_3d[i].head<2>();
    }
    auto relative_3d = window_to_eye(uv_points);
    for (size_t i = 0; i < point_3d.size(); i++) {
        absolute_3d[i] = relative_3d[i] * (point_3d[i](2) / relative_3d[i](2));
    }
    return absolute_3d;
}

template class CameraModel<PerspectiveProjection, OpenCVPinholeCameraDistortion>;
template class CameraModel<PerspectiveProjection, OpenCVFisheyeCameraDistortion>;
template class CameraModel<PerspectiveProjection, Fisheye624CameraDistortion>;
template class CameraModel<PerspectiveProjection, NoDistortion>;
}  // namespace aisdk::base
