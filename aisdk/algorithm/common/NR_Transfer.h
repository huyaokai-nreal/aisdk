#ifndef __NR_TRANSFER__
#define __NR_TRANSFER__

#include <opencv2/opencv.hpp>

#include "Eigen/Dense"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/log.h"
#include "interface/handtracking_sdk/public/nr_plugin_types.h"

#define REPROJ_POINT_NUM 21
namespace aisdk::algorithm {

void TransferCVToGL(const std::vector<cv::Vec3f>& _point3d_src, std::vector<cv::Vec3f>& _point3d_dst);

void TransferLeftCamToHead(const std::vector<cv::Vec3f>& _point3d_src, std::vector<cv::Vec3f>& _point3d_dst);

void TransferHeadToWorld(NRTransform& extrinsic_trans_head_world_, const std::vector<cv::Vec3f>& _point3d_src,
                         std::vector<cv::Vec3f>& _point3d_dst);

std::vector<cv::Vec3f> recal_lcam_kpt3d_cv_with_new_headpose(NRTransform extrinsic_world,
                                                             const std::vector<cv::Vec3f>& points3d_src);
std::vector<cv::Vec3f> cal_lcam_kpt3d_cv_to_world(NRTransform extrinsic_world,
                                                  const std::vector<cv::Vec3f>& points3d_src);

void reproj_bbox_with_new_headpose(cv::Mat intrinsics_lcam, cv::Mat intrinsics_rcam, NRTransform extrinsics_world,
                                   const std::vector<cv::Vec3f>& points_3d, cv::Rect& proj_bbox_lcam,
                                   cv::Rect& proj_bbox_rcam);
void reproj_bbox_with_new_headpose_light(std::shared_ptr<aisdk::base::OpenCVPinholeCameraModel> lcam_model,
                                         std::shared_ptr<aisdk::base::OpenCVPinholeCameraModel> rcam_model,
                                         NRTransform extrinsics_world, const std::vector<cv::Vec3f>& points_3d,
                                         cv::Rect& proj_bbox_lcam, cv::Rect& proj_bbox_rcam);
void reproj_bbox_with_new_headpose_flora(std::shared_ptr<aisdk::base::OpenCVFisheyeCameraModel> lcam_model,
                                         std::shared_ptr<aisdk::base::OpenCVFisheyeCameraModel> rcam_model,
                                         NRTransform extrinsics_world, const std::vector<cv::Vec3f>& points_3d,
                                         cv::Rect& proj_bbox_lcam, cv::Rect& proj_bbox_rcam);

}  // namespace aisdk::algorithm

#endif
