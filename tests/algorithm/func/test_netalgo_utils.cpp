#include <glog/logging.h>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "aisdk/algorithm/func/netalgo_utils.h"

TEST_CASE("testing standard stereo input") {
    Eigen::Matrix4d T;
    T << 0.98701491, 0.00404573, 0.16057769, 0.13565676, -0.00198581, 0.9999137, -0.01298659, 0.00269705, -0.16061638,
        0.01249909, 0.98693777, -0.01155529, 0., 0., 0., 1.;
    auto [left_R, right_R, baseline] = aisdk::algorithm::get_rotations_for_standard_stereo(T);
    Eigen::Matrix3d left_R_gt;
    left_R_gt << 0.996196, 0.0198058, -0.0848563, -0.0198058, 0.999803, 0.000841925, 0.0848563, 0.000841925, 0.996393;
    Eigen::Matrix3d right_R_gt;
    right_R_gt << 0.996852, 0.0227734, 0.075944, -0.0216693, 0.999648, -0.0153333, -0.0763002, 0.0136437, 0.997201;
    CHECK(left_R.isApprox(left_R_gt, 1e-6));
    CHECK(right_R.isApprox(right_R_gt, 1e-6));
    CHECK(baseline == doctest::Approx(0.136175).epsilon(1e-6));
}