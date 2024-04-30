#pragma once
#include <absl/status/statusor.h>
#include "Eigen/Dense"
#include <optional>
#include <utility>
#include "aisdk/base/camera_model.h"
namespace aisdk::algorithm {
class Keypoint3DSolver {
   public:
    explicit Keypoint3DSolver(float last_kpt_weight) : last_kpt_weight_(last_kpt_weight) {
        template_bones_ << 0.0331647, 0.04278148, 0.02955948, 0.02000255, 0.08566785, 0.04031642, 0.02216826,
            0.01616783, 0.08094267, 0.04408906, 0.02601391, 0.01838449, 0.07450476, 0.0409083, 0.02482778, 0.0185005,
            0.07156292, 0.03231286, 0.01820878, 0.01589963;
    }
    [[nodiscard]] absl::StatusOr<Eigen::Matrix<float, 21, 3>> SolveKeypoints(const Eigen::Matrix<float, 21, 3>& kpt25d,
                                                              float hand_scale, const base::CameraIntrinsics& camera_k,
                                                              bool flip_x_axis) const;

   private:
    struct CostFunctor {
        CostFunctor(Eigen::Matrix<float, 25, 3>  norm_kpt3d, Eigen::Matrix<float, 25, 1>  rel_depth,
                    Eigen::Matrix<float, 20, 1>  bones)
            : norm_kpt3d_(std::move(norm_kpt3d)), rel_depth_(std::move(rel_depth)), bones_(std::move(bones)) {}

        template <typename T>
        bool operator()(const T* const _root_depth, T* residual) const {
            auto root_depth = _root_depth[0];
            Eigen::Matrix<T, 25, 3> kpt3d = Eigen::Matrix<T, 25, 3>::Ones();
            Eigen::Matrix<T, 25, 3> norm_kpt3d = norm_kpt3d_.cast<T>();
            Eigen::Matrix<T, 25, 1> rel_depth = rel_depth_.cast<T>();
            Eigen::Matrix<T, 20, 1> bones = bones_.cast<T>();
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
            return true;
        }
        Eigen::Matrix<float, 25, 3> norm_kpt3d_;
        Eigen::Matrix<float, 25, 1> rel_depth_;
        Eigen::Matrix<float, 20, 1> bones_;
    };
    float last_kpt_weight_ = 0.1;
    float converage_cost_th_ = 1e-3;
    Eigen::Matrix<float, 20, 1> template_bones_;
};

}  // namespace aisdk::algorithm