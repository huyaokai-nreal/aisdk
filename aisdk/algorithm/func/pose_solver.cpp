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

std::vector<float> decode_hand_angle(std::vector<float>& x) {
    Eigen::Matrix<float, 19, 9> output_angle;
    int index = 0;
    for (int i = 0; i < 19; ++i) {
        // 从 index 开始读取9个数值
        std::vector<float> sub_angles(x.begin() + index, x.begin() + index + 9);
        // 在这里对 sub_angles 进行处理
        Eigen::Map<Eigen::Matrix<float, 3, 3, Eigen::RowMajor>> sub_angles_sin(sub_angles.data());

        Eigen::JacobiSVD<Eigen::Matrix3f> svd(sub_angles_sin, Eigen::ComputeFullU | Eigen::ComputeFullV);
        auto u = svd.matrixU();
        const auto& v = svd.matrixV();
        Eigen::Matrix<float, 3, 3, Eigen::RowMajor> vt = v.transpose();
        auto det = (u * vt).determinant();
        vt.row(2) *= det;
        Eigen::Matrix3f pred_matrix = u * vt;
        Eigen::Matrix<float, 1, 9> reshaped_pred_matrix;
        reshaped_pred_matrix << pred_matrix(0, 0), pred_matrix(0, 1), pred_matrix(0, 2), pred_matrix(1, 0),
            pred_matrix(1, 1), pred_matrix(1, 2), pred_matrix(2, 0), pred_matrix(2, 1), pred_matrix(2, 2);
        output_angle.row(i) = reshaped_pred_matrix;
        // 更新 index
        index += 9;
    }
    std::vector<float> output_vector;
    output_vector.reserve(output_angle.size());  // 预先分配内存以提高效率

    for (int i = 0; i < output_angle.rows(); ++i) {
        for (int j = 0; j < output_angle.cols(); ++j) {
            output_vector.push_back(output_angle(i, j));  // 将矩阵中的每个元素添加到向量中
        }
    }
    return output_vector;
}

}  // namespace aisdk::algorithm