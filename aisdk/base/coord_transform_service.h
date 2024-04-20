#pragma once
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <array>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "Eigen/src/Geometry/Transform.h"

namespace aisdk::base {

template <typename CoordinateSystemEnum, std::size_t N, typename ScalarType>
class CoordTransformService {
   public:
    using Vector3 = Eigen::Matrix<ScalarType, 3, 1>;
    using Isometry3 = Eigen::Transform<ScalarType, 3, Eigen::Isometry>;

    CoordTransformService() {
        for (auto& row : m_transforms) {
            for (auto& elem : row) {
                elem = Isometry3::Identity();
            }
        }

        for (auto& row : m_transformSet) {
            for (auto& elem : row) {
                elem = false;
            }
        }
    }

    void finalize() {
        for (std::size_t i = 0; i < N; ++i) {
            for (std::size_t j = 0; j < N; ++j) {
                if (i != j && !m_transformSet[i][j]) {
                    std::set<CoordinateSystemEnum> visited;
                    calculateTransform(static_cast<CoordinateSystemEnum>(i), static_cast<CoordinateSystemEnum>(j),
                                       visited);
                    if (!m_transformSet[i][j]) {
                        std::string err_message = "Failed to calculate transform from coord: " + std::to_string(i) +
                                                  " to coord: " + std::to_string(j) +
                                                  ". Please check your initialization settings!";
                        throw std::runtime_error(err_message);
                    }
                }
            }
        }
    }

    void setTransform(const CoordinateSystemEnum& from, const CoordinateSystemEnum& to,
                      const Eigen::Matrix<ScalarType, 3, 3>& rotation, const Vector3& translation) {
        static_assert(std::is_same<ScalarType, typename std::remove_reference_t<decltype(rotation)>::Scalar>::value,
                      "Mismatched precision types in rotation matrix.");
        static_assert(std::is_same<ScalarType, typename std::remove_reference_t<decltype(translation)>::Scalar>::value,
                      "Mismatched precision types in translation vector.");

        if (!isValidRotationMatrix(rotation)) {
            throw std::runtime_error("Invalid rotation matrix provided.");
        }

        Isometry3 transform;
        transform.setIdentity();
        transform.rotate(rotation);
        transform.pretranslate(translation);

        setTransformInternal(from, to, transform);
        setTransformInternal(to, from, transform.inverse());
    }

    void setTransform(const CoordinateSystemEnum& from, const CoordinateSystemEnum& to,
                      const Eigen::Quaternion<ScalarType>& rotation, const Vector3& translation) {
        static_assert(std::is_same<ScalarType, typename std::remove_reference_t<decltype(rotation)>::Scalar>::value,
                      "Mismatched precision types in rotation matrix.");
        static_assert(std::is_same<ScalarType, typename std::remove_reference_t<decltype(translation)>::Scalar>::value,
                      "Mismatched precision types in translation vector.");

        if (!isValidQuaternion(rotation)) {
            throw std::runtime_error("Invalid quaternion provided.");
        }

        Isometry3 transform;
        transform.setIdentity();
        transform.rotate(rotation);
        transform.pretranslate(translation);

        setTransformInternal(from, to, transform);
        setTransformInternal(to, from, transform.inverse());
    }

    Isometry3 getTransform(const CoordinateSystemEnum& from, const CoordinateSystemEnum& to) const {
        return m_transforms[static_cast<std::size_t>(from)][static_cast<std::size_t>(to)];
    }

    Vector3 transform(const CoordinateSystemEnum& from, const CoordinateSystemEnum& to, const Vector3& point) const {
        return m_transforms[static_cast<std::size_t>(from)][static_cast<std::size_t>(to)] * point;
    }

    Eigen::Isometry3f get_transform(const CoordinateSystemEnum& from, const CoordinateSystemEnum& to) {
        return m_transforms[static_cast<std::size_t>(from)][static_cast<std::size_t>(to)];
    }

    std::vector<Vector3> transform(const CoordinateSystemEnum& from, const CoordinateSystemEnum& to,
                                   const std::vector<Vector3>& points) const {
        std::vector<Vector3> transformed_points;
        transformed_points.reserve(points.size());
        const auto& isometry = m_transforms[static_cast<std::size_t>(from)][static_cast<std::size_t>(to)];
        for (const auto& point : points) {
            transformed_points.emplace_back(isometry * point);
        }
        return transformed_points;
    }

   private:
    void setTransformInternal(const CoordinateSystemEnum& from, const CoordinateSystemEnum& to,
                              const Isometry3& transform) {
        Isometry3& isometry = m_transforms[static_cast<std::size_t>(from)][static_cast<std::size_t>(to)];
        isometry = transform;
        m_transformSet[static_cast<std::size_t>(from)][static_cast<std::size_t>(to)] = true;
    }

    void calculateTransform(const CoordinateSystemEnum& from, const CoordinateSystemEnum& to,
                            std::set<CoordinateSystemEnum>& visited) {
        if (visited.count(from) > 0) {
            return;
        }
        visited.insert(from);

        for (std::size_t i = 0; i < N; ++i) {
            auto intermediate = static_cast<CoordinateSystemEnum>(i);
            if (m_transformSet[static_cast<std::size_t>(from)][i] && m_transformSet[i][static_cast<std::size_t>(to)]) {
                m_transforms[static_cast<std::size_t>(from)][static_cast<std::size_t>(to)] =
                    m_transforms[i][static_cast<std::size_t>(to)] * m_transforms[static_cast<std::size_t>(from)][i];
                m_transformSet[static_cast<std::size_t>(from)][static_cast<std::size_t>(to)] = true;
            } else if (m_transformSet[static_cast<std::size_t>(from)][i]) {
                calculateTransform(intermediate, to, visited);
                if (m_transformSet[i][static_cast<std::size_t>(to)]) {
                    m_transforms[static_cast<std::size_t>(from)][static_cast<std::size_t>(to)] =
                        m_transforms[i][static_cast<std::size_t>(to)] * m_transforms[static_cast<std::size_t>(from)][i];
                    m_transformSet[static_cast<std::size_t>(from)][static_cast<std::size_t>(to)] = true;
                }
            }
        }
    }

    bool isValidRotationMatrix(const Eigen::Matrix<ScalarType, 3, 3>& rotation) {
        Eigen::Matrix<ScalarType, 3, 3> identity = Eigen::Matrix<ScalarType, 3, 3>::Identity();
        return (rotation.transpose() * rotation).isApprox(identity) && std::abs(rotation.determinant() - 1.0) < 1e-6;
    }

    bool isValidQuaternion(const Eigen::Quaternion<ScalarType>& quaternion) {
        return std::abs(quaternion.norm() - 1.0) < 1e-6;
    }

    std::array<std::array<Isometry3, N>, N> m_transforms;
    std::array<std::array<bool, N>, N> m_transformSet;
};

}  // namespace aisdk::base