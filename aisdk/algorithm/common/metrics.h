#pragma once

#include "Eigen/Dense"
#include <opencv2/opencv.hpp>

float compute_rmse(std::vector<cv::Vec2f> lval, std::vector<cv::Vec2f> rval);

float compute_score3d(std::vector<cv::Vec3f> pred_xyz, std::vector<cv::Vec2f> leftcam_uv_ori,
                      std::vector<cv::Vec2f> rightcam_uv_ori, cv::Mat leftcam_cam_matrix, cv::Mat rightcam_cam_matrix,
                      Eigen::Isometry3f m_cvL_T_cvR);

float get_bbox_distance(cv::Rect src, cv::Rect dst);

bool check_if_rect_valid(cv::Rect rect, int max_width, int max_height);
bool check_if_rect_valid_relax(cv::Rect rect, int max_width, int max_height);
bool isNaN(const std::vector<cv::Vec3f>& kpts);
