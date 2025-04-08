#pragma  once
#include <opencv2/opencv.hpp>

#include "Eigen/Dense"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/type.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "interface/handtracking_sdk/include/public/nr_plugin_types.h"

namespace aisdk::algorithm {

void TransferCVToGL(const std::vector<Vec3f_t>& _point3d_src, std::vector<Vec3f_t>& _point3d_dst);

void TransferLeftCamToHead(const std::vector<Vec3f_t>& _point3d_src, std::vector<Vec3f_t>& _point3d_dst);

void TransferHeadToWorld(NRTransform& extrinsic_trans_head_world_, const std::vector<Vec3f_t>& _point3d_src,
                         std::vector<Vec3f_t>& _point3d_dst);

std::vector<Vec3f_t> recal_lcam_kpt3d_cv_with_new_headpose(NRTransform extrinsic_world,
                                                             const std::vector<Vec3f_t>& points3d_src);
std::vector<Vec3f_t> lcam_cv_to_rcam_cv(const std::vector<Vec3f_t>& points3d_src);
std::vector<Vec3f_t> cal_lcam_kpt3d_cv_to_world(NRTransform extrinsic_world,
                                                  const std::vector<Vec3f_t>& points3d_src);
template <typename T>
DetectRect kpts_to_bbox(const T& kps);

void reproj_bbox_with_new_headpose(std::shared_ptr<aisdk::base::BaseCameraModel> lcam_model,
                                   std::shared_ptr<aisdk::base::BaseCameraModel> rcam_model,
                                   NRTransform extrinsics_world, const std::vector<Vec3f_t>& points_3d,
                                   DetectRect& proj_bbox_lcam, DetectRect& proj_bbox_rcam);

}  // namespace aisdk::algorithm

