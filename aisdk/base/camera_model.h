#pragma once
#include <opencv2/core.hpp>

#include "camera_model_distortion.h"
#include "camera_model_projection.h"
namespace aisdk {
struct CameraIntrinsics {
    float fx_ = 0;
    float fy_ = 0;
    float cx_ = 0;
    float cy_ = 0;
};
class CameraModel {
   public:
    CameraModel(const CameraIntrinsics& camera_intrinsics, const Eigen::Isometry3f& camera_to_world_xf)
        : camera_intrinsics_(camera_intrinsics), camera_to_world_xf_(camera_to_world_xf){};
    virtual ~CameraModel();
    virtual std::vector<Eigen::Vector2f> undistort(const std::vector<Eigen::Vector2f>& point_2d) = 0;
    virtual std::vector<Eigen::Vector3f> world_to_eye(const std::vector<Eigen::Vector3f>& point_3d);
    virtual std::vector<Eigen::Vector3f> eye_to_world(const std::vector<Eigen::Vector3f>& point_3d);
    virtual std::vector<Eigen::Vector2f> eye_to_window(const std::vector<Eigen::Vector3f>& point_3d) = 0;
    virtual std::vector<Eigen::Vector3f> window_to_eye(const std::vector<Eigen::Vector2f>& point_2d) = 0;
    virtual std::vector<Eigen::Vector3f> window_to_eye(const std::vector<Eigen::Vector3f>& point_3d) = 0;
    Eigen::Isometry3f get_cam_to_world_transform() const { return camera_to_world_xf_; }
    CameraIntrinsics get_camera_intrinsics() const { return camera_intrinsics_; }
    cv::Mat get_intrinsic_matrix_cv() const {
        cv::Mat K = cv::Mat::eye(3, 3, CV_32F);
        K.at<float>(0, 0) = camera_intrinsics_.fx_;
        K.at<float>(1, 1) = camera_intrinsics_.fy_;
        K.at<float>(0, 2) = camera_intrinsics_.cx_;
        K.at<float>(1, 2) = camera_intrinsics_.cy_;

        return K;
    }

   protected:
    CameraIntrinsics camera_intrinsics_;
    PerspectiveProjection projection_model_;
    Eigen::Isometry3f camera_to_world_xf_ = Eigen::Isometry3f::Identity();
};
class OpenCVPinholeCameraModel : public CameraModel {
   public:
    OpenCVPinholeCameraModel(const CameraIntrinsics& camera_intrinsics, const OpenCVPinholeCameraDistortion& distortion,
                             const Eigen::Isometry3f& camera_to_world_xf)
        : CameraModel(camera_intrinsics, camera_to_world_xf), distortion_model_(distortion) {}
    std::vector<Eigen::Vector2f> undistort(const std::vector<Eigen::Vector2f>& point_2d) override;
    std::vector<Eigen::Vector2f> eye_to_window(const std::vector<Eigen::Vector3f>& point_3d) override;
    std::vector<Eigen::Vector3f> window_to_eye(const std::vector<Eigen::Vector2f>& point_2d) override;
    std::vector<Eigen::Vector3f> window_to_eye(const std::vector<Eigen::Vector3f>& point_3d) override;

   private:
    cv::Mat get_distortion_matrix_cv();
    PerspectiveProjection projection_model_;
    OpenCVPinholeCameraDistortion distortion_model_;
};
class OpenCVFisheyeCameraModel : public CameraModel {
   public:
    OpenCVFisheyeCameraModel(const CameraIntrinsics& camera_intrinsics, const OpenCVFisheyeCameraDistortion& distortion,
                             const Eigen::Isometry3f& camera_to_world_xf)
        : CameraModel(camera_intrinsics, camera_to_world_xf), distortion_model_(distortion) {}
    std::vector<Eigen::Vector2f> undistort(const std::vector<Eigen::Vector2f>& point_2d) override;
    std::vector<Eigen::Vector2f> eye_to_window(const std::vector<Eigen::Vector3f>& point_3d) override;
    std::vector<Eigen::Vector3f> window_to_eye(const std::vector<Eigen::Vector2f>& point_2d) override;
    std::vector<Eigen::Vector3f> window_to_eye(const std::vector<Eigen::Vector3f>& point_3d) override;

   private:
    cv::Mat get_distortion_matrix_cv();
    PerspectiveProjection projection_model_;
    OpenCVFisheyeCameraDistortion distortion_model_;
};

class Fisheye624CameraModel : public CameraModel {
   public:
    Fisheye624CameraModel(const CameraIntrinsics& camera_intrinsics, const Fisheye624CameraDistortion& distortion,
                             const Eigen::Isometry3f& camera_to_world_xf)
        : CameraModel(camera_intrinsics, camera_to_world_xf), distortion_model_(distortion) {}
    std::vector<Eigen::Vector2f> undistort(const std::vector<Eigen::Vector2f>& point_2d) override;
    std::vector<Eigen::Vector2f> eye_to_window(const std::vector<Eigen::Vector3f>& point_3d) override;
    std::vector<Eigen::Vector3f> window_to_eye(const std::vector<Eigen::Vector2f>& point_2d) override;
    std::vector<Eigen::Vector3f> window_to_eye(const std::vector<Eigen::Vector3f>& point_3d) override;

   private:
    PerspectiveProjection projection_model_;
    Fisheye624CameraDistortion distortion_model_;
};

}  // namespace aisdk