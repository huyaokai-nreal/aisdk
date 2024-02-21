#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "aisdk/base/coord_transform_service.h"
using namespace aisdk::base;
TEST_CASE("testing the pose transform") {
    enum class CoordSystem { LEFT, RIGHT, ROOT };
    float roll = 1.5707, pitch = 0, yaw = 0.707;
    Eigen::Quaternionf q;
    q = Eigen::AngleAxisf(roll, Eigen::Vector3f::UnitX()) * Eigen::AngleAxisf(pitch, Eigen::Vector3f::UnitY()) *
        Eigen::AngleAxisf(yaw, Eigen::Vector3f::UnitZ());
    CoordTransformService<CoordSystem, 3, float> transform_service;
    Eigen::Vector3f p{1, 1, 1};
    transform_service.setTransform(CoordSystem::LEFT, CoordSystem::ROOT, Eigen::Quaternionf::Identity(),
                                   Eigen::Vector3f::Zero());
    transform_service.setTransform(CoordSystem::ROOT, CoordSystem::RIGHT, q, p);
    transform_service.finalize();
    Eigen::Vector3f src_point{1, 1, 1};
    Eigen::Vector3f dst_point = transform_service.transform(CoordSystem::LEFT, CoordSystem::ROOT, src_point);
    CHECK_EQ(src_point.x(), dst_point.x());
    CHECK_EQ(src_point.y(), dst_point.y());
    CHECK_EQ(src_point.z(), dst_point.z());
    dst_point = transform_service.transform(CoordSystem::LEFT, CoordSystem::RIGHT, src_point);
    Eigen::Vector3f gt_point = q * src_point + p;
    CHECK_LT(abs(dst_point.x()-gt_point.x()), 1e-6);
    CHECK_LT(abs(dst_point.y()-gt_point.y()), 1e-6);
    CHECK_LT(abs(dst_point.z()-gt_point.z()), 1e-6);
}