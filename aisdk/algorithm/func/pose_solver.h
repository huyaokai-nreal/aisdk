#pragma  once 
#include "Eigen/Dense"
#include <vector>
namespace aisdk::algorithm {
    Eigen::Isometry3f get_transform_with_svd(const Eigen::Matrix<float, 7, 3>& src_pts, const Eigen::Matrix<float, 7, 3>& dst_pts);
    std::vector<float> decode_hand_angle(std::vector<float>& x);
}