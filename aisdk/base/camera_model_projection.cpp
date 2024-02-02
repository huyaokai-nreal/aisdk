#include "camera_model_projection.h"

#include "cmath"
namespace aisdk {
constexpr float PI = 3.14159265358979323846;
constexpr float EPS = 1e-5;
float sinc(float x) {
    if (std::abs(x) < EPS) {
        return 1.0;
    }
    return std::sin(PI * x) / (PI * x);
}

std::vector<Eigen::Vector2f> PerspectiveProjection::project(const std::vector<Eigen::Vector3f>& point_3d) {
    std::vector<Eigen::Vector2f> result_2d(point_3d.size());
    for (size_t i = 0; i < point_3d.size(); i++) {
        result_2d[i] = point_3d[i].head<2>() / point_3d[i](2);
    }
    return result_2d;
}

std::vector<Eigen::Vector3f> PerspectiveProjection::unproject(const std::vector<Eigen::Vector2f>& point_2d) {
    std::vector<Eigen::Vector3f> result_3d(point_2d.size());
    for (size_t i = 0; i < point_2d.size(); i++) {
        Eigen::Vector3f result_temp;
        result_temp << point_2d[i], 1;
        result_temp.normalize();

        result_3d[i] = result_temp;
    }
    return result_3d;
}

std::vector<Eigen::Vector2f> ArctanProjection::project(const std::vector<Eigen::Vector3f>& point_3d) {
    std::vector<Eigen::Vector2f> result_2d(point_3d.size());
    for (size_t i = 0; i < point_3d.size(); i++) {
        const auto& x = point_3d[i](0);
        const auto& y = point_3d[i](1);
        const auto& z = point_3d[i](2);

        float r = std::sqrt(x * x + y * y);
        float s = std::atan2(r, z) / std::max(r, EPS);

        result_2d[i] = Eigen::Vector2f(x * s, y * s);
    }
    return result_2d;
}

std::vector<Eigen::Vector3f> ArctanProjection::unproject(const std::vector<Eigen::Vector2f>& point_2d) {
    std::vector<Eigen::Vector3f> result_3d(point_2d.size());
    for (size_t i = 0; i < point_2d.size(); i++) {
        const auto& u = point_2d[i](0);
        const auto& v = point_2d[i](1);

        float r = std::sqrt(u * u + v * v);
        float c = std::cos(r);
        float s = sinc(r / PI);

        result_3d[i] = Eigen::Vector3f(u * s, v * s, c);
    }
    return result_3d;
}

}  // namespace aisdk