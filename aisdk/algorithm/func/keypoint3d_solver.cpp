#include "keypoint3d_solver.h"

// #include <Eigen/src/Core/Matrix.h>
#include <absl/status/status.h>
#include <ceres/tiny_solver.h>
#include <ceres/tiny_solver_autodiff_function.h>
#include <fmt/format.h>

#include <cstdlib>

#include "aisdk/base/log.h"

namespace aisdk::algorithm {
absl::StatusOr<Eigen::Matrix<float, 21, 3>> Keypoint3DSolver::SolveKeypoints(
    const Eigen::Matrix<float, 21, 3>& kpt25d, float hand_scale, const Eigen::Matrix<float, 21, 3>& last_kpt3d,
    float last_kpt3d_weight, const base::CameraIntrinsics& camera_k, bool flip_x_axis, bool source_change) const {
    AISDK_LOG_WARN("get handscale {}", hand_scale);
    Eigen::Matrix<float, 20, 1> user_bones = template_bones_.array() * hand_scale;
    Eigen::Matrix<float, 25, 3> norm_kpt3d = Eigen::Matrix<float, 25, 3>::Ones();
    Eigen::Matrix<float, 25, 3> format_kpt3d = Eigen::Matrix<float, 25, 3>::Zero();
    Eigen::Matrix<float, 25, 3> format_last_kpt3d = Eigen::Matrix<float, 25, 3>::Zero();
    Eigen::Matrix<float, 1, 2> camera_c = {camera_k.cx_, camera_k.cy_};
    Eigen::Matrix<float, 1, 2> camera_f = {camera_k.fx_, camera_k.fy_};
    for (int i = 0; i < 5; i++) {
        format_kpt3d.block<1, 3>(5 * i, 0) = kpt25d.block<1, 3>(0, 0);
        format_kpt3d.block<4, 3>(5 * i + 1, 0) = kpt25d.block<4, 3>(4 * i + 1, 0);
        format_last_kpt3d.block<1, 3>(5 * i, 0) = last_kpt3d.block<1, 3>(0, 0);
        format_last_kpt3d.block<4, 3>(5 * i + 1, 0) = last_kpt3d.block<4, 3>(4 * i + 1, 0);
    }
    norm_kpt3d.block<25, 2>(0, 0) =
        ((format_kpt3d.block<25, 2>(0, 0).array().rowwise() - camera_c.array().row(0)).rowwise() /
         camera_f.array().row(0))
            .matrix();
    Eigen::Matrix<float, 25, 1> rel_depth = format_kpt3d.block<25, 1>(0, 2).array() * hand_scale;
    constexpr int residual_dim = 20 + 3;
    using AutoDiffFunction = ceres::TinySolverAutoDiffFunction<CostFunctor, residual_dim, 1>;
    CostFunctor cost_functor(norm_kpt3d, rel_depth, user_bones, format_last_kpt3d, last_kpt3d_weight);
    AutoDiffFunction kpt_function(cost_functor);
    ceres::TinySolver<AutoDiffFunction> solver;
    solver.options.max_num_iterations = 10;
    Eigen::Matrix<double, 1, 1> kpt_root(0.5);
    auto summary = solver.Solve(kpt_function, &kpt_root);
    if (summary.final_cost < converage_cost_th_) {
        // smooth the depth change
        if (source_change) {
            if (abs(kpt_root(0, 0) - last_kpt3d(0, 2)) > 0.02) {
                kpt_root(0, 0) = (kpt_root(0, 0) + last_kpt3d(0, 2)) * 0.5;
            }
        }
        auto kpt3d = norm_kpt3d.array().colwise() * (float(kpt_root(0, 0)) + rel_depth.array().col(0));
        Eigen::Matrix<float, 21, 3> valid_kpt3d;
        valid_kpt3d.block<5, 3>(0, 0) = kpt3d.block<5, 3>(0, 0);
        valid_kpt3d.block<4, 3>(5, 0) = kpt3d.block<4, 3>(6, 0);
        valid_kpt3d.block<4, 3>(9, 0) = kpt3d.block<4, 3>(11, 0);
        valid_kpt3d.block<4, 3>(13, 0) = kpt3d.block<4, 3>(16, 0);
        valid_kpt3d.block<4, 3>(17, 0) = kpt3d.block<4, 3>(21, 0);
        if (flip_x_axis) {
            valid_kpt3d.block<21, 1>(0, 0) *= -1;
        }
        return valid_kpt3d;
    }
    return absl::UnavailableError(fmt::format("kpt3d solver failed to coverage, with cost {:.6f}", summary.final_cost));
}
}  // namespace aisdk::algorithm