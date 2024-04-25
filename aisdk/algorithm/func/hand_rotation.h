#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

#include "Eigen/Dense"
#include "aisdk/base/type.h"
namespace aisdk::algorithm {

std::vector<Vec3f_t> get_metacarpal_joints_v1(const std::vector<Vec3f_t>& joints);

bool compute_joint_rotation(const std::vector<Vec3f_t>& joint, bool left_hand,
                            std::vector<Eigen::Matrix3f>& rotations_world);
}
