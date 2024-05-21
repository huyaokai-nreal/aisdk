#pragma once
#include <absl/status/statusor.h>
#include "Eigen/Dense"
#include <utility>
#include "aisdk/base/camera_model.h"
namespace aisdk::algorithm {
class Keypoint3DSolver {
   public:
    Keypoint3DSolver() {
        template_bones_ << 0.03324628, 0.04288861, 0.02933636, 0.0189692,
            0.08489925 ,0.03961091 ,0.02224475 ,0.01538656,
            0.08044697 ,0.04323351 ,0.02637224 ,0.01726901,
            0.07485605 ,0.04020233 ,0.02505282 ,0.01739206,
            0.07221888 ,0.03225046 ,0.01788924 ,0.01490044;
    }
    [[nodiscard]] absl::StatusOr<Eigen::Matrix<float, 21, 3>> SolveKeypoints(const Eigen::Matrix<float, 21, 3>& kpt25d,
                                                              float hand_scale, const Eigen::Matrix<float, 21, 3>& last_kpt3d, float last_kpt3d_weight, const base::CameraIntrinsics& camera_k,
                                                              bool flip_x_axis) const;

   private:
    struct CostFunctor {
        CostFunctor(Eigen::Matrix<float, 25, 3>  norm_kpt3d, Eigen::Matrix<float, 25, 1>  rel_depth,
                    Eigen::Matrix<float, 20, 1>  bones, Eigen::Matrix<float, 21, 3> last_kpt3d, float last_kpt3d_weight)
            : norm_kpt3d_(std::move(norm_kpt3d)), rel_depth_(std::move(rel_depth)), bones_(std::move(bones)), last_kpt3d_(std::move(last_kpt3d)), last_kpt3d_weight_(last_kpt3d_weight) {}

        template <typename T>
        bool operator()(const T* const _root_depth, T* residual) const {
            auto root_depth = _root_depth[0];
            Eigen::Matrix<T, 25, 3> kpt3d = Eigen::Matrix<T, 25, 3>::Ones();
            Eigen::Matrix<T, 25, 3> norm_kpt3d = norm_kpt3d_.cast<T>();
            Eigen::Matrix<T, 25, 1> rel_depth = rel_depth_.cast<T>();
            Eigen::Matrix<T, 20, 1> bones = bones_.cast<T>();
            Eigen::Matrix<T, 21, 3> last_kpt3d = last_kpt3d_.cast<T>();
            kpt3d = norm_kpt3d.array().colwise() * (root_depth + rel_depth.array().col(0));
            Eigen::Matrix<T, 20, 1> e_bones = Eigen::Matrix<T, 20, 1>::Ones();
            Eigen::Map<Eigen::Matrix<T, 20, 1>> result(residual);
            e_bones.template block<4, 1>(0, 0) =
                (kpt3d.template block<4, 3>(1, 0) - kpt3d.template block<4, 3>(0, 0)).rowwise().norm();
            e_bones.template block<4, 1>(4, 0) =
                (kpt3d.template block<4, 3>(6, 0) - kpt3d.template block<4, 3>(5, 0)).rowwise().norm();
            e_bones.template block<4, 1>(8, 0) =
                (kpt3d.template block<4, 3>(11, 0) - kpt3d.template block<4, 3>(10, 0)).rowwise().norm();
            e_bones.template block<4, 1>(12, 0) =
                (kpt3d.template block<4, 3>(16, 0) - kpt3d.template block<4, 3>(15, 0)).rowwise().norm();
            e_bones.template block<4, 1>(16, 0) =
                (kpt3d.template block<4, 3>(21, 0) - kpt3d.template block<4, 3>(20, 0)).rowwise().norm();
            result = e_bones - bones;
            residual[20] = (kpt3d(0, 2) - last_kpt3d(0, 2));
            residual[20] *= T(last_kpt3d_weight_);
            return true;
        }
        Eigen::Matrix<float, 25, 3> norm_kpt3d_;
        Eigen::Matrix<float, 25, 1> rel_depth_;
        Eigen::Matrix<float, 20, 1> bones_;
        Eigen::Matrix<float, 21, 3> last_kpt3d_;
        float last_kpt3d_weight_;
    };
    float converage_cost_th_ = 5e-3;
    Eigen::Matrix<float, 20, 1> template_bones_;
};

}  // namespace aisdk::algorithm