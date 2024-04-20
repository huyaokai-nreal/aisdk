#pragma once

#include <memory>
#include <opencv2/opencv.hpp>
#include <vector>

#include "Eigen/Dense"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/type.h"
namespace aisdk::algorithm {
float compute_score_with_reprojection(const std::vector<Vec3f_t>& pred_xyz, const std::vector<Vec2f_t>& leftcam_uv_ori,
                                      const std::vector<Vec2f_t>& rightcam_uv_ori,
                                      const std::shared_ptr<base::BaseCameraModel>& left_cam,
                                      const std::shared_ptr<base::BaseCameraModel>& right_cam);

float get_bbox_distance(cv::Rect src, cv::Rect dst);

bool check_if_rect_valid(cv::Rect rect, int max_width, int max_height);
bool check_if_rect_valid_relax(cv::Rect rect, int max_width, int max_height);
bool isNaN(const std::vector<Vec3f_t>& kpts);

}  // namespace aisdk::algorithm