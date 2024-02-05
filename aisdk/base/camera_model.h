#pragma once
#include "camera_model_distortion.h"
#include "camera_model_projection.h"
namespace aisdk {
struct CameraIntrinsics {
    float fx_ = 0;
    float fy_ = 0;
    float cx_ = 0;
    float cy_ = 0;
};
template <typename ProjectType, typename DistortType>
class CameraModel {
   public:
    CameraModel(const CameraIntrinsics& camera_intrinsics, const DistortType& distortion,
                const Eigen::Isometry3f& camera_to_world_xf)
        : camera_intrinsics_(camera_intrinsics),
          distortion_model_(distortion),
          camera_to_world_xf_(camera_to_world_xf){};
    std::vector<Eigen::Vector2f> undistort(const std::vector<Eigen::Vector2f>& point_2d);
    std::vector<Eigen::Vector3f> world_to_eye(const std::vector<Eigen::Vector3f>& point_3d);
    std::vector<Eigen::Vector3f> eye_to_world(const std::vector<Eigen::Vector3f>& point_3d);
    std::vector<Eigen::Vector2f> eye_to_window(const std::vector<Eigen::Vector3f>& point_3d);
    std::vector<Eigen::Vector3f> window_to_eye(const std::vector<Eigen::Vector2f>& point_2d);
    std::vector<Eigen::Vector3f> window_to_eye(const std::vector<Eigen::Vector3f>& point_3d);
    Eigen::Isometry3f get_cam_to_world_transform() const { return camera_to_world_xf_; }
    CameraIntrinsics get_camera_intrinsics() const { return camera_intrinsics_; }
    void set_cam_to_world_transform(const Eigen::Isometry3f& xf){
        camera_to_world_xf_ = xf;
    }
   private:
    CameraIntrinsics camera_intrinsics_;
    ProjectType projection_model_;
    DistortType distortion_model_;
    Eigen::Isometry3f camera_to_world_xf_ = Eigen::Isometry3f::Identity();
};

using OpenCVPinholeCameraModel = CameraModel<PerspectiveProjection, OpenCVPinholeCameraDistortion>;
using OpenCVFisheyeCameraModel = CameraModel<PerspectiveProjection, OpenCVFisheyeCameraDistortion>;
using Fisheye624CameraModel = CameraModel<PerspectiveProjection, Fisheye624CameraDistortion>;
using PerspectiveCameraModel = CameraModel<PerspectiveProjection, NoDistortion>;

}  // namespace aisdk