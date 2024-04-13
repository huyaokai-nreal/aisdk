#include "netalgo_utils.h"

#include <Eigen/src/Geometry/Quaternion.h>

namespace aisdk::algorithm {

void bbox_to_center_and_scale(float* bbox, float* center, float* scale) {
    center[0] = bbox[0] + bbox[2] / 2.0;
    center[1] = bbox[1] + bbox[3] / 2.0;

    scale[0] = bbox[2] * 1.0;
    scale[1] = bbox[3] * 1.0;

    return;
}

void center_scale_to_bbox(float* bbox, float* center, float* scale) {
    bbox[2] = scale[0];
    bbox[3] = scale[1];
    bbox[0] = center[0] - bbox[2] / 2.0;
    bbox[1] = center[1] - bbox[3] / 2.0;

    return;
}

void adjust_bbox(float* bbox, float height, float width, float* bbox_res, float* center, float* scale) {
    bbox_res[0] = std::max(float(0), std::min(width, bbox[0]));
    bbox_res[1] = std::max(float(0), std::min(height, bbox[1]));
    bbox_res[2] = std::max(float(0), std::min(width - bbox[0], bbox[2]));
    bbox_res[3] = std::max(float(0), std::min(height - bbox[1], bbox[3]));

    bbox_to_center_and_scale(bbox_res, center, scale);
    return;
}

void expand_bbox(int min_x, int min_y, int max_x, int max_y, float* bbox) {
    cv::Rect res;
    float center_x, center_y, w, h;
    int crop_margin;

    float cx = (min_x + max_x) * 0.5;
    float cy = (min_y + max_y) * 0.5;

    min_x = (min_x - cx) * 1.5 + cx;
    min_y = (min_y - cy) * 1.4 + cy;
    max_x = (max_x - cx) * 1.5 + cx;
    max_y = (max_y - cy) * 1.4 + cy;

    bbox[0] = min_x;
    bbox[1] = min_y;
    bbox[2] = max_x - min_x;
    bbox[3] = max_y - min_y;

    return;
}

cv::Rect add_bbox_margin(int min_x, int min_y, int max_x, int max_y, int max_w, int max_h) {
    float center[2], scale[2];
    float bbox[4], bbox_crop[4];

    expand_bbox(min_x, min_y, max_x, max_y, bbox);

    // std::cout << bbox[0] << " " << bbox[1] << " " << bbox[2] << " " << bbox[3] << std::endl;

    adjust_bbox(bbox, max_h - 1, max_w - 1, bbox_crop, center, scale);

    bbox_to_center_and_scale(bbox_crop, center, scale);

    //   scale[0] *= 1.25;
    //   scale[1] *= 1.25;

    if (scale[0] > scale[1]) {
        scale[1] = scale[0];
    } else {
        scale[0] = scale[1];
    }

    center_scale_to_bbox(bbox_crop, center, scale);

    cv::Rect bbox_res = {int(bbox_crop[0]), int(bbox_crop[1]), int(bbox_crop[2]), int(bbox_crop[3])};

    return bbox_res;
}

void nms(std::vector<DetectRect>& rect, float iou_threshold) {
    std::stable_sort(rect.begin(), rect.end(),
                     [&](const DetectRect& a, const DetectRect& b) { return a.confidence > b.confidence; });

    auto get_iou_ext = [](const DetectRect& a, const DetectRect& b) {
        if (a.is_left != b.is_left) {
            return -0.1f;
        }
        if (a.x > b.x + b.w) {
            return 0.0f;
        }
        if (a.y > b.y + b.h) {
            return 0.0f;
        }
        if (a.x + a.w < b.x) {
            return 0.0f;
        }
        if (a.y + a.h < b.y) {
            return 0.0f;
        }

        float colInt = std::min(a.x + a.w, b.x + b.w) - std::max(a.x, b.x);
        float rowInt = std::min(a.y + a.h, b.y + b.h) - std::max(a.y, b.y);
        float intersection = colInt * rowInt;
        float area1 = a.w * a.h;
        float area2 = b.w * b.h;
        return intersection / (area1 + area2 - intersection);
    };

    for (unsigned int i = 0; i < rect.size(); i++) {
        auto& boxa = rect[i];
        if (boxa.nms_suppressed) {
            continue;
        }

        for (unsigned int j = i + 1; j < rect.size(); j++) {
            auto& boxb = rect[j];
            if (boxb.nms_suppressed) {
                continue;
            }

            if (get_iou_ext(boxa, boxb) > iou_threshold) {
                boxb.nms_suppressed = true;
            }
        }
    }
}

aisdk::xengine::TensorFormat checkshapeformat(aisdk::xengine::VendorType& vendor, uint32_t m_rank) {
    aisdk::xengine::TensorFormat ret = aisdk::xengine::TensorFormat::UNKNOWN;
    if (vendor == aisdk::xengine::VendorType::SNPE) {
        if (m_rank == 1) {
            ret = aisdk::xengine::TensorFormat::W;
        } else if (m_rank == 2) {
            ret = aisdk::xengine::TensorFormat::HW;  // 不确定
        } else if (m_rank == 3) {
            ret = aisdk::xengine::TensorFormat::HWC;
        } else if (m_rank == 4) {
            ret = aisdk::xengine::TensorFormat::NHWC;
        }
    } else if (vendor == aisdk::xengine::VendorType::MNN) {
        if (m_rank == 1) {
            ret = aisdk::xengine::TensorFormat::W;
        } else if (m_rank == 2) {
            ret = aisdk::xengine::TensorFormat::HW;  // 不确定
        } else if (m_rank == 3) {
            ret = aisdk::xengine::TensorFormat::CHW;
        } else if (m_rank == 4) {
            ret = aisdk::xengine::TensorFormat::NCHW;
        }
    }
    return ret;
}

std::tuple<Eigen::Matrix3d, Eigen::Matrix3d, float> get_rotations_for_standard_stereo(const Eigen::Matrix4d& T) {
    Eigen::Matrix4d left_T = Eigen::Matrix4d::Identity();
    Eigen::Matrix4d right_T = T;
    Eigen::Vector3d baseline_vec = T.block<3, 1>(0, 3);
    Eigen::Vector3d left_x{1, 0, 0};
    Eigen::Matrix3d R_left = Eigen::Quaterniond::FromTwoVectors(left_x, baseline_vec).toRotationMatrix();
    Eigen::Matrix4d virtual_left_T = left_T;
    virtual_left_T.block<3, 3>(0, 0) = R_left * left_T.block<3, 3>(0, 0);
    Eigen::Vector3d right_x = right_T.block<3, 3>(0, 0) * left_x + right_T.block<3, 1>(0, 3) - baseline_vec;
    Eigen::Matrix3d R_right = Eigen::Quaterniond::FromTwoVectors(right_x, baseline_vec).toRotationMatrix();
    Eigen::Matrix4d virtual_right_T = right_T;
    virtual_right_T.block<3, 3>(0, 0) = R_right * right_T.block<3, 3>(0, 0);
    Eigen::Vector3d left_z{0, 0, 1};
    Eigen::Vector3d left_cam_z = virtual_left_T.block<3, 3>(0, 0) * left_z + virtual_left_T.block<3, 1>(0, 3);
    Eigen::Vector3d right_cam_z =
        virtual_right_T.block<3, 3>(0, 0) * left_z + virtual_right_T.block<3, 1>(0, 3) - baseline_vec;
    Eigen::Matrix3d R_right_z = Eigen::Quaterniond::FromTwoVectors(right_cam_z, left_cam_z).toRotationMatrix();
    virtual_right_T.block<3, 3>(0, 0).noalias() = R_right_z * virtual_right_T.block<3, 3>(0, 0);
    Eigen::Matrix3d left_R = (virtual_left_T.inverse() * left_T).block<3, 3>(0, 0);
    Eigen::Matrix3d right_R = (virtual_right_T.inverse() * right_T).block<3, 3>(0, 0);
    double baseline = baseline_vec.norm();
    return {left_R, right_R, baseline};
}

}  // namespace aisdk::algorithm
