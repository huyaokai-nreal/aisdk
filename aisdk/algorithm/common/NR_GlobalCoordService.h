#pragma once

#include <Eigen/Dense>
#include <opencv2/core/core.hpp>
#include <vector>

#include "coord_transform_service.h"

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

    cv::Vec3f transform(const CoordinateSystemEnum& from, const CoordinateSystemEnum& to, const cv::Vec3f& point_cv) {
        Eigen::Matrix<TransformDataType, 3, 1> point_eigen(point_cv[0], point_cv[1], point_cv[2]);
        Eigen::Matrix<TransformDataType, 3, 1> result_eigen = service.transform(from, to, point_eigen);
        return cv::Vec3f(result_eigen[0], result_eigen[1], result_eigen[2]);
    }

    std::vector<cv::Vec3f> transform(const CoordinateSystemEnum& from, const CoordinateSystemEnum& to,
                                     const std::vector<cv::Vec3f>& point_cv) {
        std::vector<cv::Vec3f> res(point_cv.size());
        for (size_t i = 0; i < point_cv.size(); i++) {
            Eigen::Matrix<TransformDataType, 3, 1> point_eigen(point_cv[i][0], point_cv[i][1], point_cv[i][2]);
            Eigen::Matrix<TransformDataType, 3, 1> result_eigen = service.transform(from, to, point_eigen);
            res[i] = cv::Vec3f(result_eigen[0], result_eigen[1], result_eigen[2]);
        }
        return res;
    }

   private:
    GlobalCoordService() = default;

    CoordTransformService<CoordinateSystemEnum, CoordSystemSize, TransformDataType> service;
};
