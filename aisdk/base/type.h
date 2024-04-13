#pragma  once
#include <Eigen/Dense>
namespace aisdk {
#ifndef M_PI
// M_PI is not part of the C++ standard. Rather it is part of the POSIX standard. As such,
// it is not directly available on Visual C++ (although _USE_MATH_DEFINES does exist).
#define M_PI 3.14159265358979323846
#endif
template<size_t R, typename T>
using VecR_t = Eigen::Matrix<T, R, 1>;
using Vec2f_t = Eigen::Vector2f;
using Vec4f_t = Eigen::Vector4f;
using  Mat21_2f_t =  Eigen::Matrix<float,21, 2>;
using  Mat21_3f_t =  Eigen::Matrix<float,21, 3>;
}