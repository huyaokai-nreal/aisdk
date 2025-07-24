#pragma  once 
#include "Eigen/Dense"
#include <vector>
#include "aisdk/base/type.h"
namespace aisdk::algorithm {
    Eigen::Isometry3f get_transform_with_svd(const Eigen::Matrix<float, 7, 3>& src_pts, const Eigen::Matrix<float, 7, 3>& dst_pts);
    std::vector<float> decode_hand_angle(std::vector<float>& x);
    Eigen::MatrixXf cal_kpt_weight(const std::vector<float>& sigma);
    Eigen::Matrix<float, 21, 3> get_3d_kpt(
        const Eigen::Matrix<float, 21, 3>& hand3d_rel,  
        const std::vector<Vec2f_t>& cood_2d,            
        const Eigen::Matrix3f& intrix_matrix,           
        const Eigen::MatrixXf& W); 
}