#ifndef __AIK__
#define __AIK__

#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <vector>

// #include <opencv2/opencv.hpp>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/SVD>

Eigen::Matrix3f axisang2mat(Eigen::Vector3f axis, float angle);

std::vector<Eigen::Matrix3f> constraint_IK(std::vector<Eigen::Vector3f> T, std::vector<Eigen::Vector3f> P,
										   bool left_hand);

std::vector<Eigen::Vector3f> constraint_hand_v2(std::vector<Eigen::Vector3f> pred_xyz, bool left_hand);

#endif