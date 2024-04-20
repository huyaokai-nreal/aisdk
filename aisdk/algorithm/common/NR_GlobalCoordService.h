#pragma once

#include <Eigen/Dense>
#include <opencv2/core/core.hpp>
#include <vector>

#include "aisdk/base/coord_transform_service.h"
#include "aisdk/base/type.h"
namespace aisdk::algorithm {

enum class XrealCoordSystem { CV_LEFT_CAM, GL_LEFT_CAM, GL_HEAD, GL_RIGHT_CAM, CV_RIGHT_CAM, COORD_MAX_COUNT = 5 };

using CoordinateSystemEnum = XrealCoordSystem;
constexpr std::size_t CoordSystemSize = static_cast<std::size_t>(XrealCoordSystem::COORD_MAX_COUNT);
using TransformDataType = float;

class GlobalCoordService {
   public:
    GlobalCoordService(const GlobalCoordService&) = delete;
    GlobalCoordService& operator=(const GlobalCoordService&) = delete;

    static GlobalCoordService* getInstance() {
        static GlobalCoordService instance;
        return &instance;
    }

    void finalize() { service.finalize(); }

    void setTransform(const CoordinateSystemEnum& from, const CoordinateSystemEnum& to,
                      const Eigen::Matrix<TransformDataType, 3, 3>& rotation,
                      const Eigen::Matrix<TransformDataType, 3, 1>& translation) {
        service.setTransform(from, to, rotation, translation);
    }

    void setTransform(const CoordinateSystemEnum& from, const CoordinateSystemEnum& to,
                      const Eigen::Quaternion<TransformDataType>& rotation,
                      const Eigen::Matrix<TransformDataType, 3, 1>& translation) {
        service.setTransform(from, to, rotation, translation);
    }

    Vec3f_t transform(const CoordinateSystemEnum& from, const CoordinateSystemEnum& to, const Vec3f_t& point_cv) {
        return  service.transform(from, to, point_cv);
    }

    std::vector<Vec3f_t> transform(const CoordinateSystemEnum& from, const CoordinateSystemEnum& to,
                                   const std::vector<Vec3f_t>& point_cv) {
        std::vector<Vec3f_t> res(point_cv.size());
        for (size_t i = 0; i < point_cv.size(); i++) {
            res[i] = service.transform(from, to, point_cv[i]);
        }
        return res;
    }

   private:
    GlobalCoordService() = default;

    aisdk::base::CoordTransformService<CoordinateSystemEnum, CoordSystemSize, TransformDataType> service;
};

}  // namespace aisdk::algorithm