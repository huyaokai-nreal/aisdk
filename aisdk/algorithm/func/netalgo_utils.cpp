#include "netalgo_utils.h"

#include <Eigen/src/Geometry/Quaternion.h>

#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/base/type.h"
#include "thirdparty/MANO_IK-main/mano/AIK.h"

namespace aisdk::algorithm {

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
    } else if (aisdk::xengine::VendorType::ARTOSYN == vendor) {
        if (m_rank == 1) {
            ret = aisdk::xengine::TensorFormat::W;
        } else if (m_rank == 2) {
            ret = aisdk::xengine::TensorFormat::HW;  // 不确定
        } else if (m_rank == 3) {
            ret = aisdk::xengine::TensorFormat::CHW;
        } else if (m_rank == 4) {
            ret = aisdk::xengine::TensorFormat::NCHW;
        }
    } else {
        // do nothing
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

void get_metacarpal_xr_joints_v1(std::vector<Vec3f_t>& joints) {
    auto root_joint = joints[0];
    auto middle_vec = (root_joint - joints[9]).normalized();

    root_joint = joints[9] + 1.2 * (joints[0] - joints[9]).norm() * middle_vec;

    auto little_vec = ((root_joint - joints[9]) + (root_joint - joints[17])).normalized();
    auto little_metacarpal = joints[17] + 0.6667 * ((root_joint - joints[17])).norm() * little_vec;

    auto ring_vec = ((root_joint - joints[9]) + (root_joint - joints[13])).normalized();
    auto ring_metacarpal = joints[13] + 0.6667 * (root_joint - joints[13]).norm() * ring_vec;

    auto middle_metacarpal = joints[9] + 0.6667 * (root_joint - joints[9]).norm() * middle_vec;

    auto index_vec = 2 * middle_vec - ring_vec;
    auto index_metacarpal = joints[5] + 0.6667 * (root_joint - joints[5]).norm() * index_vec;
    // center point
    auto palm_center = joints[9] + 0.333 * (root_joint - joints[9]).norm() * middle_vec;

    joints[0] = root_joint;
    joints[21] = palm_center;
    joints[22] = little_metacarpal;
    joints[23] = index_metacarpal;
    joints[24] = middle_metacarpal;
    joints[25] = ring_metacarpal;
}

Vec3f_t computeNormal(const Vec3f_t& A, const Vec3f_t& B, const Vec3f_t& C, const bool left_hand) {
    Vec3f_t AB = B - A;
    Vec3f_t AC = C - A;

    Vec3f_t normal = (left_hand) ? AC.cross(AB) : AB.cross(AC);
    normal.normalize();
    return normal;
}

std::vector<Vec3f_t> middle_palm_joint(std::vector<Vec3f_t>& input, bool left_hand) {
    std::vector<Vec3f_t> joints(26);
    for (int i = 0; i < input.size(); i++) {
        joints[i] = input[i];
    }

    // 大拇指平面的法向量用于大拇指掌骨点
    Vec3f_t thumb_normal = computeNormal(joints[0], joints[1], joints[21], left_hand);
    auto thumb_moveVector = thumb_normal * 0.005f;
    joints[2] -= thumb_moveVector;

    auto root_joint = joints[0];
    auto middle_vec = (root_joint - joints[9]).normalized();

    // root_joint = joints[9] + 1.2 * (joints[0] - joints[9]).norm() * middle_vec;

    auto little_vec = ((root_joint - joints[9]) + (root_joint - joints[17])).normalized();
    auto little_metacarpal = joints[17] + 0.6667 * ((root_joint - joints[17])).norm() * little_vec;

    auto ring_vec = ((root_joint - joints[9]) + (root_joint - joints[13])).normalized();
    auto ring_metacarpal = joints[13] + 0.6667 * (root_joint - joints[13]).norm() * ring_vec;

    auto middle_metacarpal = joints[9] + 0.6667 * (root_joint - joints[9]).norm() * middle_vec;

    auto index_vec = 2 * middle_vec - ring_vec;
    auto index_metacarpal = joints[5] + 0.6667 * (root_joint - joints[5]).norm() * index_vec;

    joints[22] = (little_metacarpal + joints[22]) / 2;
    joints[23] = (index_metacarpal + joints[23]) / 2;
    joints[24] = (middle_metacarpal + joints[24]) / 2;
    joints[25] = (ring_metacarpal + joints[25]) / 2;

    // 手掌平面的法向量用于移动掌骨点
    Vec3f_t palm_normal = computeNormal(joints[0], joints[5], joints[17], left_hand);
    auto palm_moveVector = palm_normal * 0.005f;
    joints[22] += palm_moveVector;
    joints[23] += palm_moveVector;
    joints[24] += palm_moveVector;
    joints[25] += palm_moveVector;
    joints[23] += (joints[24] - joints[23]).normalized() * 0.008f;

    return joints;
}

std::vector<Vec3f_t> interpolation_to_26points(const std::vector<Vec3f_t>& input) {
    std::vector<Vec3f_t> result(26);
    // input size should be 21
    for (int i = 0; i < input.size(); i++) {
        result[i] = input[i];
    }
    result[21] = 0.5 * (input[0] + input[9]);
    result[22] = 0.5 * (0.5 * (input[0] - input[9]) + 0.5 * (input[0] - input[17])) + input[17];

    get_metacarpal_xr_joints_v1(result);
    return result;
}

std::vector<Vec3f_t> convert_to_26points(const std::vector<Vec3f_t>& input, const bool left_hand) {
    std::vector<Vec3f_t> result(26);
    // input size should be 21
    for (int i = 0; i < input.size(); i++) {
        result[i] = input[i];
    }
    result[21] = 0.5 * (input[0] + input[9]);

    auto root_joint = result[0];
    auto middle_vec = (root_joint - result[9]).normalized();
    root_joint = result[9] + 1.2 * (result[0] - result[9]).norm() * middle_vec;
    result[0] = root_joint;
    result = middle_palm_joint(result, left_hand);
    return result;
}

}  // namespace aisdk::algorithm
