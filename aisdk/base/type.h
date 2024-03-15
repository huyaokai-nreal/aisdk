#pragma  once
#include <Eigen/Dense>
namespace aisdk {
#ifndef M_PI
// M_PI is not part of the C++ standard. Rather it is part of the POSIX standard. As such,
// it is not directly available on Visual C++ (although _USE_MATH_DEFINES does exist).
#define M_PI 3.14159265358979323846
#endif
template<size_t R>
using VecR_t = Eigen::Matrix<double, R, 1>;
using Vec2_t = Eigen::Vector2f;
using Vec4_t = Eigen::Vector4f;
template<size_t R, size_t C>
using MatRC_t = Eigen::Matrix<float, R, C>;
using  Mat21_2_t =  MatRC_t<21, 2>;
using  Mat21_3_t =  MatRC_t<21, 3>;

}