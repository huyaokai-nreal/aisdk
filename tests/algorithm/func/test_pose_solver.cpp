#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "aisdk/algorithm/func/pose_solver.h"
TEST_CASE("testing the pose svd solver") {
    Eigen::Isometry3f Tg;
    Eigen::Matrix4f M;
    M << -0.83907153, 0.54402111, 0, 1, -0.54402111, -0.83907153, 0, 2, 0, 0, 1, 3, 0, 0, 0, 1;
    Tg = M;
    Eigen::Matrix<float, 7, 3> src_pts;
    src_pts << 0., 0., 0., 0.1, 0., 0., 0., 0.1, 0., 0., 0., 0.1, -0.07071068, -0.07071068, 0., -0.07071068, 0.,
        -0.07071068, 0., -0.07071068, -0.07071068;
    Eigen::Matrix<float, 7, 3> dst_pts = (Tg * (src_pts.transpose())).transpose();
    Eigen::Isometry3f Te = aisdk::algorithm::get_transform_with_svd(src_pts, dst_pts);
    auto error = (Tg.matrix() - Te.matrix());
    auto mean_error = error.cwiseAbs().mean();
    CHECK_LE(mean_error, 1e-6);
}