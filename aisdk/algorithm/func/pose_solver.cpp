#include "aisdk/algorithm/func/pose_solver.h"

#include "aisdk/base/type.h"

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

Eigen::MatrixXf cal_kpt_weight(const std::vector<float>& sigma) {
    const int weight_num = 42;
    const int num_kpts = 21;

    // Reshape sigma (1, 21, 3) equivalent by dividing into groups of 3
    Eigen::MatrixXf sigma_matrix(num_kpts, 3);
    for (int i = 0; i < num_kpts; ++i) {
        sigma_matrix(i, 0) = sigma[i * 3 + 0];
        sigma_matrix(i, 1) = sigma[i * 3 + 1];
        sigma_matrix(i, 2) = sigma[i * 3 + 2];
    }

    // Calculate mean along the last axis (axis=-1 in Python)
    Eigen::VectorXf sigma_kpt = sigma_matrix.rowwise().mean();

    // Softmax calculation over sigma_kpt
    float max_sigma = sigma_kpt.maxCoeff();
    Eigen::VectorXf exp_sigma_kpt = (sigma_kpt.array() - max_sigma).exp();
    Eigen::VectorXf sigma_kpt_softmax = exp_sigma_kpt / exp_sigma_kpt.sum();

    // Repeat and reshape to (1, 42) by duplicating each value in sigma_kpt_softmax
    Eigen::VectorXf sigma_kpt_softmax_repeated(weight_num);
    for (int i = 0; i < num_kpts; ++i) {
        sigma_kpt_softmax_repeated(2 * i) = sigma_kpt_softmax(i);
        sigma_kpt_softmax_repeated(2 * i + 1) = sigma_kpt_softmax(i);
    }

    // Create weight matrix and set diagonal values
    Eigen::MatrixXf kpt_weight = Eigen::MatrixXf::Identity(weight_num, weight_num);
    kpt_weight.diagonal() = sigma_kpt_softmax_repeated * 21;

    return kpt_weight;
}

Eigen::Matrix<float, 21, 3> get_3d_kpt(const Eigen::Matrix<float, 21, 3>& hand3d_rel,  // (21, 3)
                                       const std::vector<Vec2f_t>& cood_2d,            // (21, 2)
                                       const Eigen::Matrix3f& intrix_matrix,           // (3, 3)
                                       const Eigen::MatrixXf& W)                       // (42, 42)
{
    int K = 21;

    // Step 1: Prepare `cood_2d` with homogeneous coordinates (adding a 1 to each point).
    Eigen::Matrix<float, 21, 3> cood_2d_homo;
    for (int i = 0; i < K; ++i) {
        cood_2d_homo.row(i) << cood_2d[i].x(), cood_2d[i].y(), 1.0f;
    }

    // Step 2: Calculate `uv_cood_leftmatrix`
    Eigen::Matrix<float, 21, 2> uv_cood_leftmatrix;
    Eigen::Matrix3f intrix_matrix_inv = intrix_matrix.inverse();
    for (int i = 0; i < K; ++i) {
        Eigen::Vector3f uv_homogeneous = intrix_matrix_inv * cood_2d_homo.row(i).transpose();
        uv_cood_leftmatrix.row(i) = uv_homogeneous.head<2>();  // Take x and y
    }

    // Step 3: Set up matrix A (shape (2 * K, 3))
    Eigen::MatrixXf A = Eigen::MatrixXf::Zero(2 * K, 3);
    for (int i = 0; i < K; ++i) {
        A(2 * i, 0) = -1;
        A(2 * i + 1, 1) = -1;
        A(2 * i, 2) = uv_cood_leftmatrix(i, 0);
        A(2 * i + 1, 2) = uv_cood_leftmatrix(i, 1);
    }

    // Step 4: Set up matrix B (shape (2 * K, 1))
    Eigen::MatrixXf B = Eigen::MatrixXf::Zero(2 * K, 1);
    for (int i = 0; i < K; ++i) {
        B(2 * i, 0) = hand3d_rel(i, 0) - hand3d_rel(i, 2) * uv_cood_leftmatrix(i, 0);
        B(2 * i + 1, 0) = hand3d_rel(i, 1) - hand3d_rel(i, 2) * uv_cood_leftmatrix(i, 1);
    }

    // Step 5: Compute `result` matrix using `W` as weight matrix
    Eigen::MatrixXf A_T_W = A.transpose() * W;
    Eigen::MatrixXf part_1 = (A_T_W * A).inverse();
    Eigen::MatrixXf part_2 = A_T_W * B;
    Eigen::Matrix<float, 1, 3> result = (part_1 * part_2).transpose();

    // Step 6: Calculate the final `hand3d` by adding `result` to `hand3d_rel`
    Eigen::Matrix<float, 21, 3> hand3d = hand3d_rel;
    for (int i = 0; i < K; ++i) {
        hand3d.row(i) += result;
    }

    return hand3d;
}
}  // namespace aisdk::algorithm
