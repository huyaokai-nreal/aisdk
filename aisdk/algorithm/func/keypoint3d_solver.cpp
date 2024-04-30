#include "keypoint3d_solver.h"

#include <absl/status/status.h>
#include <ceres/tiny_solver.h>
#include <ceres/tiny_solver_autodiff_function.h>

#include <optional>

namespace aisdk::algorithm {
absl::StatusOr<Eigen::Matrix<float, 21, 3>> Keypoint3DSolver::SolveKeypoints(const Eigen::Matrix<float, 21, 3>& kpt25d,
                                                                             float hand_scale,
                                                                             const base::CameraIntrinsics& camera_k,
                                                                             bool flip_x_axis) const {
    Eigen::Matrix<float, 20, 1> user_bones = template_bones_.array();
    Eigen::Matrix<float, 25, 3> norm_kpt3d = Eigen::Matrix<float, 25, 3>::Ones();
    Eigen::Matrix<float, 25, 3> format_kpt3d = Eigen::Matrix<float, 25, 3>::Zero();
    Eigen::Matrix<float, 1, 2> camera_c = {camera_k.cx_, camera_k.cy_};
    Eigen::Matrix<float, 1, 2> camera_f = {camera_k.fx_, camera_k.fy_};
    for (int i = 0; i < 5; i++) {
        format_kpt3d.block<1, 3>(5 * i, 0) = kpt25d.block<1, 3>(0, 0);
        format_kpt3d.block<4, 3>(5 * i + 1, 0) = kpt25d.block<4, 3>(4 * i + 1, 0);
    }
    norm_kpt3d.block<25, 2>(0, 0) =
        ((format_kpt3d.block<25, 2>(0, 0).array().rowwise() - camera_c.array().row(0)).rowwise() /
         camera_f.array().row(0))
            .matrix();
    auto rel_depth = format_kpt3d.block<25, 1>(0, 2);
    using AutoDiffFunction = ceres::TinySolverAutoDiffFunction<CostFunctor, 20, 1>;
    CostFunctor cost_functor(norm_kpt3d, rel_depth, user_bones);
    AutoDiffFunction kpt_function(cost_functor);
    ceres::TinySolver<AutoDiffFunction> solver;
    solver.options.max_num_iterations = 10;
    Eigen::Matrix<double, 1, 1> kpt_root(0.5);
    auto summary = solver.Solve(kpt_function, &kpt_root);
    if (summary.final_cost < converage_cost_th_) {
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
        valid_kpt3d *= hand_scale;
        return valid_kpt3d;
    }
    return absl::UnavailableError("kpt3d solver failed to coverage");
}
// void Keypoint3DSolver::test() {
//     std::vector<float> kpt2d_data = {
//         2.99081134e+02, 4.68309402e+02,
//         0.00000000e+00, 2.78285093e+02, 4.59855592e+02, 1.32577026e-02, 2.54927623e+02,
//         4.46531351e+02, 3.26830771e-02, 2.43284140e+02, 4.34032556e+02, 5.08023827e-02, 2.36762150e+02, 4.25591465e+02,
//         6.49793228e-02, 2.73127903e+02, 4.14827664e+02, 3.89676258e-02, 2.63941740e+02, 3.92972111e+02, 4.28356587e-02,
//         2.58573485e+02, 3.81066990e+02, 4.99500539e-02, 2.55460169e+02, 3.72273413e+02, 5.50994928e-02, 2.86972177e+02,
//         4.13633138e+02, 3.70544242e-02, 2.83156790e+02, 3.87940763e+02, 4.28731947e-02, 2.80546375e+02, 3.73024666e+02,
//         5.60149618e-02, 2.79156206e+02, 3.63427399e+02, 6.71946005e-02, 2.97756489e+02, 4.17028706e+02, 3.77615507e-02,
//         2.97878970e+02, 3.92930884e+02, 4.33288447e-02, 2.97370799e+02, 3.79275567e+02, 5.76397955e-02, 2.95356341e+02,
//         3.70190960e+02, 7.04263186e-02, 3.09441783e+02, 4.21758131e+02, 3.85778209e-02, 3.10063539e+02, 4.02320596e+02,
//         4.42498364e-02, 3.12394303e+02, 3.92277787e+02, 5.09016057e-02, 3.12538192e+02, 3.83270104e+02, 5.89814812e-02};
//     Eigen::Map<Eigen::Matrix<float, 21, 3, Eigen::RowMajor>> kpt2d_depth(kpt2d_data.data());
//     CameraIntrinsics camera_k{
//         .cx = 238.24292414176563,
//         .cy = 318.9205573206751,
//         .fx = 240.47993898902308,
//         .fy = 240.45010798807022,
//     };
//     CameraDistortion camera_d{.camera_model = CameraModel::CAMERA_MODEL_RADIAL,
//                               .radial_k1 = 0,
//                               .radial_k2 = 0,
//                               .radial_p1 = 0,
//                               .radial_p2 = 0,
//                               .radial_k3 = 0};
//     Eigen::Isometry3f xf = Eigen::Isometry3f::Identity();
//     OpenCVPinholeCameraModel camera(camera_k, camera_d, xf);
//     {
//         TIMER_ONCE_WITH_TAG("solve_kpt_time");
//         auto result = SolveKeypoints(kpt2d_depth, 1.0, camera, false);
//     }
// }

}  // namespace aisdk::algorithm