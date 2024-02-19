#pragma once
#include <Eigen/Dense>
#include <vector>
namespace aisdk::base {
class CamreaProjection {
   public:
    virtual std::vector<Eigen::Vector2f> project(const std::vector<Eigen::Vector3f>& point_3d) = 0;
    virtual std::vector<Eigen::Vector3f> unproject(const std::vector<Eigen::Vector2f>& point_2d) = 0;
};

class PerspectiveProjection : public CamreaProjection {
   public:
    std::vector<Eigen::Vector2f> project(const std::vector<Eigen::Vector3f>& point_3d) override;
    std::vector<Eigen::Vector3f> unproject(const std::vector<Eigen::Vector2f>& point_2d) override;
};

class ArctanProjection : public CamreaProjection {
   public:
    std::vector<Eigen::Vector2f> project(const std::vector<Eigen::Vector3f>& point_3d) override;
    std::vector<Eigen::Vector3f> unproject(const std::vector<Eigen::Vector2f>& point_2d) override;
};

}  // namespace aisdk::base