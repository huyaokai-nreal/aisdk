#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

#include "Eigen/Dense"
#include "aisdk/base/mem_buffer.h"
#include "aisdk/xengine/nrhal_common.h"

namespace aisdk::algorithm {

enum class InputDataCategory {
    UNKNOWN = 0,
    IS_HAL_TENSOR = 1,
    IS_HAL_IMAGE = 2,
};

struct CamInfo {
    Eigen::Isometry3f cvL_T_cvR;

    cv::Mat lcam_intrinsics;
    cv::Mat rcam_intrinsics;

    cv::Mat lcam_dist_coeffs;
    cv::Mat rcam_dist_coeffs;

    int camera_type;
    // GL系1: nrsdk_api for real_camera
    // opencv系 2: nreal_studio/slam_raw_config for test 
    int generate_method;

    uint32_t video_width;
    uint32_t video_height;
};

struct DetectRect {
    float x;
    float y;
    float w;
    float h;
    float confidence;
    float left_confidence;
    float right_confidence;
    bool is_left;
    bool nms_suppressed;
};

struct Image {
    std::shared_ptr<aisdk::base::XrMem> m_warpmem;
    cv::Mat m_mat;

    Image() {}

    Image(const cv::Mat &mat) { m_mat = mat; }

    Image(const cv::Mat &mat, std::shared_ptr<aisdk::base::XrMem> &warp_mem) {
        m_mat = mat;
        m_warpmem = warp_mem;
    }
};

}  // namespace aisdk::algorithm
