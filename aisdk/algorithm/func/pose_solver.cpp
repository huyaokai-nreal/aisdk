#include "aisdk/algorithm/func/pose_solver.h"
namespace aisdk::algorithm {
Eigen::Isometry3f get_transform_with_svd(const Eigen::Matrix<float, 7, 3>& src_pts,
                                         const Eigen::Matrix<float, 7, 3>& dst_pts) {
    Eigen::Vector3f mean_src_pt = src_pts.colwise().mean();
    Eigen::Vector3f mean_dst_pt = dst_pts.colwise().mean();
    Eigen::Matrix<float, 7, 3> src_centered_pts = src_pts.rowwise() - mean_src_pt.transpose();
    Eigen::Matrix<float, 7, 3> dst_centered_pts = dst_pts.rowwise() - mean_dst_pt.transpose();
    Eigen::Matrix3f outer_prod = src_centered_pts.transpose() * dst_centered_pts;
    Eigen::JacobiSVD<Eigen::Matrix3f> svd(outer_prod, Eigen::ComputeFullU | Eigen::ComputeFullV);
    auto u = svd.matrixU();
    const auto& v = svd.matrixV();
    auto v_mut_ut = v * (u.transpose());
    Eigen::Matrix3f w = Eigen::Matrix3f::Identity();
    w(2, 2) = v_mut_ut.determinant();
    Eigen::Isometry3f xf = Eigen::Isometry3f::Identity();
    Eigen::Matrix3f rotation = v * w * (u.transpose());
    Eigen::Vector3f pos = mean_dst_pt - rotation * mean_src_pt;
    xf.rotate(rotation);
    xf.pretranslate(pos);
    return xf;
}

}  // namespace aisdk::algorithm