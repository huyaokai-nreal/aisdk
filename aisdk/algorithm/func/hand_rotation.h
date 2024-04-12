#pragma once

#include <map>
#include <opencv2/opencv.hpp>
#include <vector>

#include "Eigen/Dense"
std::vector<cv::Vec3f> get_metacarpal_joints_v1(const std::vector<cv::Vec3f>& joints);

bool compute_joint_rotation(const std::vector<cv::Vec3f>& joint, bool left_hand,
                            std::vector<Eigen::Matrix3d>& rotations_world,
                            std::vector<Eigen::Matrix3d>& rotations_local);
