#pragma once
#include "camera_model_distortion.h"
#include "camera_model_projection.h"
namespace aisdk::base {
struct CameraIntrinsics {
    float fx_ = 0;
    float fy_ = 0;
    float cx_ = 0;
    float cy_ = 0;
};

enum class CameraType {
    UNKNOWN = 0,
    PINHOLE = 1,
    FISHEYE400 = 2,
    FISHEYE624 = 3,
};

class BaseCameraModel {
   public:
    BaseCameraModel(const CameraIntrinsics& camera_intrinsics, const Eigen::Isometry3f& camera_to_world_xf,
                    CameraType camera_type, uint32_t video_width, uint32_t video_height)
        : camera_intrinsics_(camera_intrinsics),
          camera_to_world_xf_(camera_to_world_xf),
          camera_type_(camera_type),
          video_width_(video_width),
          video_height_(video_height){};
    virtual std::vector<Eigen::Vector2f> undistort(const std::vector<Eigen::Vector2f>& point_2d) = 0;
    virtual std::vector<Eigen::Vector3f> world_to_eye(const std::vector<Eigen::Vector3f>& point_3d) = 0;
    virtual std::vector<Eigen::Vector3f> eye_to_world(const std::vector<Eigen::Vector3f>& point_3d) = 0;
    virtual std::vector<Eigen::Vector2f> eye_to_window(const std::vector<Eigen::Vector3f>& point_3d) = 0;
    virtual std::vector<Eigen::Vector3f> window_to_eye(const std::vector<Eigen::Vector2f>& point_2d) = 0;
    virtual std::vector<Eigen::Vector3f> window_to_eye(const std::vector<Eigen::Vector3f>& point_3d) = 0;
    virtual Eigen::Isometry3f get_cam_to_world_transform() const { return camera_to_world_xf_; }
    virtual CameraIntrinsics get_camera_intrinsics() const { return camera_intrinsics_; }
    virtual void set_cam_to_world_transform(const Eigen::Isometry3f& xf) { camera_to_world_xf_ = xf; }

   protected:
    CameraIntrinsics camera_intrinsics_;
    Eigen::Isometry3f camera_to_world_xf_ = Eigen::Isometry3f::Identity();

   public:
    CameraType camera_type_;
    uint32_t video_width_;
    uint32_t video_height_;
};

template <typename ProjectType, typename DistortType>
class CameraModel : public BaseCameraModel {
   public:
    CameraModel(const CameraIntrinsics& camera_intrinsics, const DistortType& distortion,
                const Eigen::Isometry3f& camera_to_world_xf, CameraType camera_type, uint32_t video_width,
                uint32_t video_height)
        : BaseCameraModel(camera_intrinsics, camera_to_world_xf, camera_type, video_width, video_height),
          distortion_model_(distortion){};
    std::vector<Eigen::Vector2f> undistort(const std::vector<Eigen::Vector2f>& point_2d) override;
    std::vector<Eigen::Vector3f> world_to_eye(const std::vector<Eigen::Vector3f>& point_3d) override;
    std::vector<Eigen::Vector3f> eye_to_world(const std::vector<Eigen::Vector3f>& point_3d) override;
    std::vector<Eigen::Vector2f> eye_to_window(const std::vector<Eigen::Vector3f>& point_3d) override;
    std::vector<Eigen::Vector3f> window_to_eye(const std::vector<Eigen::Vector2f>& point_2d) override;
    std::vector<Eigen::Vector3f> window_to_eye(const std::vector<Eigen::Vector3f>& point_3d) override;

   private:
    ProjectType projection_model_;
    DistortType distortion_model_;
};

using OpenCVPinholeCameraModel = CameraModel<PerspectiveProjection, OpenCVPinholeCameraDistortion>;
using OpenCVFisheyeCameraModel = CameraModel<PerspectiveProjection, OpenCVFisheyeCameraDistortion>;
using Fisheye624CameraModel = CameraModel<PerspectiveProjection, Fisheye624CameraDistortion>;
using PerspectiveCameraModel = CameraModel<PerspectiveProjection, NoDistortion>;

}  // namespace aisdk::base
