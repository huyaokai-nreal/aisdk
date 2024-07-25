#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

#include "Eigen/Dense"
#include "aisdk/base/type.h"

namespace aisdk::algorithm {

bool compute_xr_joint_rotation_v1(const std::vector<Vec3f_t>& joint, bool left_hand,
                            std::vector<Eigen::Matrix3f>& rotations_world);
}