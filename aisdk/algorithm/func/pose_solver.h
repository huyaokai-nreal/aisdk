#pragma  once 
#include "Eigen/Dense"
namespace aisdk::algorithm {
    Eigen::Isometry3f get_transform_with_svd(const Eigen::Matrix<float, 7, 3>& src_pts, const Eigen::Matrix<float, 7, 3>& dst_pts);

}