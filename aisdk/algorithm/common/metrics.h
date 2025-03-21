#pragma once

#include <memory>
#include <opencv2/opencv.hpp>
#include <vector>

#include "Eigen/Dense"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/type.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "interface/handtracking_sdk/public/nr_plugin_types.h"

namespace aisdk::algorithm {
float compute_mono_rmse_with_reprojection(const std::vector<Vec3f_t>& pred_xyz, const std::vector<Vec2f_t>& uv_ori,
                                           const std::shared_ptr<base::BaseCameraModel>& cam_model);
float compute_score_with_reprojection(const std::vector<Vec3f_t>& pred_xyz, const std::vector<Vec2f_t>& leftcam_uv_ori,
                                      const std::vector<Vec2f_t>& rightcam_uv_ori,
                                      const std::shared_ptr<base::BaseCameraModel>& left_cam,
                                      const std::shared_ptr<base::BaseCameraModel>& right_cam);

float get_bbox_distance(cv::Rect src, cv::Rect dst);

bool check_if_rect_valid(const DetectRect& rect, float max_width, float max_height,float bbox_in_image_ratio_th, float min_bbox_area);
bool isNaN(const std::vector<Vec3f_t>& kpts);
bool isHeadPoseValid(const NRTransform& headpose);
}  // namespace aisdk::algorithm
