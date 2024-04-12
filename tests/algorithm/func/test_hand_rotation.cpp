#include <opencv2/core/matx.hpp>
#include <vector>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "aisdk/algorithm/func/hand_rotation.h"
/*
[[   4.92867047    4.02336109 -103.55305733]
 [   5.05712707   -1.15956522 -114.56521424]
 [   1.78547145  -11.20192062 -123.08335293]
 [  -3.63650955  -21.68715163 -126.79463752]]
 */

TEST_CASE("testing the pose svd solver") {
    std::vector<cv::Vec3f> kpts{{9.456082, -23.14286, -102.04225},   {-2.481389, 1.7483234, -82.89849},
                                {-22.376545, 37.343575, -81.94794},  {-41.47552, 58.679825, -78.18042},
                                {-55.764217, 67.73949, -83.255775},  {-2.033105, 52.642273, -123.78874},
                                {-30.565258, 76.83467, -131.10605},  {-51.843487, 73.819176, -126.12402},
                                {-64.35334, 70.50857, -118.5594},    {-3.7421026, 42.81362, -139.6149},
                                {-39.343044, 64.95279, -146.91512},  {-61.417873, 58.841316, -136.93018},
                                {-70.0507, 52.31716, -124.94162},    {-8.268178, 26.325562, -151.32083},
                                {-43.674435, 43.029274, -154.39574}, {-62.485165, 40.709145, -139.9269},
                                {-71.35381, 34.714935, -127.356155}, {-16.500355, 11.297039, -157.28973},
                                {-45.16375, 20.769608, -167.20609},  {-59.602566, 19.249035, -159.10095},
                                {-70.787415, 20.62072, -150.15366}};
    auto metacarpal_joints = get_metacarpal_joints_v1(kpts);
    Eigen::Vector3f index_meta {metacarpal_joints[0][0], metacarpal_joints[0][1], metacarpal_joints[0][2]};
    CHECK(index_meta.isApprox(Eigen::Vector3f{4.92867047,4.02336109,-103.55305733}));
    Eigen::Vector3f middle_meta {metacarpal_joints[1][0], metacarpal_joints[1][1], metacarpal_joints[1][2]};
    CHECK(middle_meta.isApprox(Eigen::Vector3f{5.05712707,-1.15956522,-114.56521424}));
    Eigen::Vector3f ring_meta {metacarpal_joints[2][0], metacarpal_joints[2][1], metacarpal_joints[2][2]};
    CHECK(ring_meta.isApprox(Eigen::Vector3f{1.78547145, -11.20192062, -123.08335293}));
    Eigen::Vector3f little_meta {metacarpal_joints[3][0], metacarpal_joints[3][1], metacarpal_joints[3][2]};
    CHECK(little_meta.isApprox(Eigen::Vector3f{-3.63650955, -21.68715163, -126.79463752}));
    std::cout << metacarpal_joints[0] << "\n";
    std::cout << metacarpal_joints[1] << "\n";
    std::cout << metacarpal_joints[2] << "\n";
    std::cout << metacarpal_joints[3] << "\n";

    };